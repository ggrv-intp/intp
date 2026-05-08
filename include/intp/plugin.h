/* SPDX-License-Identifier: MIT */
/*
 * intp/plugin.h — stable plugin ABI for libintp.
 *
 * This header defines the ABI a plugin must implement to be loadable
 * by libintp's plugin loader. Three plugin kinds are supported:
 *
 *   - INTP_PLUGIN_KIND_BACKEND          — alternate collection mechanism
 *                                         for an existing core metric.
 *   - INTP_PLUGIN_KIND_METRIC_PROVIDER  — adds new metrics with their
 *                                         own schema fragment.
 *   - INTP_PLUGIN_KIND_OUTPUT_SINK      — consumes the merged sample
 *                                         stream and emits it in some
 *                                         format.
 *
 * Each plugin .so exports one symbol (`intp_plugin_descriptor`) of
 * type struct intp_plugin_descriptor. Use the INTP_PLUGIN_DEFINE
 * macro to generate it.
 *
 * Stability: pre-1.0 (INTP_PLUGIN_ABI_VERSION = 0) the ABI is
 * unstable and may change in any minor release. From intp 1.0 the
 * ABI freezes; bumping ABI_VERSION will require a major-version
 * release.
 *
 * See docs/PLUGINS.md for the contributor-facing guide and
 * docs/design/0004-plugin-abi.md for the design rationale.
 */

#ifndef INTP_PLUGIN_H
#define INTP_PLUGIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <intp/metrics.h>
#include <intp/schema.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */
/* ABI version                                                               */
/* ------------------------------------------------------------------------ */

/**
 * INTP_PLUGIN_ABI_VERSION — the ABI version this header was built for.
 *
 * Plugins record this in their descriptor; the loader refuses to load
 * a plugin whose declared range does not include the running libintp's
 * INTP_PLUGIN_ABI_VERSION.
 *
 * Pre-1.0: this number is bumped freely. From 1.0 it is stable.
 */
#define INTP_PLUGIN_ABI_VERSION 0u

/* ------------------------------------------------------------------------ */
/* Plugin kinds                                                              */
/* ------------------------------------------------------------------------ */

enum intp_plugin_kind {
	INTP_PLUGIN_KIND_BACKEND = 1,
	INTP_PLUGIN_KIND_METRIC_PROVIDER,
	INTP_PLUGIN_KIND_OUTPUT_SINK,
};

/* ------------------------------------------------------------------------ */
/* Backend priority hints                                                    */
/* ------------------------------------------------------------------------ */

enum intp_priority {
	INTP_PRIORITY_LOWEST  = 0,
	INTP_PRIORITY_LOW     = 1,
	INTP_PRIORITY_MEDIUM  = 2,
	INTP_PRIORITY_HIGH    = 3,
	INTP_PRIORITY_HIGHEST = 4,
};

/* ------------------------------------------------------------------------ */
/* Capability declarations (placeholder — full set lands in Step 5)          */
/* ------------------------------------------------------------------------ */

#define INTP_CAP_NONE			0u
#define INTP_CAP_PERFMON		(1u << 0)
#define INTP_CAP_BPF			(1u << 1)
#define INTP_CAP_DAC_READ_SEARCH	(1u << 2)
#define INTP_CAP_SYS_PTRACE		(1u << 3)

#define INTP_VENDOR_INTEL		(1u << 0)
#define INTP_VENDOR_AMD			(1u << 1)
#define INTP_VENDOR_ARM			(1u << 2)
#define INTP_VENDOR_RISCV		(1u << 3)
#define INTP_VENDOR_ANY			(~0u)

struct intp_plugin_requirements {
	const char *min_kernel;	/* "5.8" or NULL */
	bool        needs_btf;
	uint32_t    needs_capabilities;
	uint32_t    vendor_filter;
	const char *const *needs_paths;	/* NULL-terminated; may be NULL */
};

/* ------------------------------------------------------------------------ */
/* Plugin context and sample buffer (opaque)                                  */
/* ------------------------------------------------------------------------ */

/**
 * struct intp_plugin_ctx — opaque per-plugin context.
 *
 * Allocated by the loader, passed to every vtable callback. Plugins
 * may attach private state via intp_plugin_ctx_set_private() and
 * retrieve it via intp_plugin_ctx_get_private().
 */
struct intp_plugin_ctx;

void *intp_plugin_ctx_get_private(struct intp_plugin_ctx *ctx);
void  intp_plugin_ctx_set_private(struct intp_plugin_ctx *ctx, void *priv);

/**
 * struct intp_sample_buffer — opaque buffer plugins append metric
 * values to, from inside their sample() callback.
 *
 * Use intp_sample_append_double / _i64 / _u64 to populate it. The
 * buffer is owned by the dispatcher; do not free it.
 */
struct intp_sample_buffer;

int intp_sample_append_double(struct intp_sample_buffer *buf,
			      const char *name,
			      enum intp_unit unit,
			      double value);

int intp_sample_append_i64(struct intp_sample_buffer *buf,
			   const char *name,
			   enum intp_unit unit,
			   int64_t value);

int intp_sample_append_u64(struct intp_sample_buffer *buf,
			   const char *name,
			   enum intp_unit unit,
			   uint64_t value);

/**
 * intp_sample_append_unavailable — record that a metric is
 * unavailable on this host for this sample. Distinct from "zero".
 */
int intp_sample_append_unavailable(struct intp_sample_buffer *buf,
				   const char *name);

/* ------------------------------------------------------------------------ */
/* Vtables                                                                   */
/* ------------------------------------------------------------------------ */

struct intp_backend_vtable {
	const char        *backs;	/* core metric name this backs */
	enum intp_priority priority;
	int  (*probe)(void);
	int  (*init)(struct intp_plugin_ctx *ctx);
	int  (*sample)(struct intp_plugin_ctx *ctx,
		       struct intp_sample_buffer *out);
	void (*shutdown)(struct intp_plugin_ctx *ctx);
};

struct intp_metric_provider_vtable {
	int  (*init)(struct intp_plugin_ctx *ctx);
	int  (*sample)(struct intp_plugin_ctx *ctx,
		       struct intp_sample_buffer *out);
	void (*shutdown)(struct intp_plugin_ctx *ctx);
};

struct intp_output_vtable {
	const char *format_name;
	int  (*open)(struct intp_plugin_ctx *ctx, int fd);
	int  (*emit)(struct intp_plugin_ctx *ctx,
		     const struct intp_sample *sample);
	void (*close)(struct intp_plugin_ctx *ctx);
};

/* ------------------------------------------------------------------------ */
/* Plugin descriptor                                                         */
/* ------------------------------------------------------------------------ */

struct intp_plugin_descriptor {
	uint32_t                          abi_min;
	uint32_t                          abi_max;
	const char                       *name;
	const char                       *vendor;
	const char                       *license;	/* SPDX */
	const char                       *version;
	enum intp_plugin_kind             kind;
	const struct intp_metric_schema  *metrics;	/* NULL-terminated list, may be NULL for backends */
	struct intp_plugin_requirements   requires;
	union {
		struct intp_backend_vtable          backend;
		struct intp_metric_provider_vtable  metric_provider;
		struct intp_output_vtable           output;
	} vt;
};

/* ------------------------------------------------------------------------ */
/* Registration macro                                                        */
/* ------------------------------------------------------------------------ */

#ifndef INTP_PLUGIN_VISIBLE
#define INTP_PLUGIN_VISIBLE __attribute__((visibility("default")))
#endif

/**
 * INTP_PLUGIN_DEFINE — declare a plugin's descriptor.
 *
 * Expands to an exported `intp_plugin_descriptor` symbol that the
 * loader resolves via dlsym(3). The ABI version range is filled in
 * automatically with the version this header was built against;
 * plugins compiled against newer headers can declare a wider range
 * by setting `.abi_min`/`.abi_max` explicitly inside the macro
 * arguments (the explicit values override the defaults).
 *
 * Usage:
 *   INTP_PLUGIN_DEFINE(
 *           .name    = "hello-world",
 *           .vendor  = "intp examples",
 *           .license = "MIT",
 *           .version = "0.1.0",
 *           .kind    = INTP_PLUGIN_KIND_METRIC_PROVIDER,
 *           .metrics = my_metric_table,
 *           .vt.metric_provider = {
 *                   .init     = my_init,
 *                   .sample   = my_sample,
 *                   .shutdown = my_shutdown,
 *           },
 *   );
 */
#define INTP_PLUGIN_DEFINE(...)							\
	INTP_PLUGIN_VISIBLE							\
	const struct intp_plugin_descriptor intp_plugin_descriptor = {		\
		.abi_min = INTP_PLUGIN_ABI_VERSION,				\
		.abi_max = INTP_PLUGIN_ABI_VERSION,				\
		__VA_ARGS__							\
	}

/**
 * intp_plugin_self_test — optional plugin-side smoke check.
 *
 * Plugins may export this symbol; the loader runs it once before
 * registering the plugin. Returning a non-zero value causes the
 * plugin to be skipped, with the failure logged.
 */
int intp_plugin_self_test(void);

#ifdef __cplusplus
}
#endif

#endif /* INTP_PLUGIN_H */
