# Portability Roadmap

Analysis of IntP portability across kernel versions, distributions, operating
systems, and hardware architectures.

## Dimensions of Portability

1. **Kernel version**: Which kernel versions does each variant support?
2. **Distribution**: Package availability (SystemTap, bpftrace, libbpf)
3. **Architecture**: x86_64, ARM64/aarch64, RISC-V
4. **Hardware features**: Intel RDT, AMD PQoS, ARM MPAM
5. **Deployment complexity**: Dependencies, build requirements, privileges

## Portability Matrix

Each variant has different portability characteristics. The key tradeoff is
between measurement fidelity (per-event tracing vs polling) and deployment
simplicity (no kernel modules, no debuginfo).

---

> TODO: Populate from the portability research.
> Each section should cover:
> - Supported kernel version range with specific breakpoints
> - Distribution package requirements
> - Hardware feature requirements per metric
> - Deployment steps and privilege requirements
> - Known issues and workarounds per platform
