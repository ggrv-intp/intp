# ADR-0004 — Plugin ABI shape and stability policy

- **Status:** Accepted
- **Date:** 2026-05-08
- **Deciders:** Lead maintainer
- **Supersedes:** —
- **Related:** [ADR-0001](0001-backend-registry.md), [ADR-0002](0002-output-schema-and-versioning.md), [ARCHITECTURE.md](../ARCHITECTURE.md), [PLUGINS.md](../PLUGINS.md)

## Context

`intp` ships a tight set of seven OS-level metrics in its core. Every
extension domain we have committed to (GPU, databases, observability
sinks, scheduler adapters, vector-DB internals, and whatever lands
later) needs to coexist with that core *without* growing it. The
mechanism we chose is a stable plugin ABI loaded from `.so` files at
runtime.

We had to answer four questions:

1. What is the contract surface — what does a plugin actually export
   and what does the loader expect?
2. Which plugin kinds are first-class?
3. How do we encode capability declarations so the loader can refuse
   to load a plugin on an incompatible host without `init()` running
   anything risky?
4. What is the stability promise, and what is the breakage path?

## Decision

### Surface

Each plugin `.so` exports one symbol — `intp_plugin_descriptor` of
type `struct intp_plugin_descriptor` — and optionally
`intp_plugin_self_test()`. The loader `dlopen`s the file, `dlsym`s
the descriptor, validates the ABI version range, runs the optional
self-test, and (if everything passes) registers the plugin.

The descriptor carries:

- **Identity**: name, vendor, SPDX license, semantic version.
- **ABI range**: `[abi_min, abi_max]` of supported
  `INTP_PLUGIN_ABI_VERSION` values.
- **Kind discriminator**: `backend` / `metric_provider` /
  `output_sink`.
- **Schema fragment**: the metrics this plugin publishes (for
  metric_provider) or backs (for backend); NULL-terminated array of
  `struct intp_metric_schema`.
- **Requirements**: `struct intp_plugin_requirements` — minimum
  kernel, BTF needs, capability flags, vendor filter, required
  filesystem paths.
- **Vtable**: a union of the three kind-specific vtables. The
  loader inspects `kind` to pick the right one.

The `INTP_PLUGIN_DEFINE` macro hides the boilerplate (visibility
attribute, descriptor name, default ABI range).

### Three plugin kinds

| Kind              | Vtable                                                  |
|-------------------|---------------------------------------------------------|
| `backend`         | `backs` (metric name) + `priority` + `probe/init/sample/shutdown` |
| `metric_provider` | `init/sample/shutdown` (publishes its own metrics)      |
| `output_sink`     | `format_name` + `open/emit/close`                       |

We considered separating "subscriber" (live consumer of the sample
stream) into a fourth kind but concluded that subscribers are not
plugins — they live on the *outside* of `libintp`, talking over the
gRPC streaming API. Plugin = code that runs inside libintp.

### Capability declarations

Every plugin declares, in its descriptor:

- `min_kernel`: a SemVer-ish string (`"5.8"`, `"6.1"`); compared
  against `uname -r`.
- `needs_btf`: bool.
- `needs_capabilities`: bitmask of `INTP_CAP_PERFMON`,
  `INTP_CAP_BPF`, `INTP_CAP_DAC_READ_SEARCH`, etc.
- `vendor_filter`: bitmask of `INTP_VENDOR_INTEL`, `_AMD`, `_ARM`,
  `_RISCV`, `_ANY`.
- `needs_paths`: NULL-terminated array of filesystem paths the
  plugin will read.

The loader checks every declaration before calling `init()`. Plugins
that declare correctly fail closed on incompatible hosts without
running risky code; the loader skips them quietly and moves on. This
is what lets a single distro package install on hosts with and
without GPU drivers, with and without resctrl, etc.

### Stability policy

- **Pre-1.0** (`INTP_PLUGIN_ABI_VERSION = 0`): the ABI is unstable.
  We may rearrange struct fields, rename callbacks, or change
  capability flag values in any minor release. The CHANGELOG
  documents each break.
- **From 1.0** (`INTP_PLUGIN_ABI_VERSION ≥ 1`): the ABI freezes
  within a major version. Source-and-binary compatibility is
  preserved across minor and patch releases. Breaking changes
  require a major-version bump and an ADR.
- **`abi_min` / `abi_max`** lets a plugin built against e.g.
  ABI v3 declare it also works under v2; the loader picks the
  newest ABI in the intersection of plugin's range and libintp's
  range.

`tests/plugin-abi/` runs `abidiff` against the previous tagged
release on every PR. A failure blocks merge unless the PR also
bumps `INTP_PLUGIN_ABI_VERSION` and adds an ADR.

## Consequences

**Positive**

- Out-of-tree plugins are first-class. A vendor (NVIDIA, AMD,
  Intel, Cloudflare, …) can ship a plugin without forking `intp`.
- Distro packagers can split optional functionality (`intp-plugin-
  gpu-nvml`, `intp-plugin-db-postgres`) into separate packages with
  their own dependency closure, without touching the base.
- Capability declarations let plugins fail closed on incompatible
  hosts without crashing; one DaemonSet image can serve a
  heterogeneous fleet.
- A small, explicit ABI is easier to audit than an ad-hoc plugin
  protocol.

**Negative**

- Versioning discipline becomes load-bearing. We must run `abidiff`
  in CI and ensure every contributor knows that touching `plugin.h`
  is special.
- The ABI is C, which restricts the languages plugins can be
  written in directly. Mitigation: the
  [bindings/](../../bindings/) directory provides Python / Go /
  Rust wrappers post-1.0 for plugin authors who prefer those
  languages.
- Registration via `dlsym(intp_plugin_descriptor)` precludes
  multiple plugins per `.so`. We considered an ELF-section-based
  registry but the symbol-based path is simpler and sufficient.

## Alternatives considered

- **No plugin ABI; only in-tree extensions.** Rejected: makes the
  base package grow with every domain (GPU, every DB, every
  scheduler) — the exact failure mode we want to avoid.
- **WASM-based plugins.** Tempting for safety, but the WASM
  toolchain plus runtime adds heavy build deps that distros do not
  package uniformly, and the eBPF subsystem (which we already
  depend on) is the existing answer to "safely run user code in
  a privileged context".
- **Lua / Python embedded interpreter for plugins.** Rejected for
  performance, footprint, and dependency-tree reasons.
- **GLib's `GModule` or libtool's `lt_dlopen`.** Rejected: extra
  deps for no measurable benefit over plain `dlopen(3)`.

## Implementation notes

- Plugin `.so`s build with `-fvisibility=hidden` plus an explicit
  `__attribute__((visibility("default")))` on the descriptor symbol
  (provided by `INTP_PLUGIN_VISIBLE`).
- The loader scans `/usr/lib/intp/plugins/`,
  `/usr/local/lib/intp/plugins/`, and `$XDG_DATA_HOME/intp/plugins/`
  in that order; later entries override earlier ones with the same
  plugin name. CLI flags (`--plugin-dir`, `--enable-plugin`,
  `--disable-plugin`) override the discovery rules.
- A plugin descriptor whose `kind` is unknown is rejected with a
  warning — this is the forward-compatibility hook for future
  kinds (we may eventually add `transform` plugins for sample
  preprocessing, but that is out of v1.0 scope).
