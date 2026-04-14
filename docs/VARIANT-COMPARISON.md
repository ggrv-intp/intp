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

Pure C program with no kernel module. Uses stable kernel interfaces
(procfs, sysfs, perf_event_open, resctrl). Trading per-event detail for
deployment simplicity and zero crash risk.

### V5 -- bpftrace (eBPF scripts)

bpftrace scripts replacing SystemTap. eBPF safety guarantees (verifier
prevents kernel crashes), no debuginfo needed (uses BTF). Software metrics
via bpftrace, hardware metrics via resctrl.

### V6 -- eBPF/CO-RE with libbpf (full prototype)

Full eBPF implementation using CO-RE (Compile Once Run Everywhere). The
dissertation's Phase 2 prototype for the three-way comparison: original IntP
vs refactored IntP vs eBPF prototype.

---

> TODO: Populate from the variant analysis.
> Each section should include:
>
> - Detailed architecture description
> - Metric collection method for each of the 7 metrics
> - Measurement fidelity analysis (event-driven vs polling)
> - Deployment requirements and complexity
> - Performance overhead analysis
> - Known limitations
