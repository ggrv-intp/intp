/*
 * intp_kern.bpf.c -- IntP eBPF Kernel Programs (V6)
 *
 * Contains eBPF programs for collecting IntP's software metrics:
 *   - netp: network physical utilization (tracepoint:net)
 *   - nets: network stack service time (kprobe:net)
 *   - blk:  block I/O utilization (tracepoint:block)
 *   - cpu:  CPU utilization (tracepoint:sched)
 *
 * Hardware metrics (mbw, llcocc) are collected in userspace via resctrl.
 * LLC miss ratio (llcmr) uses perf_event in userspace.
 *
 * Data is passed to userspace via a BPF ring buffer.
 *
 * Build: clang -O2 -target bpf -g -c intp_kern.bpf.c -o intp_kern.bpf.o
 */

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "intp_kern.bpf.h"

/* Target PID -- set by userspace before loading */
const volatile int target_pid = 0;

/* ---- BPF Maps ---- */

/* Ring buffer for sending events to userspace */
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, INTP_RINGBUF_SIZE);
} events SEC(".maps");

/* Hash map for tracking TX timestamps (nets metric) */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, INTP_MAX_ENTRIES);
    __type(key, __u32);    /* tid */
    __type(value, __u64);  /* timestamp */
} tx_start SEC(".maps");

/* Hash map for tracking NAPI timestamps (nets metric) */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, INTP_MAX_ENTRIES);
    __type(key, __u32);    /* tid */
    __type(value, __u64);  /* timestamp */
} rx_start SEC(".maps");

/* Hash map for tracking block I/O issue timestamps */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, INTP_MAX_ENTRIES);
    __type(key, __u64);    /* sector */
    __type(value, __u64);  /* timestamp */
} blk_start SEC(".maps");

/* Hash map for tracking CPU on-time */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, INTP_MAX_ENTRIES);
    __type(key, __u32);    /* pid */
    __type(value, __u64);  /* timestamp */
} cpu_on SEC(".maps");

/* ---- Helper ---- */

static __always_inline int pid_matches(void)
{
    __u32 pid = bpf_get_current_pid_tgid() >> 32;
    return target_pid == 0 || pid == target_pid;
}

/* ===========================================================================
 * netp -- Network Physical Utilization
 * Equivalent to: print_netphy_report() in v1-original/intp.stp
 *
 * Tracepoints:
 *   net:netif_receive_skb -- each received packet (skb->len bytes)
 *   net:net_dev_xmit      -- each transmitted packet (skb->len bytes)
 * =========================================================================== */

SEC("tracepoint/net/netif_receive_skb")
int handle_netif_receive_skb(struct trace_event_raw_net_dev_template *ctx)
{
    /* TODO: Check PID filter */
    /* TODO: Read packet length from tracepoint args */
    /* TODO: Submit netp event to ring buffer */
    /* struct intp_event *e = bpf_ringbuf_reserve(&events, sizeof(*e), 0); */
    /* if (!e) return 0; */
    /* e->type = INTP_EVENT_NETP; */
    /* e->netp.rx_bytes = ctx->len; */
    /* bpf_ringbuf_submit(e, 0); */
    return 0;
}

SEC("tracepoint/net/net_dev_xmit")
int handle_net_dev_xmit(struct trace_event_raw_net_dev_xmit *ctx)
{
    /* TODO: Check PID filter */
    /* TODO: Read packet length and submit netp event */
    return 0;
}

/* ===========================================================================
 * nets -- Network Stack Service Time
 * Equivalent to: print_netstack_report() in v1-original/intp.stp
 *
 * TX: kprobe __dev_queue_xmit (entry) -> net_dev_xmit tracepoint (exit)
 * RX: kprobe __napi_schedule_irqoff (entry) -> napi_complete_done (exit)
 * =========================================================================== */

SEC("kprobe/__dev_queue_xmit")
int handle_dev_queue_xmit(struct pt_regs *ctx)
{
    /* TODO: Store entry timestamp in tx_start map */
    /* __u32 tid = bpf_get_current_pid_tgid(); */
    /* __u64 ts = bpf_ktime_get_ns(); */
    /* bpf_map_update_elem(&tx_start, &tid, &ts, BPF_ANY); */
    return 0;
}

SEC("kprobe/napi_complete_done")
int handle_napi_complete(struct pt_regs *ctx)
{
    /* TODO: Compute RX service time */
    /* TODO: Submit nets event to ring buffer */
    return 0;
}

/* ===========================================================================
 * blk -- Block I/O Utilization
 * Equivalent to: print_block_report() in v1-original/intp.stp
 *
 * Tracepoints:
 *   block:block_rq_issue    -- I/O request sent to device
 *   block:block_rq_complete -- I/O request completed
 * =========================================================================== */

SEC("tracepoint/block/block_rq_issue")
int handle_block_rq_issue(struct trace_event_raw_block_rq *ctx)
{
    /* TODO: Store issue timestamp keyed by sector */
    /* __u64 sector = ctx->sector; */
    /* __u64 ts = bpf_ktime_get_ns(); */
    /* bpf_map_update_elem(&blk_start, &sector, &ts, BPF_ANY); */
    return 0;
}

SEC("tracepoint/block/block_rq_complete")
int handle_block_rq_complete(struct trace_event_raw_block_rq_completion *ctx)
{
    /* TODO: Look up issue timestamp */
    /* TODO: Compute service time */
    /* TODO: Submit blk event to ring buffer */
    return 0;
}

/* ===========================================================================
 * cpu -- CPU Utilization
 * Equivalent to: print_cpu_report() in v1-original/intp.stp
 *
 * Tracepoint:
 *   sched:sched_switch -- context switch between processes
 * =========================================================================== */

SEC("tracepoint/sched/sched_switch")
int handle_sched_switch(struct trace_event_raw_sched_switch *ctx)
{
    /* TODO: Measure on-CPU time for prev_pid */
    /* __u32 prev_pid = ctx->prev_pid; */
    /* __u32 next_pid = ctx->next_pid; */

    /* Look up when prev_pid went on-CPU */
    /* __u64 *start_ts = bpf_map_lookup_elem(&cpu_on, &prev_pid); */
    /* if (start_ts) { */
    /*     __u64 delta = bpf_ktime_get_ns() - *start_ts; */
    /*     // Submit cpu event for prev_pid */
    /*     bpf_map_delete_elem(&cpu_on, &prev_pid); */
    /* } */

    /* Record when next_pid goes on-CPU */
    /* __u64 ts = bpf_ktime_get_ns(); */
    /* bpf_map_update_elem(&cpu_on, &next_pid, &ts, BPF_ANY); */

    return 0;
}

/* License declaration (required for eBPF programs using GPL helpers) */
char LICENSE[] SEC("license") = "Dual MIT/GPL";
