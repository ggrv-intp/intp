/* SPDX-License-Identifier: MIT */
/*
 * bpftrace_plugin.c — packaging-validation stub for intp-plugin-bpftrace.
 *
 * Stage 1.5 scaffold: this file exists to make the
 * intp-plugin-bpftrace Debian package buildable end-to-end. The
 * real implementation — loading user-supplied .bt scripts via
 * bpftrace(8), parsing their JSON output, and republishing the
 * results as plugin-defined metrics — lands in Stage 2/post-1.0.
 *
 * Until then, the plugin declares itself but its sample() callback
 * returns -ENOSYS. The loader's capability check should keep it from
 * being selected; the descriptor is exercised by the abidiff and
 * lintian CI gates.
 */

#include <errno.h>

#include <intp/plugin.h>

static int bpftrace_init(struct intp_plugin_ctx *ctx)
{
	(void)ctx;
	return 0;
}

static int bpftrace_sample(struct intp_plugin_ctx *ctx,
			   struct intp_sample_buffer *out)
{
	(void)ctx;
	(void)out;
	return -ENOSYS;	/* Stage 2 fills this in. */
}

static void bpftrace_shutdown(struct intp_plugin_ctx *ctx)
{
	(void)ctx;
}

INTP_PLUGIN_DEFINE(
	.name    = "bpftrace",
	.vendor  = "intp",
	.license = "MIT",
	.version = "0.1.0-stub",
	.kind    = INTP_PLUGIN_KIND_METRIC_PROVIDER,
	.metrics = NULL,	/* metrics are user-defined per .bt script */
	.requires = {
		.vendor_filter = INTP_VENDOR_ANY,
		.needs_capabilities = INTP_CAP_BPF,
	},
	.vt.metric_provider = {
		.init     = bpftrace_init,
		.sample   = bpftrace_sample,
		.shutdown = bpftrace_shutdown,
	},
);

INTP_PLUGIN_VISIBLE
int intp_plugin_self_test(void)
{
	/* Always fail self-test in the stub so the loader skips it
	 * cleanly until the real implementation lands. */
	return -ENOSYS;
}
