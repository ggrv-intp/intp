# Variant Comparison

Detailed comparison of all 6 implementation variants with rationale for each.

## Overview

The dissertation compares six instrumentation approaches for collecting the
same 7 interference metrics. Each variant represents a different point in the
tradeoff space between measurement fidelity, portability, safety, and
deployment complexity.

## Variants

### V1 -- Original IntP (SystemTap, kernel <= 6.6)

The unmodified 2022 baseline. Uses SystemTap guru mode with embedded C to
access kernel internals directly. Maximum measurement fidelity but fragile
across kernel versions.

### V2 -- Updated for Kernel 6.8 (LLC disabled)

Minimal patch to compile on kernel 6.8+. Disables LLC occupancy (returns 0).
Demonstrates the fragility of the embedded C approach.

### V3 -- Resctrl Solution (kernel 6.8+, full metrics)

Restores 7/7 metrics by using the resctrl filesystem for LLC occupancy.
Introduces a companion daemon pattern that later variants also use.

### V4 -- Hybrid procfs/perf_event/resctrl (no framework)

**Architecture summary.** V4 is a single C99 binary
(`v4-hybrid-procfs/intp-hybrid`) with no kernel module, no debuginfo
dependency, and no compile-time selection of a collection path. Each of
the seven metrics (netp, nets, blk, mbw, llcmr, llcocc, cpu) carries an
ordered list of backends. At startup the binary runs a capability
detection pass (CPU vendor, resctrl availability, perf_event_paranoid,
execution environment, PMU passthrough for VMs) and then probes each
metric's backends in order, binding the first one that succeeds. The
output is IntP-compatible 7-column TSV, plus JSON and Prometheus
exposition formats. A leading `# v4 backends:` banner in the TSV lets
downstream consumers see which backend produced each column.

**Backend hierarchy per metric.** The decision tree below is the
operational contract of V4; `v4-hybrid-procfs/DESIGN.md` section 2
carries the full per-backend detail including minimum kernel versions
and privilege requirements.

| metric | 1st choice     | 2nd choice            | 3rd / 4th choice          |
|--------|----------------|-----------------------|---------------------------|
| netp   | sysfs          | procfs                |                           |
| nets   | procfs_softirq | procfs_throughput     |                           |
| blk    | diskstats      | sysfs                 |                           |
| mbw    | resctrl_mbm    | perf_uncore_imc       | perf_amd_df, perf_arm_cmn |
| llcmr  | perf_hwcache   | perf_raw              |                           |
| llcocc | resctrl        | proxy_from_miss_ratio |                           |
| cpu    | procfs_pid     | procfs_system         |                           |

**Measurement fidelity.** V4 produces seconds-resolution aggregate data,
not sub-second event capture. Samples are integrated over the
`--interval` window (default 1 s) and reported as a single value per
metric per sample. This is an intentional design decision, not a
workaround: IntP's target is steady-state interference
characterisation, and aggregate readings from stable kernel ABIs are
sufficient for that regime. When a sample cannot be computed reliably
(e.g. LLC miss ratio with no cache activity in the interval), the
runtime carries an explicit `status` field (`ok`, `degraded`, `proxy`,
`unavailable`) plus an optional `note` on each sample, so consumers can
always tell a real reading from a fallback or an approximation.

**Deployment requirements.** The binary depends only on glibc and
libpthread. `/sys/fs/resctrl` is optional: when mounted (or mountable
by root) the hardware metrics `mbw` and `llcocc` use kernel resctrl
byte counters, otherwise they fall back to perf uncore counters or to
the llcmr-based directional proxy. Uncore PMU access requires
`perf_event_paranoid<=-1` or `CAP_PERFMON`/`CAP_SYS_ADMIN`. The build
is a plain `make` against C99 flags with `-Wall -Wextra -Wpedantic
-Wshadow -Wstrict-prototypes -Wmissing-prototypes`; a Debian package
build is provided via `scripts/build-deb.sh`. No kernel module, no BTF,
no libbpf, no SystemTap runtime, no Python at collection time.

**Performance overhead analysis.** Definitive numbers come from the
Phase 3 evaluation, which runs the same workload under V1, V3, and V4
bare-metal / container / VM and reports RSS, CPU%, and per-metric
correlation. The expected order of magnitude for the polling loop is
around 10^2 microseconds per sample: one `fread` each on `/proc/stat`,
`/proc/diskstats`, `/proc/net/dev`, `/proc/softirqs`, a handful of
sysfs byte reads, and either a resctrl byte-counter read or a
`read(perf_fd)`. None of these allocate in the hot path. The loop
itself uses `clock_nanosleep(TIMER_ABSTIME)` so overhead is
deterministic and drift-free; there are no event storms, no verifier
stalls, and no probe re-insertion cost.

**Known limitations.** V4 does **not** address three aspects that an
event-driven tracer can:

1. *Sub-second events.* Transient spikes shorter than `--interval` are
   smoothed into the surrounding window. V5 (bpftrace) and V6
   (eBPF/CO-RE) cover this case via tracepoints and kprobes.
2. *Causal attribution.* resctrl and perf counters show *what* hit the
   cache, bandwidth, or softirq, but not *whose* instruction did so
   to *whose* cache line. V1's SystemTap probes can tag stack frames
   with the calling task; V4 cannot replicate that.
3. *Per-packet service time for nets.* Both nets backends are
   approximations (softirq-fraction ratio or a fixed 1us/packet
   heuristic). Both carry `status=degraded`;
   `v4-hybrid-procfs/DESIGN.md` section 3 explains the gap.

**Relationship to other variants.** V4 differs from **V1** in trading
event-driven SystemTap probes for polling over stable ABIs, losing
per-event resolution but removing kernel-module risk. V4 differs from
**V3** by not using SystemTap at all, instead reaching resctrl directly
and adding perf_event_open as a second hardware-counter path. V4
differs from **V5** by not using bpftrace: no BTF dependency and no
per-event handler, at the cost of causal attribution. V4 differs from
**V6** by not using eBPF/CO-RE: no verifier, no maps, no BPF object
loading, at the cost of kernel-internal counters that are only exposed
through tracepoints.

### V5 -- bpftrace (eBPF scripts)

bpftrace scripts replacing SystemTap. eBPF safety guarantees (verifier
prevents kernel crashes), no debuginfo needed (uses BTF). Software metrics
via bpftrace, hardware metrics via resctrl.

### V6 -- eBPF/CO-RE with libbpf (full prototype)

Full eBPF implementation using CO-RE (Compile Once Run Everywhere). The
dissertation's Phase 2 prototype for the three-way comparison: original IntP
vs refactored IntP vs eBPF prototype.

---

> V1, V2, V3, V5, V6: detailed architecture / metric method / fidelity
> analysis pending; they are owned by their respective variant
> directories and will be summarised here as each lands.
