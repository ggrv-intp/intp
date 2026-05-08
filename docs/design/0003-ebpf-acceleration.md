# ADR-0003 — eBPF as opt-in acceleration backend

- **Status:** Accepted
- **Date:** 2026-05-08
- **Deciders:** Lead maintainer
- **Supersedes:** —
- **Related:** [ARCHITECTURE.md](../ARCHITECTURE.md), [ADR-0001](0001-backend-registry.md), `intp-comparison/v3-ebpf-libbpf/DESIGN.md`

## Context

The dissertation comparison
([`intp-comparison/docs/VARIANT-COMPARISON.md`](https://github.com/ggrv-intp/intp-comparison/blob/main/docs/VARIANT-COMPARISON.md))
shows that the eBPF variant (v3) achieves the highest metric fidelity
at the lowest runtime overhead, but that it requires:

- Kernel ≥ 5.8 with BTF.
- A working `libbpf` ≥ 0.8 and `bpftool` at build time.
- A runtime that can pass the eBPF verifier — which depends on
  kernel internals that are not strictly part of the stable ABI
  (Zhong et al., 2025).

v2's pure-C path needs none of those things and works on kernel
4.10+. It pays a small fidelity cost on `blk` (`io_ticks` instead of
queue-depth integration) and `nets` (procfs softirq fraction instead
of per-event).

The "default Linux app" goal forbids forcing eBPF on hosts that
cannot use it — Debian 11, Ubuntu 18.04, RHEL 8, Alpine on minimal
kernels — because those are exactly the long tail of the
distribution audience.

## Decision

`intp` adopts **v2's pure-C implementation as its baseline backend
set, with v3's eBPF programs registered as opt-in higher-priority
backends per metric.**

- Build-time: `--with-ebpf` is `auto` by default. `auto` enables eBPF
  if `libbpf` and `clang` are present at configure time. `enabled`
  fails the build if they are missing. `disabled` skips eBPF entirely.
- Runtime: each metric's backend list (per ADR-0001) places the eBPF
  candidate at higher priority than the pure-C candidate. The
  capability probe checks BTF, `CAP_BPF`, and the kernel version. On
  capable hosts, eBPF wins; on incapable hosts, pure-C wins, and the
  observable schema is identical.
- Distro packaging: the base `.deb` / `.rpm` / `PKGBUILD` builds with
  `--with-ebpf=enabled`. A small minority of distros that cannot
  satisfy the eBPF build deps (e.g. very old Alpine) build with
  `--with-ebpf=disabled` — the resulting binary is still useful and
  schema-compatible.

The eBPF programs themselves live in `src/backends/ebpf/*.bpf.c` and
are compiled to portable bytecode via libbpf's CO-RE machinery,
following the layout established in v3 of the comparison repository.

## Consequences

**Positive**

- Kernel-4.10+ portability: `intp` runs on every distro release we
  care about, including legacy LTS.
- Best-available fidelity on modern kernels: closes v2's known
  fidelity gap on `blk` and `nets` without forcing eBPF on older
  hosts.
- Clean degradation: a kernel upgrade or a `libbpf` upgrade flips
  the active backend without any user intervention.
- Build-system simplicity: one binary, one configure step, two
  meaningful axes (kernel ≥ 5.8 + BTF? capability flags?). The
  matrix multiplication happens at runtime, not at packaging time.

**Negative**

- Two paths to test for every core metric. The cross-variant
  equivalence harness (ported from
  `intp-comparison/shared/validate-cross-variant.sh` to
  `tests/regression/`) keeps the two honest.
- CO-RE relocation can fail on kernels with inlined or renamed
  functions (Zhong et al., 2025). Mitigation: the eBPF probe is
  expected to detect a verifier rejection and fall through to the
  pure-C candidate cleanly. Failures are logged, not crashes.
- Distros that do not satisfy the eBPF build deps lose the fidelity
  improvements. Acceptable: their users still get the v2-grade
  output that the comparison repo already validates.

## Alternatives considered

- **Pure v2 (no eBPF at all in `intp`).** Simpler build, broader
  portability. Rejected: leaves the `blk` / `nets` fidelity gap
  permanently, on hosts that *can* close it.
- **Pure v3 (eBPF only, kernel ≥ 5.8).** Highest fidelity, simpler
  build matrix at the eBPF level. Rejected: locks out the
  ~30–40% of "default Linux" hosts on older kernels.
- **Two separate binaries (`intp` and `intp-ebpf`).** Rejected:
  doubles the packaging burden and complicates downstream
  documentation. The runtime selection is cheap and avoids the
  split.
- **Shipping eBPF as a separate plugin (`intp-plugin-ebpf`).**
  Considered seriously. Rejected for the *core* eBPF backends
  because they back metrics already in `intp`'s scope; the plugin
  story is for *new* metrics. Note that out-of-tree eBPF plugins
  (e.g. `intp-plugin-blk-iouring`) are still possible via the
  plugin ABI — this ADR is about the in-tree backends only.

## Implementation notes

- `src/backends/ebpf/` holds the eBPF programs and their loader code.
  Skeleton generation (`bpftool gen skeleton`) is a Meson custom
  target.
- `vmlinux.h` is generated at build time from the running kernel's
  BTF, with a fallback to a checked-in vmlinux BTF dump for
  environments where `/sys/kernel/btf/vmlinux` is unavailable.
- The eBPF probes per metric live alongside their pure-C
  counterparts so the failure modes are co-located in the source.
- Verifier failures should be reported with enough context to be
  debuggable. The failure path logs the rejected program name, the
  kernel version, and the relevant BTF probe; users filing
  kernel-compat issues can paste this into the
  [kernel-compat issue template](../.github/ISSUE_TEMPLATE/kernel-compat.yml).
