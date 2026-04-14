# Hardware Compatibility

Intel RDT, AMD PQoS, and ARM MPAM feature tables across processor generations.

## Overview

IntP's hardware-dependent metrics (mbw, llcocc) require platform-specific
monitoring features. This document maps which processor families support
which features.

## Feature Requirements by Metric

| Metric | Hardware Feature Required       | Intel          | AMD            | ARM           |
|--------|---------------------------------|----------------|----------------|---------------|
| netp   | NIC (any)                       | Yes            | Yes            | Yes           |
| nets   | None (kernel probes)            | Yes            | Yes            | Yes           |
| blk    | None (kernel probes)            | Yes            | Yes            | Yes           |
| mbw    | Memory bandwidth monitoring     | RDT MBM        | PQoS MBM      | MPAM          |
| llcmr  | HW cache perf counters          | Yes            | Yes            | Partial       |
| llcocc | LLC occupancy monitoring        | RDT CMT        | PQoS L3 Mon   | MPAM          |
| cpu    | None (kernel probes)            | Yes            | Yes            | Yes           |

---

> TODO: Populate from the hardware research.
> Each section should include:
> - Processor generation table (e.g., Haswell, Broadwell, ... Sapphire Rapids)
> - Which RDT/PQoS/MPAM sub-features are supported
> - How to detect support at runtime
> - Known errata or limitations
> - resctrl filesystem availability by kernel version
