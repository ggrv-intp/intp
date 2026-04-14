# V6 Design -- eBPF/CO-RE Architecture

## How CO-RE Works

CO-RE (Compile Once Run Everywhere) solves the portability problem of eBPF
programs. Without CO-RE, eBPF programs that access kernel data structures
must be compiled against the headers of the exact running kernel. CO-RE
enables a single compiled eBPF binary to run on different kernel versions:

1. **Compile time**: The program is compiled against `vmlinux.h` (generated
   from one kernel's BTF). The compiler records relocation information about
   which struct fields are accessed.

2. **Load time**: libbpf reads the target kernel's BTF
   (`/sys/kernel/btf/vmlinux`) and adjusts field offsets in the eBPF bytecode
   to match the running kernel's layout.

3. **Result**: The same `.o` file works across kernel versions as long as the
   fields exist (possibly at different offsets).

### Generating vmlinux.h

```bash
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
```

This generates a single header with ALL kernel type definitions, replacing
the need for kernel headers or debuginfo.

## Comparison with iprof (Gogge 2023, Becker et al. 2024)

iprof is an eBPF-based interference profiler developed at PUCRS as a
predecessor to this work. Key differences:

| Aspect             | iprof                | V6 IntP              |
|--------------------|----------------------|----------------------|
| Metrics            | Subset of IntP       | All 7 IntP metrics   |
| eBPF framework     | libbpf (early)       | libbpf + CO-RE       |
| Data transport     | perf event array     | Ring buffer          |
| Per-thread         | Limited              | Full attribution     |
| resctrl integration| No                   | Yes (mbw, llcocc)    |

## Ring Buffer vs Perf Event Array

V6 uses BPF ring buffer (BPF_MAP_TYPE_RINGBUF) instead of perf event array:

- **Ring buffer** (chosen): Single shared buffer, no per-CPU allocation waste,
  supports variable-length records, no data loss notification.
- **Perf event array**: Per-CPU buffers, fixed-size records, older API but
  more widely supported (kernel 4.x).

Ring buffer requires kernel 5.8+ which is already our minimum.

## Per-Thread Attribution Strategy

IntP needs per-process (and ideally per-thread) metric attribution.
Strategy for each metric:

- **netp, nets, blk**: Filter by PID in the eBPF program using
  `bpf_get_current_pid_tgid()`. Only record events from the target process.
- **cpu**: Track via sched_switch. Record on-CPU time for the target PID.
- **llcmr**: Use perf_event with inherit=1 to count across all threads of
  the target process.
- **mbw, llcocc**: resctrl monitoring group is per-process (handles all
  threads via the tasks file).

## Data Flow

```text
Kernel Space                          User Space
+------------------+                  +------------------+
| tracepoint/net   |--+               |                  |
| tracepoint/block |--+               |                  |
| tracepoint/sched |--+--> Ring Buf --+--> main.c        |
| kprobe/net       |--+               |    (aggregator)  |
| perf_event/cache |--+               |        |         |
+------------------+                  |        v         |
                                      |  +----------+    |
                                      |  | resctrl/ |    |
                                      |  | mbw.c    |    |
                                      |  | llcocc.c |    |
                                      |  +----------+    |
                                      |        |         |
                                      |        v         |
                                      |   IntP output    |
                                      +------------------+
```

## Build System

The build has three stages:

1. Generate `vmlinux.h` from the running kernel's BTF
2. Compile eBPF programs with `clang -O2 -target bpf -g`
3. Generate BPF skeleton header with `bpftool gen skeleton`
4. Compile and link userspace loader against libbpf
