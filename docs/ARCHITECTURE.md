# Architecture

This document describes how `intp` is organised internally and the
contracts that make it extensible without bloating the distro-packaged
core. It is the canonical reference for new contributors. The *why*
behind individual decisions lives in the ADRs under
[`design/`](design/).

---

## High-level shape

```
                ┌──────────────────────────────────────────┐
                │                  intp(1)                  │ ← thin CLI wrapper
                └───────────────────────┬──────────────────┘
                                        │
                ┌───────────────────────▼──────────────────┐
                │                 libintp                  │ ← public C API
                │ ┌────────────────────────────────────┐   │   (intp.h, plugin.h,
                │ │              core                  │   │    schema.h)
                │ │  registry • dispatcher • loader    │   │
                │ └────┬──────────────┬────────────┬───┘   │
                │      │              │            │       │
                │  ┌───▼────┐   ┌─────▼──────┐ ┌──▼───┐   │
                │  │backends│   │  plugins   │ │output│   │
                │  │(in-tree│   │  (in-tree  │ │sinks │   │
                │  │ for 7  │   │   ref +    │ │      │   │
                │  │ core   │   │  out-of-   │ │      │   │
                │  │metrics)│   │  tree .so) │ │      │   │
                │  └────────┘   └────────────┘ └──────┘   │
                └──────────────────────────────────────────┘
                            │                      │
                            ▼                      ▼
                ┌─────────────────────┐   ┌─────────────────┐
                │ kernel & hardware   │   │ TSV / JSONL /   │
                │ (procfs, sysfs,     │   │ Prometheus /    │
                │  perf, resctrl,     │   │ OTLP / Unix /   │
                │  netlink, eBPF)     │   │ gRPC streams    │
                └─────────────────────┘   └─────────────────┘
```

Three layers, three boundaries:

1. **CLI / library boundary** — `intp(1)` is a thin wrapper; all real
   work happens in `libintp`. Anything `intp` can do, a linked program
   can also do via the public C API.
2. **Core / extension boundary** — the seven core OS-level metrics
   live in `src/backends/`. New metrics, new data sources, new
   application-level signals live in `src/plugins/` (in-tree) or in
   out-of-tree `.so` files. Crossing this boundary requires an ADR.
3. **Collection / output boundary** — sample collection produces a
   structured stream (`struct intp_sample` with metric values keyed
   by schema descriptor); output sinks consume that stream and
   serialise it. The stream is the *only* coupling between the two
   sides; adding a new sink does not touch a single backend.

---

## The three plugin kinds

`include/intp/plugin.h` defines a stable, versioned ABI for three
distinct extension points. The kind is recorded in the plugin
descriptor; the loader uses it to wire the plugin into the right
registry.

| Kind              | Purpose                                                                   | Examples                                                |
|-------------------|---------------------------------------------------------------------------|---------------------------------------------------------|
| `backend`         | Alternate collection mechanism for an *existing* metric.                  | eBPF backend for `blk` on kernel ≥5.8.                  |
| `metric_provider` | Adds *new* metrics with their own schema fragment.                        | `gpu-nvml`, `db-postgres`, `db-vector`, `bpftrace`.     |
| `output_sink`     | Consumes the merged sample stream and emits it in a particular format.    | TSV, JSON-Lines, Prometheus, OTLP. Out-of-tree welcome. |

The three kinds share one ABI struct (`struct intp_plugin_descriptor`)
with a `kind` discriminator and three small vtables, one per kind.
This keeps the loader simple and the contract small.

See [docs/design/0004-plugin-abi.md](design/0004-plugin-abi.md) (lands
with Step 5) for the full vtable definitions.

---

## Backend registry & runtime selection

The seven core metrics each carry an *ordered list of backends*. At
startup, the registry walks the list and picks the first backend
that:

1. Builds successfully (some backends are compiled out by Meson
   options — e.g. `--without-ebpf`).
2. Probes successfully on the running system (capability check:
   sysfs path exists, `perf_event_open` accepted, BTF available, …).
3. Is not explicitly disabled via `--disable-backend=name` or the
   environment.

A metric whose entire backend list fails to probe is *reported* as
unavailable in `intp --print-schema` rather than silently zeroed —
this avoids the "metric quietly returns 0 forever" trap that bit
several intp-comparison variants.

This is the same pattern v2 introduced in `intp-comparison/v2-c-stable-abi/`
(see `DESIGN.md` there); the new contribution in `intp` is opening
the list to plugins, so an out-of-tree eBPF backend or a vendor's
ROCm SMI backend can register without rebuilding the binary.

See [docs/design/0001-backend-registry.md](design/0001-backend-registry.md).

---

## Capability model

Every backend / plugin declares its required capabilities in the
descriptor: file paths it reads, sysfs nodes it probes,
`perf_event_open(2)` types it needs, kernel features (`CAP_PERFMON`,
`CAP_BPF`, `CAP_DAC_READ_SEARCH`), minimum kernel version, vendor
constraints (Intel-RDT-only, AMD-uncore-only).

The loader cross-references those declarations against:

- The running process's actual capabilities (`getcap` / `prctl`).
- The kernel's reported features (`/proc/kallsyms`,
  `/sys/kernel/btf/`, `/sys/fs/resctrl/info/`).
- The detected hardware (vendor, model, microarchitecture; see
  `tools/intp-detect.sh`).

Missing capability ⇒ the backend is skipped (not loaded) and the next
candidate in the metric's backend list is tried. The CLI exposes the
result via `intp --print-schema`, which lists, for each metric, which
backend was selected and why competing candidates were rejected.

This is how `intp` runs the same binary on Debian 12 (kernel 6.1, no
BTF) and Debian 13 (kernel 6.8 with BTF) and gets different but
sensible results on each.

---

## Output schema & versioning

The output is **self-describing**. Every text format carries a
`# schema:` header (TSV: leading comment line; JSONL: one JSON object
per line including a `schema_version` field; Prometheus: comments at
the top of `/metrics`; OTLP: in resource attributes).

`intp --print-schema` dumps the active schema as JSON Schema. Code
generators in downstream consumers (IADA, custom schedulers, exporter
glue) are expected to consume that.

The seven core metrics live under the namespace `system.interference.*`
and are stable from `intp 1.0`. Plugin-defined metrics live under
`<kind>.<vendor-or-name>.*` (e.g. `gpu.nvidia.*`, `db.postgres.*`,
`app.<service>.*`).

A *legacy* mode (`--format tsv-legacy-v1`) emits exactly the
seven-column TSV that all `intp-comparison` consumers already parse,
bit-for-bit. This is the bridge that lets IADA, the dissertation
plotting code, and the Meyer-CSV converter keep working unchanged.

See [docs/design/0002-output-schema-and-versioning.md](design/0002-output-schema-and-versioning.md).

---

## Streaming subscriber API

For consumers that need *live* data without polling files (Kubernetes
scheduler plugin, IADA in online mode, SLURM SPANK plugin), `libintp`
exposes a `subscribe()` API plus two transport implementations:

- **Unix-domain socket** (`/run/intp/sample.sock`) — local, low-overhead.
  Default authentication: SO_PEERCRED + a plugin-declared allowlist.
- **gRPC** (`intp serve --grpc :9100`) — for cross-host scheduler
  integration. Authentication via mTLS or static bearer token. Schema
  served over reflection + a generated `intp.proto`.

Both transports speak the same `intp_sample` proto/struct; the only
difference is who can connect. See
[docs/design/0005-streaming-subscriber-api.md](design/0005-streaming-subscriber-api.md).

---

## Build-time vs runtime decisions

Some decisions belong at build time (which backends to even compile),
others at runtime (which compiled backend wins on this host).

| Build-time (Meson option)              | Runtime (capability probe)                    |
|----------------------------------------|------------------------------------------------|
| `--with-ebpf=auto/enabled/disabled`    | BTF availability, `CAP_BPF`, kernel ≥ 5.8     |
| `--with-bpftrace`                      | `bpftrace` binary on `PATH`                   |
| `--with-gpu-nvml`                      | NVIDIA driver loaded, NVML library present    |
| `--with-gpu-rocm`                      | ROCm runtime present                          |
| `--with-gpu-zes`                       | Level Zero runtime present                    |
| `--with-otel`                          | (none — pure formatter)                       |
| `--with-prometheus`                    | (none — pure formatter)                       |

The principle: if a feature can be turned on without growing the
distro-packaged binary's `Depends:` list, prefer build-time inclusion.
If it pulls in a heavy dependency (CUDA, ROCm, `bpftrace`), put it
behind a Meson flag and ship as a separate `intp-plugin-*` package.

---

## Threat model (summary)

`intp` is a *measurement* tool, not a security boundary. It assumes
the operator running it has the capabilities they declared. The
plugin-descriptor mechanism is a *clarity* tool, not a *containment*
tool. The full discussion is in [SECURITY.md](../SECURITY.md).

What we *do* harden:

- Input parsing of `/proc` and `/sys` text (no fixed-buffer
  assumptions).
- Bounds checking on all eBPF ring-buffer reads.
- Refusing to load plugins whose `INTP_PLUGIN_ABI_VERSION` does not
  match.
- Refusing to load plugins whose declared capabilities exceed the
  running process's actual capabilities.
- Failing closed when a backend's capability probe is ambiguous,
  rather than guessing.

---

## Where things go in the source tree

A condensed map (the full layout is in the project plan):

| Directory                 | Holds                                                            |
|---------------------------|------------------------------------------------------------------|
| `include/intp/`           | Public C API and plugin ABI headers.                            |
| `src/core/`               | Registry, dispatcher, plugin loader, capability probe.          |
| `src/backends/<kind>/`    | In-tree backends for the 7 core metrics.                         |
| `src/plugins/<name>/`     | In-tree reference plugins, each builds as its own `.so`.        |
| `src/output/`             | In-tree output sinks (TSV, JSONL, Prometheus, OTLP).            |
| `src/ipc/`                | Streaming transports (Unix socket, gRPC).                       |
| `src/lib/`                | `libintp.c` — public API entry points.                          |
| `src/cli/`                | `main.c` — argument parsing, output sink wiring.                |
| `tests/`                  | Unit, integration, plugin-ABI, regression tests.                |
| `bench/`                  | Workload-pair benchmarks ported from `intp-comparison`.         |
| `integrations/`           | Adapters (Prometheus exporter, k8s plugin, IADA glue, …).        |
| `bindings/`               | Python / Go / Rust bindings over `libintp`.                     |
| `packaging/`              | Distro packaging (Debian, RPM, Arch, …).                        |

---

## Heritage from `intp-comparison`

Where this design comes from, in one paragraph: v2's pure-C backend
registry is the spine; v3's eBPF programs slot in as just another
backend per metric on capable kernels; v3.1's bpftrace becomes a
research/experimental plugin rather than a competing implementation;
v0/v0.1/v1/v1.1's SystemTap variants stay in the comparison repo as
historical references. The hybrid recovers v0's metric coverage
without v0's kernel-module footprint.

The dissertation chapter on variant comparison
(`intp-comparison/docs/VARIANT-COMPARISON.md`) is the long-form
justification.
