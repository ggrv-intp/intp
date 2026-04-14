# V5 Design -- bpftrace IntP

## bpftrace vs SystemTap: What Changes

### What bpftrace CAN do that SystemTap does

- Attach to tracepoints (sched, block, net, etc.)
- Attach to kfuncs/kretfuncs (kernel functions via BTF)
- Read function arguments and return values
- Timestamp events and compute latencies
- Aggregate data with built-in maps (@variable)
- Filter by PID, TID, or other conditions
- Access hardware performance counters

### What bpftrace CANNOT do that SystemTap guru mode does

- **Direct MSR access**: SystemTap guru mode can execute arbitrary C including
  rdmsr/wrmsr. bpftrace cannot read MSRs. This affects the original llcocc
  implementation (QOS_L3_OCC MSR). Solution: use resctrl instead.

- **Arbitrary memory access**: SystemTap guru mode allows unchecked pointer
  dereference. bpftrace/eBPF requires all memory access to go through
  bpf_probe_read(). This is a safety feature, not a limitation.

- **Complex data structures**: SystemTap can traverse linked lists and complex
  kernel structures. bpftrace can access struct fields (via BTF) but complex
  traversals require helper functions.

- **Loops**: eBPF programs have bounded execution. bpftrace unrolls loops at
  compile time. This means no variable-length iteration within a probe.

### Key Translation Patterns

Original IntP pattern -> bpftrace equivalent:

1. `probe kernel.function("X") { ... }`
   -> `kfunc:X { ... }` or `kretfunc:X { ... }`

2. `probe kernel.trace("net:net_dev_xmit") { ... }`
   -> `tracepoint:net:net_dev_xmit { ... }`

3. `%{ /* embedded C */ %}` (guru mode)
   -> Not available. Use bpftrace built-in functions or resctrl.

4. `@variable[key] = value` (stats/aggregation)
   -> `@variable[key] = value` (same syntax, eBPF maps)

5. `perf_event.counter("cache-misses")`
   -> `hardware:cache-misses:SAMPLE_PERIOD { ... }`

## Metric-by-Metric Translation

### netp (Network Physical)

Original: probes on netif_receive_skb and net_dev_xmit tracepoints,
accumulates skb->len per interval.

bpftrace: identical approach using tracepoint:net:* probes.

### nets (Network Stack Service Time)

Original: timestamps packets at dev_queue_xmit entry, measures at
net_dev_xmit completion. Service time = exit - entry timestamp.

bpftrace: kfunc:__dev_queue_xmit stores timestamp in @start[tid],
tracepoint:net:net_dev_xmit computes delta. Equivalent fidelity.

### blk (Block I/O)

Original: probes on block request issue and completion tracepoints.

bpftrace: tracepoint:block:block_rq_issue stores timestamp,
tracepoint:block:block_rq_complete computes service time.

### mbw (Memory Bandwidth)

Original: perf_event for MBM counters.

bpftrace: Cannot directly access MBM counters. Use resctrl filesystem
(same as V3/V4). The run script reads resctrl in parallel.

### llcmr (LLC Miss Ratio)

Original: perf_event for cache-references and cache-misses.

bpftrace: hardware:cache-misses and hardware:cache-references probes
with sampling. Compute ratio from accumulated counts.

### llcocc (LLC Occupancy)

Original: embedded C reading cqm_rmid and QOS_L3_OCC MSR.

bpftrace: Cannot access MSRs. Use resctrl filesystem (same as V3/V4).

### cpu (CPU Utilization)

Original: sched_switch tracepoint with per-CPU time accounting.

bpftrace: tracepoint:sched:sched_switch with @cpu_time[pid] tracking.
Equivalent fidelity.
