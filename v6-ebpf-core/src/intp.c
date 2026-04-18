/*
 * intp.c -- userspace main for IntP V6 (eBPF / CO-RE / libbpf).
 *
 * First cut: open + load the BPF skeleton, attach tracepoints/kprobes,
 * push a system-wide config into the PID-filter map, and consume the
 * ring buffer with aggregation per event type. Every 1 s we emit a
 * V1-byte-compatible TSV record.
 *
 * llcmr requires attaching the SEC("perf_event") programs to perf fds;
 * that wiring arrives with the resctrl integration (resctrl handles
 * mbw/llcocc) so at this commit llcmr reports zero. Arg parsing and
 * resctrl land in the next two commits.
 */

#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "intp.bpf.h"
#include "intp.skel.h"

#include "../detect/detect.h"

static volatile sig_atomic_t g_running = 1;
static void on_signal(int sig) { (void)sig; g_running = 0; }

/* ------------------------------------------------------------------ state */

typedef struct {
    unsigned long long tx_bytes;
    unsigned long long rx_bytes;
    unsigned long long tx_lat_ns_sum;
    unsigned long long tx_lat_n;
    unsigned long long rx_lat_ns_sum;
    unsigned long long rx_lat_n;
    unsigned long long blk_svctm_ns_sum;
    unsigned long long blk_ops;
    unsigned long long blk_bytes;
    unsigned long long cpu_on_ns_sum;
    unsigned long long llc_refs;
    unsigned long long llc_misses;
} intp_state_t;

typedef struct {
    double netp, nets, blk, mbw, llcmr, llcocc, cpu;
} intp_sample_t;

static intp_state_t *g_state;

static int handle_event(void *ctx, void *data, size_t size)
{
    (void)ctx;
    if (size < sizeof(struct intp_event_header)) return 0;
    struct intp_event_header *hdr = data;

    switch (hdr->type) {
    case INTP_EVENT_NET_XMIT:
        if (size >= sizeof(struct intp_net_event))
            g_state->tx_bytes += ((struct intp_net_event *)data)->bytes;
        break;
    case INTP_EVENT_NET_RECV:
        if (size >= sizeof(struct intp_net_event))
            g_state->rx_bytes += ((struct intp_net_event *)data)->bytes;
        break;
    case INTP_EVENT_NAPI_TX_LAT:
        if (size >= sizeof(struct intp_netstack_event)) {
            g_state->tx_lat_ns_sum +=
                ((struct intp_netstack_event *)data)->latency_ns;
            g_state->tx_lat_n++;
        }
        break;
    case INTP_EVENT_NAPI_RX_LAT:
        if (size >= sizeof(struct intp_netstack_event)) {
            g_state->rx_lat_ns_sum +=
                ((struct intp_netstack_event *)data)->latency_ns;
            g_state->rx_lat_n++;
        }
        break;
    case INTP_EVENT_BLOCK_COMPLETE:
        if (size >= sizeof(struct intp_block_event)) {
            struct intp_block_event *b = data;
            g_state->blk_ops++;
            g_state->blk_bytes        += b->bytes;
            g_state->blk_svctm_ns_sum += b->svctm_ns;
        }
        break;
    case INTP_EVENT_SCHED_SWITCH:
        if (size >= sizeof(struct intp_sched_event))
            g_state->cpu_on_ns_sum +=
                ((struct intp_sched_event *)data)->on_cpu_ns;
        break;
    case INTP_EVENT_PERF_SAMPLE:
        if (size >= sizeof(struct intp_perf_event)) {
            struct intp_perf_event *p = data;
            if (p->perf_type == 0) g_state->llc_refs   += p->value;
            else                   g_state->llc_misses += p->value;
        }
        break;
    }
    return 0;
}

/* ------------------------------------------------------------------ metric math */

static double safe_pct(double num, double den)
{
    if (den <= 0.0) return 0.0;
    double p = num / den * 100.0;
    if (p < 0.0)   p = 0.0;
    if (p > 100.0) p = 100.0;
    return p;
}

static void compute_sample(const intp_state_t *st,
                           const system_capabilities_t *caps,
                           double interval_sec,
                           intp_sample_t *out)
{
    memset(out, 0, sizeof(*out));

    long max_nic = caps->nic_speed_bps > 0 ? caps->nic_speed_bps : 125000000L;
    double bytes_per_sec = (double)(st->tx_bytes + st->rx_bytes) / interval_sec;
    out->netp = safe_pct(bytes_per_sec, (double)max_nic);

    double net_lat_total = (double)(st->tx_lat_ns_sum + st->rx_lat_ns_sum);
    double interval_ns   = interval_sec * 1e9;
    out->nets = safe_pct(net_lat_total, interval_ns);

    out->blk = safe_pct((double)st->blk_svctm_ns_sum, interval_ns);

    int num_cores = caps->num_cores > 0 ? caps->num_cores : 1;
    out->cpu = safe_pct((double)st->cpu_on_ns_sum, interval_ns * num_cores);

    out->llcmr = safe_pct((double)st->llc_misses, (double)st->llc_refs);
}

static void emit_tsv_header(FILE *out, const system_capabilities_t *caps)
{
    fprintf(out,
        "# v6 ebpf-core -- netp:tracepoint nets:kprobe blk:tracepoint"
        " cpu:sched_switch llcmr:off mbw:off llcocc:off\n");
    fprintf(out, "# kernel %d.%d env=%s\n",
            caps->kernel_major, caps->kernel_minor,
            caps->env == ENV_CONTAINER ? "container" :
            caps->env == ENV_VM        ? "vm" : "bare-metal");
    fprintf(out, "netp\tnets\tblk\tmbw\tllcmr\tllcocc\tcpu\n");
    fflush(out);
}

static void emit_tsv(FILE *out, const intp_sample_t *s)
{
    fprintf(out, "%02d\t%02d\t%02d\t%02d\t%02d\t%02d\t%02d\n",
            (int)(s->netp   + 0.5),
            (int)(s->nets   + 0.5),
            (int)(s->blk    + 0.5),
            (int)(s->mbw    + 0.5),
            (int)(s->llcmr  + 0.5),
            (int)(s->llcocc + 0.5),
            (int)(s->cpu    + 0.5));
    fflush(out);
}

/* ------------------------------------------------------------------ main */

int main(void)
{
    system_capabilities_t caps;
    detect_all(&caps);

    struct intp_bpf *skel = intp_bpf__open();
    if (!skel) {
        fprintf(stderr, "failed to open BPF skeleton: %s\n", strerror(errno));
        return 1;
    }

    /* perf_event programs need userspace to attach them to perf fds; until
     * the args + resctrl commits land, skip them so intp_bpf__attach() does
     * not try (and fail) on autoload. */
    bpf_program__set_autoload(skel->progs.perf_llc_refs,   false);
    bpf_program__set_autoload(skel->progs.perf_llc_misses, false);

    if (intp_bpf__load(skel)) {
        fprintf(stderr, "failed to load BPF: %s\n", strerror(errno));
        intp_bpf__destroy(skel);
        return 1;
    }

    struct intp_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.system_wide = 1;
    unsigned int cfg_key = 0;
    if (bpf_map__update_elem(skel->maps.intp_cfg_map, &cfg_key, sizeof(cfg_key),
                             &cfg, sizeof(cfg), BPF_ANY) != 0) {
        fprintf(stderr, "failed to write intp_config: %s\n", strerror(errno));
        intp_bpf__destroy(skel);
        return 1;
    }

    if (intp_bpf__attach(skel)) {
        fprintf(stderr, "failed to attach BPF programs: %s\n", strerror(errno));
        intp_bpf__destroy(skel);
        return 1;
    }

    intp_state_t state;
    memset(&state, 0, sizeof(state));
    g_state = &state;

    struct ring_buffer *rb =
        ring_buffer__new(bpf_map__fd(skel->maps.events), handle_event, NULL, NULL);
    if (!rb) {
        fprintf(stderr, "failed to create ring_buffer: %s\n", strerror(errno));
        intp_bpf__destroy(skel);
        return 1;
    }

    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);
    setvbuf(stdout, NULL, _IOLBF, 0);

    emit_tsv_header(stdout, &caps);

    struct timespec tick;
    clock_gettime(CLOCK_MONOTONIC, &tick);
    const double interval_sec = 1.0;

    while (g_running) {
        ring_buffer__poll(rb, 200);

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (double)(now.tv_sec  - tick.tv_sec)
                       + (double)(now.tv_nsec - tick.tv_nsec) / 1e9;
        if (elapsed < interval_sec) continue;

        intp_sample_t sample;
        compute_sample(&state, &caps, elapsed, &sample);
        emit_tsv(stdout, &sample);
        memset(&state, 0, sizeof(state));
        tick = now;
    }

    ring_buffer__free(rb);
    intp_bpf__destroy(skel);
    return 0;
}
