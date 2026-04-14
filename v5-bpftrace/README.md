# V5 -- bpftrace IntP

This variant replaces SystemTap with bpftrace scripts compiled to eBPF bytecode.

## Why bpftrace

- **No debuginfo required**: bpftrace uses BTF (BPF Type Format) for kernel
  type information, which is built into modern kernels (CONFIG_DEBUG_INFO_BTF=y).
  No separate debuginfo packages needed.
- **No kernel crash risk**: eBPF programs are verified by the kernel verifier
  before execution. Invalid memory access, infinite loops, and other dangerous
  operations are rejected at load time.
- **Fast startup**: bpftrace compiles scripts to eBPF bytecode in seconds,
  compared to SystemTap's kernel module compilation (minutes).
- **Widely available**: bpftrace is packaged in all major distributions.

## Architecture

Software metrics (netp, nets, blk, cpu) are collected via bpftrace tracepoints
and kfuncs. Hardware metrics (mbw, llcocc) are collected via the resctrl
filesystem using the same helper as V3/V4. The llcmr metric uses bpftrace's
hardware event support.

| Metric | Collection Method                                       |
|--------|------------------------------------------------------- -|
| netp   | tracepoint:net:netif_receive_skb + net:net_dev_xmit    |
| nets   | kfunc:__dev_queue_xmit + kfunc:napi_complete_done      |
| blk    | tracepoint:block:block_rq_complete                     |
| mbw    | resctrl (shared/intp-resctrl-helper.sh)                |
| llcmr  | hardware:cache-misses / hardware:cache-references      |
| llcocc | resctrl (shared/intp-resctrl-helper.sh)                |
| cpu    | tracepoint:sched:sched_switch                          |

## bpftrace vs SystemTap Comparison

| Aspect              | SystemTap (V1-V3)      | bpftrace (V5)        |
|---------------------|------------------------|----------------------|
| Backend             | Kernel module (.ko)    | eBPF bytecode        |
| Type info source    | DWARF debuginfo        | BTF (built-in)       |
| Safety              | None (guru mode)       | Verifier-enforced    |
| Embedded C          | Yes (guru mode)        | No                   |
| Startup time        | Minutes (compile .ko)  | Seconds              |
| Aggregations        | Manual                 | Built-in (@hist, @)  |
| Per-event detail    | Full                   | Full                 |
| MSR access          | Yes (via embedded C)   | No (use resctrl)     |

## Requirements

- Linux kernel 5.8+ with CONFIG_DEBUG_INFO_BTF=y
- bpftrace >= 0.15.0
- Root privileges (or CAP_BPF + CAP_PERFMON)
- resctrl filesystem for mbw and llcocc

## Usage

```bash
# Run all metrics
sudo ./run-intp-bpftrace.sh <PID> <interval_ms>

# Run individual metric scripts
sudo bpftrace scripts/netp.bt
```

## Files

- `intp.bt` -- Combined bpftrace script (all software metrics)
- `scripts/netp.bt` -- Network physical utilization
- `scripts/nets.bt` -- Network stack utilization (service time)
- `scripts/blk.bt` -- Block I/O utilization
- `scripts/cpu.bt` -- CPU utilization
- `scripts/llcmr.bt` -- LLC miss ratio (hardware events)
- `run-intp-bpftrace.sh` -- Orchestrator script
