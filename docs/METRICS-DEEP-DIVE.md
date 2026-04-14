# Metrics Deep Dive

Technical documentation of IntP's 7 interference metrics. Covers kernel probe
points, embedded C functions, normalization formulas, and hardcoded constants
for each metric.

## Overview

IntP collects 7 interference metrics from the Linux kernel:

1. **netp** -- Network physical utilization
2. **nets** -- Network stack utilization (service time)
3. **blk** -- Block I/O utilization
4. **mbw** -- Memory bandwidth utilization
5. **llcmr** -- LLC miss ratio
6. **llcocc** -- LLC occupancy
7. **cpu** -- CPU utilization

Each section below documents the kernel interface, the SystemTap probe points,
the embedded C functions, the normalization formula, and any hardcoded constants.

---

> TODO: Populate from the line-by-line analysis of intp.stp.
> Each metric section should include:
> - SystemTap probe points used
> - Embedded C function names and their kernel API calls
> - Raw value collection method
> - Normalization formula
> - Hardcoded constants and their derivation
> - Output format and units
> - Known limitations and kernel version dependencies
