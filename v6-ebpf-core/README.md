# V6 -- eBPF/CO-RE IntP with libbpf

This is the full eBPF prototype for the dissertation's three-way comparison:
original IntP (SystemTap) vs refactored IntP vs eBPF prototype.

## Approach

Uses C + libbpf with CO-RE (Compile Once Run Everywhere) and BTF (BPF Type
Format) for portable, safe kernel instrumentation:

- **CO-RE**: eBPF programs are compiled once and relocated at load time using
  BTF type information. The same binary runs across different kernel versions
  without recompilation.
- **BTF**: The kernel exposes its type information at `/sys/kernel/btf/vmlinux`.
  This replaces the need for DWARF debuginfo packages.
- **Verifier safety**: All eBPF programs pass through the kernel verifier before
  execution, which guarantees no crashes, no infinite loops, no invalid memory
  access.

## Metrics

| Metric | eBPF Collection Method | Userspace Augmentation |
| -------- | ------------------------ | ------------------------ |
| netp | tracepoint:net:net_dev_xmit, netif_receive_skb | - |
| nets | kprobe:__dev_queue_xmit, kprobe:napi_complete_done | - |
| blk | tracepoint:block:block_rq_issue, block_rq_complete | - |
| mbw | - | resctrl MBM reader |
| llcmr | perf_event (HW_CACHE counters) | - |
| llcocc | - | resctrl llc_occupancy reader |
| cpu | tracepoint:sched:sched_switch | - |

Software metrics are collected in-kernel via eBPF programs attached to
tracepoints and kprobes. Data is passed to userspace via a BPF ring buffer.
Hardware metrics (mbw, llcocc) use resctrl filesystem readers in userspace.

## Requirements

- Linux kernel 5.8+ with CONFIG_DEBUG_INFO_BTF=y
- clang >= 11 (for BPF target compilation)
- libbpf >= 0.8
- bpftool (for vmlinux.h generation)
- resctrl filesystem for mbw and llcocc

## Building

```bash
make           # Generates vmlinux.h, compiles BPF programs, builds loader
```

## Usage

```bash
sudo ./intp-ebpf -p <PID> -i <interval_ms>
```

## Files

- `Makefile` -- Build system (vmlinux.h gen, BPF compile, userspace link)
- `src/vmlinux.h` -- Generated kernel type definitions (placeholder)
- `src/intp_kern.bpf.h` -- Shared header (kernel/userspace data structures)
- `src/intp_kern.bpf.c` -- eBPF kernel programs (one per metric)
- `src/main.c` -- Userspace loader and metric aggregator
- `resctrl/mbw.c` -- resctrl MBM reader (userspace)
- `resctrl/llcocc.c` -- resctrl LLC occupancy reader (userspace)

## References

- libbpf-bootstrap: <https://github.com/libbpf/libbpf-bootstrap>
- Gogge (2023): iprof -- eBPF-based interference profiler
- Becker et al. (2024): Cloud interference profiling with eBPF
- Nakryiko, A.: BPF CO-RE reference guide
