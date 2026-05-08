/* SPDX-License-Identifier: MIT */
/*
 * libintp.c — stage-1.5 scaffold implementation.
 *
 * Real backend dispatch, plugin loading, and sample collection land
 * in Stage 2 of the roadmap. This file currently provides:
 *
 *   - intp_version_string() — reads INTP_VERSION from version.h.
 *   - intp_unit_name() — returns the canonical unit name.
 *   - intp_strerror() — translates intp error codes.
 *   - The seven core metric name constants (intp_core_metric_names).
 *   - intp_open/close/sample/subscribe/print_schema — return -ENOSYS,
 *     i.e. "no implementation yet". The CLI handles --version,
 *     --help, and --print-schema (with a static skeleton) directly,
 *     so the binary builds and runs even before Stage 2.
 *
 * This is deliberately small: the goal of Stage 1.5 is to lock in the
 * public surface (linkage, header layout, version reporting) so that
 * downstream tooling — pkg-config, distro packagers, the example
 * plugin in Step 5 — can compile against libintp before the
 * implementation backfills it.
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <intp/intp.h>
#include <intp/metrics.h>
#include <intp/plugin.h>
#include <intp/schema.h>

/* INTP_VERSION is already a quoted string literal (Meson injects it
 * via configuration_data.set_quoted), so we return it as-is. */
__attribute__((visibility("default")))
const char *intp_version_string(void)
{
	return INTP_VERSION;
}

__attribute__((visibility("default")))
const char *const intp_core_metric_names[INTP_CORE_METRICS_COUNT] = {
	INTP_METRIC_NETP,
	INTP_METRIC_NETS,
	INTP_METRIC_BLK,
	INTP_METRIC_MBW,
	INTP_METRIC_LLCMR,
	INTP_METRIC_LLCOCC,
	INTP_METRIC_CPU,
};

__attribute__((visibility("default")))
const char *intp_unit_name(enum intp_unit u)
{
	switch (u) {
	case INTP_UNIT_UNSPECIFIED:		return "unspecified";
	case INTP_UNIT_RATIO:			return "ratio";
	case INTP_UNIT_PERCENT:			return "percent";
	case INTP_UNIT_BYTES:			return "bytes";
	case INTP_UNIT_BYTES_PER_SECOND:	return "bytes_per_second";
	case INTP_UNIT_OPS_PER_SECOND:		return "ops_per_second";
	case INTP_UNIT_NANOSECONDS:		return "ns";
	case INTP_UNIT_MICROSECONDS:		return "us";
	case INTP_UNIT_MILLISECONDS:		return "ms";
	case INTP_UNIT_SECONDS:			return "s";
	case INTP_UNIT_COUNT:			return "count";
	}
	return "unspecified";
}

__attribute__((visibility("default")))
const struct intp_metric_value *
intp_sample_find(const struct intp_sample *sample, const char *name)
{
	if (!sample || !name)
		return NULL;
	for (size_t i = 0; i < sample->values_len; i++) {
		if (sample->values[i].name &&
		    strcmp(sample->values[i].name, name) == 0)
			return &sample->values[i];
	}
	return NULL;
}

__attribute__((visibility("default")))
const char *intp_strerror(int err)
{
	if (err >= 0)
		return "no error";
	return strerror(-err);
}

/* ------------------------------------------------------------------------ */
/* Stub implementations — Stage 2 will fill these in.                        */
/* ------------------------------------------------------------------------ */

__attribute__((visibility("default")))
int intp_open(const struct intp_options *opts, struct intp_ctx **out)
{
	(void)opts;
	(void)out;
	return -ENOSYS;
}

__attribute__((visibility("default")))
void intp_close(struct intp_ctx *ctx)
{
	(void)ctx;
}

__attribute__((visibility("default")))
int intp_sample(struct intp_ctx *ctx, struct intp_sample *out)
{
	(void)ctx;
	(void)out;
	return -ENOSYS;
}

__attribute__((visibility("default")))
int intp_subscribe(struct intp_ctx *ctx,
		   intp_sample_callback cb, void *user)
{
	(void)ctx;
	(void)cb;
	(void)user;
	return -ENOSYS;
}

__attribute__((visibility("default")))
int intp_print_schema(struct intp_ctx *ctx, int fd)
{
	(void)ctx;
	(void)fd;
	return -ENOSYS;
}

/* Plugin sample-buffer helpers — the real implementations live with
 * the registry in Stage 2; for now these just return -ENOSYS so the
 * symbols exist for plugins to link against (plugins do not link
 * against libintp directly, but the unit tests in tests/plugin-abi/
 * do, and they need the symbols to resolve). */
__attribute__((visibility("default")))
int intp_sample_append_double(struct intp_sample_buffer *buf,
			      const char *name, enum intp_unit unit,
			      double value)
{
	(void)buf;
	(void)name;
	(void)unit;
	(void)value;
	return -ENOSYS;
}

__attribute__((visibility("default")))
int intp_sample_append_i64(struct intp_sample_buffer *buf,
			   const char *name, enum intp_unit unit,
			   int64_t value)
{
	(void)buf;
	(void)name;
	(void)unit;
	(void)value;
	return -ENOSYS;
}

__attribute__((visibility("default")))
int intp_sample_append_u64(struct intp_sample_buffer *buf,
			   const char *name, enum intp_unit unit,
			   uint64_t value)
{
	(void)buf;
	(void)name;
	(void)unit;
	(void)value;
	return -ENOSYS;
}

__attribute__((visibility("default")))
int intp_sample_append_unavailable(struct intp_sample_buffer *buf,
				   const char *name)
{
	(void)buf;
	(void)name;
	return -ENOSYS;
}

__attribute__((visibility("default")))
void *intp_plugin_ctx_get_private(struct intp_plugin_ctx *ctx)
{
	(void)ctx;
	return NULL;
}

__attribute__((visibility("default")))
void intp_plugin_ctx_set_private(struct intp_plugin_ctx *ctx, void *priv)
{
	(void)ctx;
	(void)priv;
}

/* Schema introspection — minimal placeholders until the real
 * registry lands in Stage 2. */
__attribute__((visibility("default")))
size_t intp_schema_size(const struct intp_ctx *ctx)
{
	(void)ctx;
	return 0;
}

__attribute__((visibility("default")))
const struct intp_metric_schema *
intp_schema_get(const struct intp_ctx *ctx, size_t index)
{
	(void)ctx;
	(void)index;
	return NULL;
}

__attribute__((visibility("default")))
const struct intp_metric_schema *
intp_schema_find(const struct intp_ctx *ctx, const char *name)
{
	(void)ctx;
	(void)name;
	return NULL;
}
