# `hello-world` — minimal `intp` plugin

This example is the canonical "smallest plugin that does something":
one C file, one declared metric, one increment per sample. It is
also the contract test for the plugin ABI — if `hello-world` stops
building or stops being loadable, the ABI broke.

If you are looking for the *guide* to writing plugins, read
[docs/PLUGINS.md](../../docs/PLUGINS.md). This README documents only
this specific example.

---

## Build

The example is gated behind a Meson option:

```bash
meson configure build -Dwith_examples=true
meson compile -C build
```

This produces `build/examples/plugin-hello-world/intp-plugin-hello-world.so`.
The example is *not* installed by `meson install`; it is intended for
inspection and as a CI fixture, not as a deployable plugin.

## Use it from `intp`

```bash
./build/intp \
    --plugin-dir ./build/examples/plugin-hello-world \
    --enable-plugin hello-world \
    --format jsonl \
    --interval 1s
```

Each emitted JSON-Lines record will include an `app.hello.tick`
field whose value increases by one per sample.

```json
{"schema_version": "0.1.0", "ts": "...", "metrics": {
   "system.interference.cpu":  0.42,
   "system.interference.mbw":  0.17,
   ...,
   "app.hello.tick":           42
}}
```

## What the example covers

- Defining a `metric_provider` plugin via `INTP_PLUGIN_DEFINE`.
- Declaring a metric in a `struct intp_metric_schema` table.
- Implementing the `init` / `sample` / `shutdown` vtable.
- Using `intp_plugin_ctx_set_private` /
  `intp_plugin_ctx_get_private` for plugin-private state.
- Calling `intp_sample_append_u64` to publish a value.
- An optional `intp_plugin_self_test` so the loader can sanity-check
  the build before registering.

What it deliberately does *not* cover (because they are documented
elsewhere or are out of scope for the smallest example):

- Reading from `/proc` or `/sys` (parsing helpers will land in
  `src/core/procutil.c` in Stage 2 of the roadmap).
- Declaring real capability requirements (`needs_capabilities`,
  `min_kernel`) — see [docs/PLUGINS.md](../../docs/PLUGINS.md) for
  the full list.
- Writing an `output_sink` or `backend` plugin — different vtables;
  also covered in `docs/PLUGINS.md`.

## Why this is the contract test

`tests/plugin-abi/` (Step 7 of the scaffolding plan) loads this
plugin into a libintp test harness and asserts:

1. The descriptor's `intp_plugin_descriptor` symbol resolves cleanly
   under `dlsym(3)`.
2. The declared `abi_min`/`abi_max` range includes the running
   library's `INTP_PLUGIN_ABI_VERSION`.
3. The capability declarations parse cleanly.
4. `intp_plugin_self_test()` returns 0.
5. The plugin's metric appears in `intp --print-schema` output after
   `--plugin-dir` is set.

Any change that breaks one of those assertions is, by definition,
breaking the plugin ABI. CI's `abidiff` gate catches the binary side;
this example catches the source-compile side.
