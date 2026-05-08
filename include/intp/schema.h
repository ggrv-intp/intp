/* SPDX-License-Identifier: MIT */
/*
 * intp/schema.h — schema descriptors and JSON Schema dump helpers.
 *
 * Every metric — core or plugin-defined — declares a schema fragment
 * that records its name, unit, and description. The set of fragments
 * loaded into a libintp context is the *active schema*, retrievable
 * via intp_print_schema() (see intp.h).
 *
 * Plugins use intp_metric_schema entries to declare the metrics they
 * publish; backends use them to describe the metrics they back.
 *
 * Stability: the type layout below is part of the plugin ABI.
 */

#ifndef INTP_SCHEMA_H
#define INTP_SCHEMA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <intp/metrics.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */
/* Schema versioning                                                         */
/* ------------------------------------------------------------------------ */

#define INTP_SCHEMA_VERSION_STR "0.1.0"

/* ------------------------------------------------------------------------ */
/* Per-metric schema fragment                                                */
/* ------------------------------------------------------------------------ */

struct intp_metric_schema {
	/** Fully-qualified metric name (e.g. "system.interference.cpu",
	 *  "gpu.nvidia.sm_occupancy", "db.postgres.lock_wait_seconds"). */
	const char     *name;

	/** Unit of the value. */
	enum intp_unit  unit;

	/** Optional one-line description, suitable for help output and
	 *  Prometheus exposition `# HELP` lines. */
	const char     *description;

	/** True if the metric is always present when its provider is
	 *  loaded; false if it can be conditionally unavailable per host. */
	bool            always_present;
};

/* ------------------------------------------------------------------------ */
/* Active schema introspection                                               */
/* ------------------------------------------------------------------------ */

struct intp_ctx;	/* forward-declared in intp.h */

/**
 * intp_schema_size — number of metric schema fragments in the active set.
 */
size_t intp_schema_size(const struct intp_ctx *ctx);

/**
 * intp_schema_get — fetch one schema fragment by index, [0, size).
 */
const struct intp_metric_schema *
intp_schema_get(const struct intp_ctx *ctx, size_t index);

/**
 * intp_schema_find — fetch a schema fragment by metric name, or NULL.
 */
const struct intp_metric_schema *
intp_schema_find(const struct intp_ctx *ctx, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* INTP_SCHEMA_H */
