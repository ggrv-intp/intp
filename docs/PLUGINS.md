# Writing plugins for `intp`

`intp` ships with a tight set of seven OS-level metrics in its core.
Everything else — GPU counters, database internals, application-level
signals, exotic output formats — lives in plugins. This document is
the contract you need to satisfy to write one.

If you are adding a *new metric* to `intp`, you are almost certainly
writing a plugin. Adding a metric to core requires an ADR and very
strong justification (see [GOVERNANCE.md](../GOVERNANCE.md) and
[CONTRIBUTING.md](../CONTRIBUTING.md)).

---

## The three plugin kinds

| Kind              | When to use it                                                                |
|-------------------|-------------------------------------------------------------------------------|
| `backend`         | You have an alternate way to collect an *existing* core metric on some hosts. |
| `metric_provider` | You are adding *new* metrics — GPU, DB, vector, app, custom.                  |
| `output_sink`     | You are adding an output format we don't ship in tree.                        |

Most contributors write `metric_provider` plugins. The other two are
rarer.

---

## What a plugin is, mechanically

A plugin is a `.so` (shared object) that:

1. Declares the ABI version it was built against
   (`INTP_PLUGIN_ABI_VERSION`).
2. Exports a single symbol — `intp_plugin_descriptor` — that returns
   a `struct intp_plugin_descriptor *` describing the plugin.
3. Implements the small vtable for its kind (init, sample / format,
   shutdown).
4. Optionally exports `intp_plugin_self_test()` so the loader can
   sanity-check the build before registering.

The descriptor includes:

- **Identity**: name, vendor, license SPDX identifier, semantic
  version.
- **Kind**: `backend` / `metric_provider` / `output_sink`.
- **Schema**: a JSON Schema fragment for the metrics it publishes
  (metric_provider only) or the metrics it claims to back (backend
  only).
- **Capabilities required**: file paths, sysfs nodes, kernel features
  (`CAP_PERFMON`, `CAP_BPF`), minimum kernel version, vendor
  constraints. The loader cross-references these against the running
  system before letting the plugin register.
- **Vtable**: function pointers the registry calls.

The full struct lives in [`include/intp/plugin.h`](../include/intp/plugin.h)
once Step 5 of the scaffolding lands.

---

## Where plugins are looked up

In order:

1. Paths passed via `--plugin-dir <path>` on the command line (most
   specific).
2. `$XDG_DATA_HOME/intp/plugins/` (defaulting to
   `~/.local/share/intp/plugins/`).
3. `/usr/local/lib/intp/plugins/` — for `make install` builds.
4. `/usr/lib/intp/plugins/` — for distro-packaged plugins.

Disable a plugin without uninstalling it: `--disable-plugin=<name>`.
Enable only specific plugins: `--enable-plugin=<name>,<name>`. The
discovered list is visible in `intp --print-schema`.

Distro packagers ship each plugin as its own `.deb` / `.rpm` /
`PKGBUILD` (e.g. `intp-plugin-gpu-nvml`, `intp-plugin-db-postgres`),
so installing them is `apt install intp-plugin-<name>`.

---

## ABI stability

- Pre-1.0 (`0.x` releases): the ABI is **unstable**. Breaking changes
  can land in any minor release; we will flag them in
  [`CHANGELOG.md`](../CHANGELOG.md) and bump
  `INTP_PLUGIN_ABI_VERSION`.
- From 1.0 onwards: the ABI freezes. We commit to source-and-binary
  compatibility within a major version. Plugins can declare a *range*
  of supported ABI versions in their descriptor; the loader picks the
  highest compatible one.

`tests/plugin-abi/` runs `abidiff` against the previous tagged release
on every PR, and a breaking change blocks the merge unless the PR also
bumps the major version and adds an ADR.

---

## Skeleton for a metric_provider plugin

The example landing in `examples/plugin-hello-world/` (Step 5 of the
scaffolding plan) is the minimal end-to-end example. Until that
exists in the tree, here is the shape:

```c
#include <intp/plugin.h>

static int hello_init(struct intp_plugin_ctx *ctx)
{
        /* Open file handles, set up state. Return 0 on success. */
        return 0;
}

static int hello_sample(struct intp_plugin_ctx *ctx,
                        struct intp_sample_buffer *out)
{
        /* Append your metric values to `out`. */
        intp_sample_append(out, "app.hello.tick", INTP_UNIT_COUNT,
                           ctx->iter++);
        return 0;
}

static void hello_shutdown(struct intp_plugin_ctx *ctx)
{
        /* Free anything you allocated in init. */
}

static const struct intp_metric_schema hello_metrics[] = {
        {
                .name = "app.hello.tick",
                .unit = INTP_UNIT_COUNT,
                .description = "Monotonic tick counter (demo).",
        },
        { 0 }
};

INTP_PLUGIN_DEFINE(
        .name      = "hello-world",
        .vendor    = "intp examples",
        .license   = "MIT",
        .version   = "0.1.0",
        .kind      = INTP_PLUGIN_KIND_METRIC_PROVIDER,
        .abi_min   = INTP_PLUGIN_ABI_VERSION,
        .abi_max   = INTP_PLUGIN_ABI_VERSION,
        .metrics   = hello_metrics,
        .vt.metric_provider = {
                .init     = hello_init,
                .sample   = hello_sample,
                .shutdown = hello_shutdown,
        },
);
```

Build it as a `.so` against `pkg-config --cflags --libs intp` and
drop it in one of the plugin directories above. `intp --print-schema`
shows it; `intp --format jsonl` includes its metric in the stream.

---

## Skeleton for a backend plugin

A backend declares which core metric it backs (`backs = "blk"`) plus a
priority hint and a probe function. The registry inserts it into the
metric's backend list at the priority position, and the runtime
selection logic picks it (or doesn't) based on its probe result.

```c
INTP_PLUGIN_DEFINE(
        .name = "blk-ebpf-iouring",
        ...
        .kind = INTP_PLUGIN_KIND_BACKEND,
        .vt.backend = {
                .backs    = "blk",
                .priority = INTP_PRIORITY_HIGH,
                .probe    = blk_eiou_probe,
                .init     = blk_eiou_init,
                .sample   = blk_eiou_sample,
                .shutdown = blk_eiou_shutdown,
        },
);
```

Backends do not declare schema (they back a metric whose schema is
already declared by core); they declare *capabilities* the host must
have for `probe()` to succeed.

---

## Skeleton for an output_sink plugin

Output sinks consume the merged sample stream. The vtable is one
function: `emit(stream, sample)`. The sink is selected by
`--format <name>`.

```c
INTP_PLUGIN_DEFINE(
        .name = "csv-extended",
        ...
        .kind = INTP_PLUGIN_KIND_OUTPUT_SINK,
        .vt.output = {
                .format_name = "csv-extended",
                .open        = csvx_open,
                .emit        = csvx_emit,
                .close       = csvx_close,
        },
);
```

---

## Capability declarations

Be specific. The loader cross-checks declared capabilities against
the running host *before* `init()` is called, and skips your plugin
quietly if the host can't meet them. This is how `intp` runs on a
laptop without GPU drivers without crashing the GPU plugin.

Examples of well-formed declarations:

```c
.requires = {
        .min_kernel       = "5.8",
        .needs_btf        = true,
        .needs_capabilities = INTP_CAP_BPF | INTP_CAP_PERFMON,
        .needs_paths = (const char *[]) {
                "/sys/fs/resctrl/info/L3/num_closids",
                NULL
        },
        .vendor_filter = INTP_VENDOR_INTEL | INTP_VENDOR_AMD,
},
```

Avoid declaring more than you need. The bar is "init() will fail
without this", not "I might use this".

---

## Testing your plugin

- **Self-test**: implement `intp_plugin_self_test()` and exercise the
  happy path. The loader runs it before registering. A failing self-
  test is reported but does *not* block the rest of `intp` from
  starting; only your plugin is skipped.
- **Fixture-driven unit tests**: see `tests/unit/` for the harness.
  You can ship them in your plugin's repo or upstream them under
  `tests/plugin-abi/<plugin-name>/`.
- **Integration**: run `intp --plugin-dir <build-dir>
  --enable-plugin=<your-name> --format jsonl` and confirm your
  metrics appear and the schema is valid.
- **ABI**: link against the system `libintp` headers and run
  `abidiff` against the previous tag. Breaking ABI in a minor release
  is not OK after 1.0.

---

## Submitting your plugin

You do **not** have to upstream a plugin into the `intp` repository.
That is the point of the ABI: third-party plugins live wherever
their authors prefer.

If you do want yours listed in the official plugin registry, open a
PR adding a one-line entry under the *Community plugins* section
below. Requirements:

- OSI-approved license (declare it via the SPDX identifier in the
  descriptor).
- Listed source-code repository.
- Maintainer commits to addressing breaking ABI changes within one
  release cycle of the relevant `intp` major version.

---

## Community plugins

| Plugin name | Author | Repository | License | Status |
|-------------|--------|------------|---------|--------|
| _(none yet)_ |        |            |         |        |
