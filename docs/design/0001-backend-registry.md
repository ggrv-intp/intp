# ADR-0001 — Backend registry & runtime selection

- **Status:** Accepted
- **Date:** 2026-05-08
- **Deciders:** Lead maintainer
- **Supersedes:** —
- **Related:** [ARCHITECTURE.md](../ARCHITECTURE.md), [ADR-0003](0003-ebpf-acceleration.md), [ADR-0004](0004-plugin-abi.md)

## Context

The seven core metrics in `intp` can each be collected from multiple
kernel/hardware interfaces, and which one is *available* depends on
the running host: kernel version, BTF presence, hardware vendor,
process capabilities, sysfs layout. The variants in
`intp-comparison/` each picked one mechanism per metric and lived with
the resulting portability ceiling — v0–v1.x demand SystemTap; v2 falls
back to procfs everywhere; v3 demands eBPF + BTF.

`intp` ships a *single binary* expected to land in distro repos and
run across Debian 12 (kernel 6.1, no BTF), Debian 13 (kernel 6.8,
BTF, eBPF), and Alpine (musl, possibly no eBPF). It cannot pick one
mechanism per metric at compile time.

We need a dispatcher that tries each candidate at startup and uses
whichever works on this host.

## Decision

Each core metric carries an **ordered list of backends** registered
in `src/core/registry.c`. At startup, the registry walks the list per
metric and selects the first backend whose:

1. **Build inclusion** is true (the backend was compiled in; some are
   gated by Meson options).
2. **Capability probe** succeeds (a small `probe()` function that
   checks for required kernel features, sysfs paths, and process
   capabilities — *not* the same as `init()`).
3. **Operator allow-list** lets it run (no `--disable-backend=name`,
   not gated by `--prefer-backend=...`).

The selected backend is recorded in a `struct intp_backend_selection`
exposed via `intp --print-schema`, including which competing backends
were rejected and why. A metric whose entire list fails to probe is
*marked unavailable* (not silently zeroed).

The list is open to plugins: an out-of-tree `intp_plugin_descriptor`
of kind `backend` declares which metric it backs and a priority hint;
the loader inserts it into the metric's list at the right position.
This is what lets `intp-plugin-blk-iouring` (hypothetical) ship as a
separate package and override the in-tree eBPF backend on hosts where
its `probe()` succeeds.

## Consequences

**Positive**

- Single binary works across the kernel-version / hardware-vendor
  matrix that distros actually ship — including older LTS hosts
  where eBPF is not an option.
- New collection mechanisms (an eventual `iouring`-based block
  backend, a future MPAM-based ARM `llcocc` backend) land as PRs that
  add a new entry to a metric's list, not as architectural changes.
- The model extends to plugins for free — same vtable, same probe/
  init/sample/shutdown contract. ADR-0004 formalises that.
- Honest reporting: if a metric is unavailable, the user sees it in
  `--print-schema`, not as a misleading 0.

**Negative**

- Slightly higher startup cost than v2's "compile in one path" model,
  because every candidate's `probe()` runs even when later candidates
  win. Bounded: probes are cheap (path stat / `/sys` read), and they
  short-circuit on the first success.
- Moves complexity from build time to runtime, which makes some
  bugs harder to reproduce (the failing user's machine has a
  different selection than yours). Mitigation: `intp --print-schema`
  reports the full selection state.
- Demands a clear naming and priority convention to avoid surprise
  re-orderings when plugins are added. We define priorities as a
  small enum (`INTP_PRIORITY_LOWEST` … `INTP_PRIORITY_HIGHEST`) with
  documented semantics for each tier in `plugin.h`.

## Alternatives considered

- **Compile-time selection (v2's pattern).** Rejected: forces a
  per-distro build matrix and breaks the "single binary" goal.
- **Runtime detection without an ordered list — let backends
  self-rank.** Rejected: makes the precedence between two competing
  backends opaque from the binary's user's perspective. Explicit
  priority in the registry is auditable.
- **Configuration file driving selection.** Rejected for v1.0:
  unnecessary state. The CLI flags (`--disable-backend`,
  `--prefer-backend`) cover the operational use cases without a
  config file. We can add one later if usage demands it.

## Implementation notes

- Registration is via static initialisers in each backend's `.c`
  file (`INTP_BACKEND_REGISTER(...)`), wired by the linker section
  collection trick used in `intp-comparison/v2-c-stable-abi/`.
- Probes must be idempotent and have no side effects; they may run
  multiple times during a session if `intp` is reloaded.
- When eBPF backends are compiled in but verifier-rejected on the
  running kernel, the probe is responsible for catching that
  cleanly and falling through to the next candidate.
