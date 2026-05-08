/* SPDX-License-Identifier: MIT */
/*
 * intp/intp.h — public C API for libintp.
 *
 * libintp is the Linux interference profiler library. It collects the
 * seven core IntP metrics (see metrics.h) plus any plugin-defined
 * metrics, and emits them as structured samples that callers can
 * read synchronously (intp_sample) or stream live (intp_subscribe).
 *
 * Lifecycle:
 *
 *	struct intp_ctx *ctx;
 *	int err = intp_open(&(struct intp_options) {
 *		.target = INTP_TARGET_SYSTEM,
 *		.interval_ms = 1000,
 *	}, &ctx);
 *	if (err)
 *		exit_or_handle(err);
 *
 *	struct intp_sample sample;
 *	while (intp_sample(ctx, &sample) == 0)
 *		consume(&sample);
 *
 *	intp_close(ctx);
 *
 * For live streaming consumers, use intp_subscribe instead of polling
 * intp_sample.
 *
 * This header is the only one libintp consumers need to include for
 * the synchronous polling API. metrics.h, schema.h, and plugin.h are
 * pulled in transitively where relevant.
 */

#ifndef INTP_INTP_H
#define INTP_INTP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <intp/metrics.h>
#include <intp/schema.h>
#include <intp/version.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */
/* Versioning                                                                */
/* ------------------------------------------------------------------------ */

/**
 * intp_version_string — return the libintp version as a static string.
 *
 * Format: "MAJOR.MINOR.PATCH" with optional "-suffix" for pre-release
 * builds. The string is statically allocated; do not free.
 */
const char *intp_version_string(void);

/* ------------------------------------------------------------------------ */
/* Targets                                                                   */
/* ------------------------------------------------------------------------ */

/**
 * enum intp_target_kind — what to profile.
 *
 * INTP_TARGET_SYSTEM	 - the whole host (no PID/cgroup attachment).
 * INTP_TARGET_PID	 - a single process. Use intp_target_set_pid().
 * INTP_TARGET_CGROUP	 - a cgroup-v2 path (post-1.0).
 */
enum intp_target_kind {
	INTP_TARGET_SYSTEM = 0,
	INTP_TARGET_PID,
	INTP_TARGET_CGROUP,
};

struct intp_target {
	enum intp_target_kind kind;
	union {
		pid_t       pid;
		const char *cgroup_path;
	};
};

/* ------------------------------------------------------------------------ */
/* Options                                                                   */
/* ------------------------------------------------------------------------ */

/**
 * struct intp_options — passed to intp_open().
 *
 * Zero-initialised callers get sensible defaults: system target,
 * 1000 ms interval, all backends auto-selected.
 */
struct intp_options {
	struct intp_target target;

	/** Sampling interval in milliseconds. Defaults to 1000. */
	uint32_t interval_ms;

	/** Optional plugin search dirs (NULL-terminated). NULL → use
	 *  the standard search path documented in PLUGINS.md. */
	const char *const *plugin_dirs;

	/** Optional list of explicitly disabled plugins (by name). */
	const char *const *disabled_plugins;

	/** Optional list of explicitly disabled backends (by name). */
	const char *const *disabled_backends;

	/** Verbosity: 0 = silent, 1 = warnings, 2 = info, 3 = debug. */
	int log_level;
};

/* ------------------------------------------------------------------------ */
/* Lifecycle                                                                 */
/* ------------------------------------------------------------------------ */

struct intp_ctx;

/**
 * intp_open — create a libintp context and resolve backends.
 *
 * Loads plugins from the configured search path, runs the capability
 * probe per backend, and selects the highest-priority backend
 * available for each metric. The result is recorded and visible via
 * intp_print_schema().
 *
 * Returns 0 on success; a negative errno-style code on failure.
 */
int intp_open(const struct intp_options *opts, struct intp_ctx **out);

/**
 * intp_close — tear down a libintp context.
 *
 * Idempotent; safe to call with NULL.
 */
void intp_close(struct intp_ctx *ctx);

/* ------------------------------------------------------------------------ */
/* Synchronous sampling                                                      */
/* ------------------------------------------------------------------------ */

/**
 * intp_sample — collect one sample.
 *
 * Blocks until the next sample is ready (subject to opts.interval_ms).
 * The returned struct's lifetime is tied to ctx; do not free its
 * fields. Subsequent calls overwrite the buffer.
 *
 * Returns 0 on success, -EINTR on signal, other negative errno on
 * failure.
 */
int intp_sample(struct intp_ctx *ctx, struct intp_sample *out);

/* ------------------------------------------------------------------------ */
/* Streaming                                                                 */
/* ------------------------------------------------------------------------ */

/**
 * intp_sample_callback — callback invoked once per sample.
 *
 * The sample pointer is valid for the duration of the callback only.
 * Returning a non-zero value asks intp_subscribe() to stop and return.
 */
typedef int (*intp_sample_callback)(const struct intp_sample *sample,
				    void *user);

/**
 * intp_subscribe — stream samples to the callback until it asks to stop.
 *
 * Same selection logic as intp_sample(); samples are emitted at
 * opts.interval_ms cadence.
 *
 * Returns 0 if the callback asked to stop cleanly, negative errno on
 * failure.
 */
int intp_subscribe(struct intp_ctx *ctx,
		   intp_sample_callback cb,
		   void *user);

/* ------------------------------------------------------------------------ */
/* Schema introspection                                                      */
/* ------------------------------------------------------------------------ */

/**
 * intp_print_schema — write the active schema as JSON Schema to fd.
 *
 * Includes the resolved backend selection per metric, the loaded
 * plugins, and the metric catalogue. Suitable for downstream code
 * generation and for human inspection.
 *
 * Returns 0 on success, negative errno on failure.
 */
int intp_print_schema(struct intp_ctx *ctx, int fd);

/* ------------------------------------------------------------------------ */
/* Error handling                                                            */
/* ------------------------------------------------------------------------ */

/**
 * intp_strerror — translate an intp error code into a static string.
 */
const char *intp_strerror(int err);

#ifdef __cplusplus
}
#endif

#endif /* INTP_INTP_H */
