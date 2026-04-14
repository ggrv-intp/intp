# V4 Design -- Hybrid procfs/perf_event/resctrl Architecture

## Rationale

This variant exists to answer a key dissertation question: can IntP's
7 interference metrics be collected without loading any kernel module?

The original IntP (V1) uses SystemTap in guru mode, which compiles embedded C
into a kernel module loaded at runtime. This gives maximum flexibility (direct
access to kernel data structures) but creates three problems:

1. **Fragility**: Kernel internal changes break the script (proven by kernel 6.8)
2. **Safety**: A bug in guru-mode embedded C can crash the kernel
3. **Deployment**: Requires SystemTap + matching kernel debuginfo packages

V4 trades per-event granularity for stability. Instead of instrumenting kernel
functions, it polls stable interfaces that the kernel explicitly exposes for
user-space monitoring.

## Architecture

```text
+------------------+
| intp-hybrid.c    |  Main: arg parsing, polling loop, output formatting
+------------------+
        |
        v
+-------+--------+--------+--------+--------+--------+--------+
| netp  | nets   | blk    | mbw    | llcmr  | llcocc | cpu    |
| sysfs | procfs | procfs | resctrl| perf   | resctrl| procfs |
+-------+--------+--------+--------+--------+--------+--------+
        |
        v
+------------------+
| detect.c         |  Hardware detection (NIC speed, LLC size, etc.)
+------------------+
```

The main loop:

1. Read current values from all metric sources
2. Sleep for the configured interval
3. Read values again
4. Compute deltas and normalize
5. Output in IntP-compatible format
6. Repeat

## Interface Stability

| Interface              | Stable since | ABI guarantee    |
|------------------------|------------- |------------------|
| /proc/stat             | Linux 1.0    | Stable ABI       |
| /proc/diskstats        | Linux 2.6    | Stable ABI       |
| /proc/net/dev          | Linux 1.0    | Stable ABI       |
| /sys/class/net/*/      | Linux 2.6    | Stable sysfs ABI |
| perf_event_open()      | Linux 2.6.31 | Stable syscall   |
| /sys/fs/resctrl/       | Linux 4.10   | Stable ABI       |

## Tradeoff Analysis: Polling vs Event-Driven

The fundamental tradeoff is temporal resolution:

- **Event-driven** (V1 SystemTap): timestamps individual events (packets,
  I/O requests, context switches). Can compute service times and per-event
  latencies. Higher overhead per event but precise attribution.

- **Polling** (V4): reads aggregate counters at fixed intervals. Can compute
  utilization and throughput but not per-event latencies. Lower overhead but
  bounded by sampling interval (typically 1-10 seconds for IntP).

For IntP's use case (interference profiling at 1-second intervals), the
polling approach provides sufficient fidelity for 6 of 7 metrics. Only the
nets (network stack service time) metric loses information, as it requires
per-packet timestamps that polling cannot provide.
