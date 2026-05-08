/* SPDX-License-Identifier: MIT */
/*
 * hello-world — a minimal `intp` plugin.
 *
 * Demonstrates the smallest viable metric_provider: declares one
 * metric (`app.hello.tick`), increments it once per sample. Used as
 * the contract test for the plugin ABI; if this stops loading or
 * publishing its metric, the ABI broke.
 *
 * Build via Meson: `meson configure build -Dwith_examples=true`,
 * then `meson compile -C build`. The resulting
 * `intp-plugin-hello-world.so` is loadable by `intp --plugin-dir
 * ${BUILDDIR}/examples/plugin-hello-world ...`.
 */

#include <errno.h>
#include <stdlib.h>

#include <intp/plugin.h>

struct hello_state {
	uint64_t tick;
};

static int hello_init(struct intp_plugin_ctx *ctx)
{
	struct hello_state *s = calloc(1, sizeof(*s));
	if (!s)
		return -ENOMEM;
	intp_plugin_ctx_set_private(ctx, s);
	return 0;
}

static int hello_sample(struct intp_plugin_ctx *ctx,
			struct intp_sample_buffer *out)
{
	struct hello_state *s = intp_plugin_ctx_get_private(ctx);
	return intp_sample_append_u64(out, "app.hello.tick",
				      INTP_UNIT_COUNT, s->tick++);
}

static void hello_shutdown(struct intp_plugin_ctx *ctx)
{
	free(intp_plugin_ctx_get_private(ctx));
	intp_plugin_ctx_set_private(ctx, NULL);
}

static const struct intp_metric_schema hello_metrics[] = {
	{
		.name           = "app.hello.tick",
		.unit           = INTP_UNIT_COUNT,
		.description    = "Monotonic tick counter (hello-world example).",
		.always_present = true,
	},
	{ 0 },
};

INTP_PLUGIN_DEFINE(
	.name    = "hello-world",
	.vendor  = "intp examples",
	.license = "MIT",
	.version = "0.1.0",
	.kind    = INTP_PLUGIN_KIND_METRIC_PROVIDER,
	.metrics = hello_metrics,
	.requires = {
		.vendor_filter = INTP_VENDOR_ANY,
	},
	.vt.metric_provider = {
		.init     = hello_init,
		.sample   = hello_sample,
		.shutdown = hello_shutdown,
	},
);

/*
 * Optional self-test the loader runs before registering. We simply
 * verify the schema table is well-formed; a real plugin would also
 * check that its data sources exist and are readable.
 */
INTP_PLUGIN_VISIBLE
int intp_plugin_self_test(void)
{
	return hello_metrics[0].name != NULL ? 0 : -1;
}

