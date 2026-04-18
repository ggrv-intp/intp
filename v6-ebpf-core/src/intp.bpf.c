/*
 * intp.bpf.c -- kernel-side programs for IntP V6.
 *
 * This is the canonical eBPF implementation: one translation unit holds
 * every probe, maps, and the shared ring buffer. libbpf relocates every
 * BPF_CORE_READ at load time against the running kernel's BTF so the
 * same compiled .o runs across kernel versions (5.8+).
 *
 * This initial drop wires up the scaffolding (ringbuf, config map, PID
 * filter helper) and the netp path. Subsequent commits layer in blk,
 * cpu, nets, and llcmr.
 */

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_tracing.h>

#include "intp.bpf.h"

char LICENSE[] SEC("license") = "Dual MIT/GPL";

/* ------------------------------------------------------------------ Maps */

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, INTP_RINGBUF_BYTES);
} events SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, struct intp_config);
} intp_cfg_map SEC(".maps");

/* -------------------------------------------------------------- Helpers */

static __always_inline struct intp_config *intp_cfg(void)
{
    __u32 key = 0;
    return bpf_map_lookup_elem(&intp_cfg_map, &key);
}

/* Returns 1 if pid is to be observed under the current config. */
static __always_inline int pid_in_filter(struct intp_config *cfg, __u32 pid)
{
    if (!cfg)              return 1;
    if (cfg->system_wide)  return 1;

    __u32 n = cfg->num_target_pids;
    if (n > INTP_MAX_PIDS) n = INTP_MAX_PIDS;

    for (__u32 i = 0; i < INTP_MAX_PIDS; i++) {
        if (i >= n) break;
        if (cfg->target_pids[i] == pid) return 1;
    }
    return 0;
}

/* Same, but short-circuits using the current task's pid. */
static __always_inline int should_monitor_current(void)
{
    struct intp_config *cfg = intp_cfg();
    __u32 pid = bpf_get_current_pid_tgid() >> 32;
    return pid_in_filter(cfg, pid);
}

static __always_inline void
fill_header(struct intp_event_header *hdr, __u32 type)
{
    hdr->type  = type;
    hdr->cpu   = bpf_get_smp_processor_id();
    hdr->ts_ns = bpf_ktime_get_ns();
    __u64 pid_tgid = bpf_get_current_pid_tgid();
    hdr->pid   = pid_tgid >> 32;
    hdr->tid   = (__u32)pid_tgid;
}

/* =====================================================================
 * netp -- network physical utilization
 * ===================================================================== */

SEC("tracepoint/net/net_dev_xmit")
int tp_net_dev_xmit(struct trace_event_raw_net_dev_xmit *ctx)
{
    if (!should_monitor_current()) return 0;

    struct intp_net_event *e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (!e) return 0;

    fill_header(&e->hdr, INTP_EVENT_NET_XMIT);
    e->bytes = BPF_CORE_READ(ctx, len);
    e->_pad  = 0;

    bpf_ringbuf_submit(e, 0);
    return 0;
}

SEC("tracepoint/net/netif_receive_skb")
int tp_netif_receive_skb(struct trace_event_raw_net_dev_template *ctx)
{
    /* netif_receive_skb runs in softirq context; current task is whoever
     * was interrupted, so PID filtering here is only approximate. For
     * per-PID accuracy we'd need to correlate by socket owner, which
     * isn't cheap. Emit everything and let userspace decide when in
     * system-wide mode; filter in per-PID mode. */
    if (!should_monitor_current()) return 0;

    struct intp_net_event *e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (!e) return 0;

    fill_header(&e->hdr, INTP_EVENT_NET_RECV);
    e->bytes = BPF_CORE_READ(ctx, len);
    e->_pad  = 0;

    bpf_ringbuf_submit(e, 0);
    return 0;
}
