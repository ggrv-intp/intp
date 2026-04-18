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

/* Per-request issue timestamp, keyed by request pointer for block svctm. */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 16384);
    __type(key, __u64);
    __type(value, __u64);
} rq_start SEC(".maps");

/* Per-task on-CPU start timestamp, keyed by tid. */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 32768);
    __type(key, __u32);
    __type(value, __u64);
} task_oncpu_start SEC(".maps");

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

/* =====================================================================
 * blk -- block I/O utilization
 * ===================================================================== */

SEC("tracepoint/block/block_rq_issue")
int tp_block_rq_issue(struct trace_event_raw_block_rq *ctx)
{
    /*
     * sector uniquely identifies the request at the moment of issue
     * within a given device. For svctm we really want the request
     * pointer, but that isn't exposed in the tracepoint record. Using
     * (dev << 32) | sector as the key is good enough in practice.
     */
    __u64 dev_sec = ((__u64)BPF_CORE_READ(ctx, dev) << 32)
                   | BPF_CORE_READ(ctx, sector);
    __u64 ts = bpf_ktime_get_ns();
    bpf_map_update_elem(&rq_start, &dev_sec, &ts, BPF_ANY);
    return 0;
}

SEC("tracepoint/block/block_rq_complete")
int tp_block_rq_complete(struct trace_event_raw_block_rq_completion *ctx)
{
    if (!should_monitor_current()) return 0;

    __u64 dev_sec = ((__u64)BPF_CORE_READ(ctx, dev) << 32)
                   | BPF_CORE_READ(ctx, sector);
    __u64 now = bpf_ktime_get_ns();
    __u64 svctm = 0;
    __u64 *start_ts = bpf_map_lookup_elem(&rq_start, &dev_sec);
    if (start_ts) {
        svctm = now - *start_ts;
        bpf_map_delete_elem(&rq_start, &dev_sec);
    }

    struct intp_block_event *e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (!e) return 0;

    fill_header(&e->hdr, INTP_EVENT_BLOCK_COMPLETE);
    e->hdr.ts_ns   = now;
    e->bytes       = BPF_CORE_READ(ctx, nr_sector) * 512;
    e->_pad        = 0;
    e->svctm_ns    = svctm;
    __u32 dev      = BPF_CORE_READ(ctx, dev);
    e->dev_major   = dev >> 20;
    e->dev_minor   = dev & 0xfffff;

    bpf_ringbuf_submit(e, 0);
    return 0;
}

/* =====================================================================
 * cpu -- CPU utilization via sched_switch
 * ===================================================================== */

SEC("tracepoint/sched/sched_switch")
int tp_sched_switch(struct trace_event_raw_sched_switch *ctx)
{
    struct intp_config *cfg = intp_cfg();

    __u32 prev_pid = BPF_CORE_READ(ctx, prev_pid);
    __u32 next_pid = BPF_CORE_READ(ctx, next_pid);
    __u64 now      = bpf_ktime_get_ns();

    /* Finalize the outgoing task's on-CPU interval. */
    __u64 *start_ts = bpf_map_lookup_elem(&task_oncpu_start, &prev_pid);
    if (start_ts && pid_in_filter(cfg, prev_pid)) {
        __u64 delta = now - *start_ts;
        struct intp_sched_event *e =
            bpf_ringbuf_reserve(&events, sizeof(*e), 0);
        if (e) {
            e->hdr.type   = INTP_EVENT_SCHED_SWITCH;
            e->hdr.cpu    = bpf_get_smp_processor_id();
            e->hdr.ts_ns  = now;
            e->hdr.pid    = prev_pid;
            e->hdr.tid    = prev_pid;
            e->prev_pid   = prev_pid;
            e->next_pid   = next_pid;
            e->on_cpu_ns  = delta;
            bpf_ringbuf_submit(e, 0);
        }
    }
    if (start_ts)
        bpf_map_delete_elem(&task_oncpu_start, &prev_pid);

    /* Start timing the incoming task, unless it is the idle task. */
    if (next_pid != 0)
        bpf_map_update_elem(&task_oncpu_start, &next_pid, &now, BPF_ANY);

    return 0;
}
