# V4 -- Hybrid procfs/perf_event/resctrl (No Framework)

This variant collects all 7 IntP interference metrics using only stable
kernel interfaces -- no SystemTap, no eBPF, no kernel module. It is a pure C
program that polls procfs, sysfs, perf_event_open(), and the resctrl
filesystem.

## Approach

Each metric uses the most appropriate stable kernel interface:

| Metric | Interface                                      | Method       |
|--------|----------------------------------------------  |--------------|
| netp   | `/sys/class/net/<iface>/statistics/`           | sysfs poll   |
| nets   | `/proc/net/dev`                                | procfs poll  |
| blk    | `/proc/diskstats`                              | procfs poll  |
| mbw    | `/sys/fs/resctrl/mon_data/.../mbm_total_bytes` | resctrl poll |
| llcmr  | `perf_event_open()` with HW_CACHE events       | syscall      |
| llcocc | `/sys/fs/resctrl/mon_groups/.../llc_occupancy` | resctrl poll |
| cpu    | `/proc/stat`                                   | procfs poll  |

## Why This Variant Exists

This is "Path 2" from the portability research: the question of whether IntP's
metrics can be collected without any kernel instrumentation framework at all.
The answer is yes, with one important tradeoff:

- **nets** becomes a polling approximation. The original IntP computes network
  stack service time by timestamping individual packets through kernel probes.
  Without kernel probes, we can only measure aggregate throughput and estimate
  utilization -- we cannot measure per-packet latency or service time.

All other metrics can be collected with equivalent or better fidelity.

## Tradeoffs vs SystemTap (V1-V3)

**Advantages:**

- No kernel module loaded -- zero crash risk
- No debuginfo packages required
- No SystemTap dependency (often unavailable or broken)
- Works across kernel versions with minimal changes
- Trivial deployment: single static binary

**Disadvantages:**

- Polling-based: resolution limited to sampling interval
- Cannot compute per-event service times (nets metric is approximate)
- perf_event_open() requires CAP_PERFMON or root

## Building

```bash
make
```

## Usage

```bash
sudo ./intp-hybrid -p <PID> -i <interval_ms>
```

## Files

- `Makefile` -- Build system
- `intp-hybrid.c` -- Main program (argument parsing, polling loop, output)
- `src/netp.c` -- Network physical utilization (sysfs)
- `src/nets.c` -- Network stack utilization (procfs, approximate)
- `src/blk.c` -- Block I/O utilization (procfs)
- `src/mbw.c` -- Memory bandwidth (resctrl)
- `src/llcmr.c` -- LLC miss ratio (perf_event_open)
- `src/llcocc.c` -- LLC occupancy (resctrl)
- `src/cpu.c` -- CPU utilization (procfs)
- `src/detect.c` -- Hardware detection functions
