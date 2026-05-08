# ADR-0002 — Output schema and versioning

- **Status:** Accepted
- **Date:** 2026-05-08
- **Deciders:** Lead maintainer
- **Supersedes:** —
- **Related:** [ARCHITECTURE.md](../ARCHITECTURE.md), [ADR-0004](0004-plugin-abi.md), [ADR-0007](0007-extension-domains-gpu-db-app.md)

## Context

`intp-comparison/`'s seven variants all emit the same 7-column TSV.
That format is the existing integration contract with IADA, the
dissertation plotting code, and the Meyer-CSV converter. Breaking it
breaks every downstream consumer.

But `intp` opens the metric set to plugins. A 7-column TSV cannot
self-describe — adding a new column silently shifts every other
column. Downstream parsers that hardcoded "column 4 is `mbw`" break
with no warning.

We need an output strategy that:

1. Preserves bit-for-bit compatibility with existing IntP consumers.
2. Lets plugins add metrics without breaking those consumers.
3. Makes the schema discoverable from the data itself (so consumers
   don't have to read `intp --version` to know what the columns mean).
4. Maps cleanly onto existing observability stacks (Prometheus, OTel)
   without per-format gymnastics.

## Decision

`intp` outputs are **self-describing** and **schema-versioned**. The
specifics:

### Primary format: JSON-Lines (`--format jsonl`)

One JSON object per sample. Required fields:

```json
{
  "schema_version": "1.0.0",
  "ts": "2026-05-08T12:34:56.789Z",
  "host": "...",
  "target": {"pid": 1234, "comm": "myapp"},
  "metrics": {
    "system.interference.netp": 0.42,
    "system.interference.nets": 0.17,
    "system.interference.blk":  0.05,
    "system.interference.mbw":  0.78,
    "system.interference.llcmr": 0.22,
    "system.interference.llcocc": 0.91,
    "system.interference.cpu":  0.66
  }
}
```

Plugin-defined metrics appear as additional keys under their own
namespace (`gpu.nvidia.*`, `db.postgres.*`, `app.<service>.*`).
Consumers ignore unknown keys safely; missing keys mean the metric
was unavailable, not zero.

This is the **primary** machine format from v1.0 onward. New
integrations should target it.

### Legacy format: 7-column TSV (`--format tsv-legacy-v1`)

Bit-for-bit compatible with `intp-comparison`. Tab-separated, integer
percentages 0–99, no header line. This is what IADA, the dissertation
scripts, and the Meyer-CSV converter consume.

We commit to keeping `tsv-legacy-v1` available for the lifetime of
the `1.x` major version. A new `--format tsv-v2` (with header,
machine-parseable schema comment) is reserved for post-1.0; we will
not break `tsv-legacy-v1` to introduce it.

### Prometheus exposition (`--format prometheus`)

Standard Prometheus text format. The seven core metrics live under
`intp_<metric>_ratio` (gauges); plugin metrics under
`intp_<plugin>_<metric>`. A `intp_info` gauge carries build/host
metadata as labels.

### OpenTelemetry OTLP (`--format otlp`)

Maps to OTel semantic conventions. Core metrics under
`system.interference.*`; plugin metrics under
`system.interference.<plugin>.*`. Resource attributes carry host
identity. Both gRPC and HTTP transports supported.

### Schema discovery: `intp --print-schema`

Dumps the active schema as JSON Schema, suitable for downstream code
generation. Includes the resolved backend selection (which mechanism
was chosen for each core metric), the loaded plugins, and the metric
catalogue.

### Versioning policy

- `schema_version` follows semver. **Major** bumps mean removed or
  renamed metrics, or changed unit semantics — a breaking change for
  consumers. **Minor** means *added* metrics or output-formatting
  refinements. **Patch** is bugfix.
- The seven core metrics' `system.interference.*` namespace is frozen
  from `1.0.0`. We can extend with new plugin-namespaced metrics
  without bumping the major.
- When the schema changes, the CHANGELOG documents what changed and
  which `schema_version` was bumped to.

## Consequences

**Positive**

- Existing consumers keep working unchanged via `tsv-legacy-v1`.
- Plugins can add metrics without coordinating with downstream
  parsers.
- Prometheus and OTel users get first-class output, not a
  best-effort wrapper.
- `--print-schema` makes the contract auditable and lets downstream
  consumers code-generate against it.

**Negative**

- More formats to maintain. Each is a small separate file under
  `src/output/`; we judge the cost acceptable.
- JSONL is heavier on the wire than TSV. For high-frequency
  sampling, prefer Prometheus (pull) or OTLP-batch (push) over JSONL
  streaming.
- Schema versioning discipline is now an upstream maintenance
  burden. ADR-0004 formalises the corresponding plugin-ABI
  versioning so the two move in lock-step.

## Alternatives considered

- **Stay with the 7-column TSV.** Rejected: incompatible with
  plugin-defined metrics. Adding columns without a schema breaks
  existing parsers silently.
- **CSV with header.** Rejected: still ambiguous on units, types,
  and unavailable values. JSONL solves all three.
- **Protobuf as the primary format.** Rejected: heavier toolchain
  burden for a mostly-textual workflow. Protobuf is available via
  the OTLP and gRPC streaming paths where it fits naturally.
- **Make consumers read `intp --version` to interpret columns.**
  Rejected: that is exactly the brittle pattern we want to avoid.

## Implementation notes

- The `# schema:` comment header in TSV (`tsv-v2`, post-1.0) and
  Prometheus output is the same one-line schema descriptor in all
  three text formats. Helpers in `src/output/schema_header.c`
  produce it consistently.
- Schema validation tests live in `tests/integration/schema/`. Every
  output format is round-tripped through a validator on every PR.
- The IADA / Meyer-CSV converter (`tools/convert-meyer.py`) consumes
  `tsv-legacy-v1` exclusively.
