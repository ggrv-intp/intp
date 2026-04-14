# IntP -- Interference Profiler: Multi-Variant Comparison

This repository contains six implementation variants of IntP, an interference
profiler that collects 7 metrics from the Linux kernel. The variants are
organized for systematic comparison as part of a Master's dissertation on
kernel instrumentation for interference profiling (PPGCC/PUCRS, advisor
Prof. Cesar De Rose). The research compares the original SystemTap-based IntP
against modern instrumentation approaches (procfs polling, bpftrace, eBPF/CO-RE)
to evaluate portability, safety, and measurement fidelity tradeoffs.

## Variant Comparison

| Feature                  | V1 original | V2 updated | V3 resctrl | V4 hybrid | V5 bpftrace | V6 ebpf-core |
|--------------------------|:-----------:|:----------:|:----------:|:---------:|:-----------:|:------------:|
| Kernel module required   |     Yes     |    Yes     |    Yes     |    No     |     No      |      No      |
| Debuginfo required       |     Yes     |    Yes     |    Yes     |    No     |   No (BTF)  |   No (BTF)   |
| Kernel crash risk        |    High     |   High     |   High     |   None    |    None     |     None     |
| Min kernel version       |   <=6.6     |   6.8+     |   6.8+     |   4.10+   |    5.8+     |     5.8+     |
| netp                     |      x      |     x      |     x      |     x     |      x      |       x      |
| nets (service-time)      |      x      |     x      |     x      |     ~     |      x      |       x      |
| blk                      |      x      |     x      |     x      |     x     |      x      |       x      |
| mbw                      |      x      |     x      |     x      |     x     |      x      |       x      |
| llcmr                    |      x      |     x      |     x      |     x     |      x      |       x      |
| llcocc                   |      x      |            |     x      |     x     |      x      |       x      |
| cpu                      |      x      |     x      |     x      |     x     |      x      |       x      |
| Framework                | SystemTap   | SystemTap  | SystemTap  |   None    |  bpftrace   |    libbpf    |
| AMD EPYC compatible      |   Partial   |  Partial   |  Partial   |    Yes    |     Yes     |      Yes     |
| ARM server compatible    |     No      |    No      |    No      |  Partial  |   Partial   |    Partial   |

x = supported, ~ = polling approximation, empty = not supported

## The 7 Metrics

- **netp** -- Network physical utilization (NIC TX+RX bandwidth)
- **nets** -- Network stack utilization (kernel networking service time)
- **blk** -- Block I/O utilization (disk busy percentage)
- **mbw** -- Memory bandwidth utilization (LLC-to-DRAM traffic)
- **llcmr** -- LLC miss ratio (cache misses / cache references)
- **llcocc** -- LLC occupancy (bytes of last-level cache occupied)
- **cpu** -- CPU utilization (user + system time percentage)

## Directory Layout

```text
.
|-- README.md                  This file
|-- LICENSE                    MIT license
|-- docs/                      Cross-variant documentation
|   |-- METRICS-DEEP-DIVE.md   Technical details of all 7 metrics
|   |-- KERNEL-6.8-CHANGES.md  What kernel 6.8 broke and why
|   |-- PORTABILITY-ROADMAP.md Portability analysis
|   |-- HARDWARE-COMPATIBILITY.md  Hardware feature tables
|   |-- VARIANT-COMPARISON.md  Detailed variant rationale
|-- shared/                    Components used across variants
|   |-- intp-detect.sh         Hardware capability detection
|   |-- intp-resctrl-helper.sh resctrl companion daemon
|-- v1-original/               Unmodified 2022 IntP (SystemTap)
|-- v2-updated/                Kernel 6.8 patch (LLC disabled)
|-- v3-updated-resctrl/        Kernel 6.8+ with resctrl LLC
|-- v4-hybrid-procfs/          procfs/perf_event/resctrl [scaffold]
|-- v5-bpftrace/               bpftrace scripts + resctrl [scaffold]
|-- v6-ebpf-core/              Full eBPF/CO-RE with libbpf [scaffold]
```

## Quick Start

### V1 -- Original IntP (kernel <= 6.6)

```bash
cd v1-original
sudo stap -g intp.stp <PID> <interval_ms>
```

Requires: SystemTap, kernel debuginfo, kernel <= 6.6.

### V2 -- Updated for Kernel 6.8 (LLC disabled)

```bash
cd v2-updated
sudo stap -g intp-6.8.stp <PID> <interval_ms>
```

Requires: SystemTap, kernel debuginfo, kernel 6.8+. Note: llcocc returns 0.

### V3 -- Resctrl Solution (full 7/7 metrics)

```bash
cd v3-updated-resctrl
# Start the resctrl helper daemon first:
sudo ../shared/intp-resctrl-helper.sh start <PID>
sudo stap -g intp-resctrl.stp <PID> <interval_ms>
```

Requires: SystemTap, kernel debuginfo, kernel 6.8+, Intel RDT or AMD PQoS.

### V4 -- Hybrid procfs (scaffold)

```bash
cd v4-hybrid-procfs
make
sudo ./intp-hybrid -p <PID> -i <interval_ms>
```

No framework dependencies. Requires: resctrl for mbw/llcocc.

### V5 -- bpftrace (scaffold)

```bash
cd v5-bpftrace
sudo ./run-intp-bpftrace.sh <PID> <interval_ms>
```

Requires: bpftrace, kernel BTF, resctrl for mbw/llcocc.

### V6 -- eBPF/CO-RE (scaffold)

```bash
cd v6-ebpf-core
make
sudo ./intp-ebpf -p <PID> -i <interval_ms>
```

Requires: libbpf, clang, kernel BTF, resctrl for mbw/llcocc.

## Documentation

- [Metrics Deep Dive](docs/METRICS-DEEP-DIVE.md) -- Kernel probe points, formulas, constants
- [Kernel 6.8 Changes](docs/KERNEL-6.8-CHANGES.md) -- What broke and the fix paths
- [Portability Roadmap](docs/PORTABILITY-ROADMAP.md) -- Cross-kernel, cross-arch analysis
- [Hardware Compatibility](docs/HARDWARE-COMPATIBILITY.md) -- RDT, PQoS, MPAM tables
- [Variant Comparison](docs/VARIANT-COMPARISON.md) -- Detailed rationale for each variant

## References

- Original IntP: [projectintp/intp](https://github.com/projectintp/intp)
- Gogge, L. M. (2023). iprof -- eBPF-based interference profiler
- Becker et al. (2024). Cloud interference profiling with eBPF
- De Rose et al. (2022). IntP -- Interference Profiler for Linux

## License

MIT -- see [LICENSE](LICENSE).
