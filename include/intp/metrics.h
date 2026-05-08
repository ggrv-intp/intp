/* SPDX-License-Identifier: MIT */
/*
 * intp/metrics.h — metric data types and the seven core metric IDs.
 *
 * This header defines:
 *   - The seven core metric identifiers (system.interference.*).
 *   - The unit enum used in metric values and schema descriptors.
 *   - The struct intp_metric_value (one per metric in a sample).
 *   - The struct intp_sample (a single sampling tick — timestamp,
 *     target identification, list of metric values).
 *
 * Plugin-defined metrics live in their own namespaces; their values
 * use the same intp_metric_value carrier.
 *
 * Stability: from intp 1.0 onward, the seven core metric names and
 * their units are frozen. Plugin metrics are namespaced and may
 * evolve independently.
 */

#ifndef INTP_METRICS_H
#define INTP_METRICS_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */
/* Core metric names                                                         */
/* ------------------------------------------------------------------------ */

#define INTP_METRIC_NETP	"system.interference.netp"
#define INTP_METRIC_NETS	"system.interference.nets"
#define INTP_METRIC_BLK		"system.interference.blk"
#define INTP_METRIC_MBW		"system.interference.mbw"
#define INTP_METRIC_LLCMR	"system.interference.llcmr"
#define INTP_METRIC_LLCOCC	"system.interference.llcocc"
#define INTP_METRIC_CPU		"system.interference.cpu"

/* Convenient list for iteration. */
#define INTP_CORE_METRICS_COUNT	7

extern const char *const intp_core_metric_names[INTP_CORE_METRICS_COUNT];

/* ------------------------------------------------------------------------ */
/* Units                                                                     */
/* ------------------------------------------------------------------------ */

enum intp_unit {
	INTP_UNIT_UNSPECIFIED = 0,
	INTP_UNIT_RATIO,	/* fractional [0.0, 1.0] */
	INTP_UNIT_PERCENT,	/* integer or fractional [0, 100] */
	INTP_UNIT_BYTES,
	INTP_UNIT_BYTES_PER_SECOND,
	INTP_UNIT_OPS_PER_SECOND,
	INTP_UNIT_NANOSECONDS,
	INTP_UNIT_MICROSECONDS,
	INTP_UNIT_MILLISECONDS,
	INTP_UNIT_SECONDS,
	INTP_UNIT_COUNT,	/* monotonic counter */
};

const char *intp_unit_name(enum intp_unit u);

/* ------------------------------------------------------------------------ */
/* Metric value                                                              */
/* ------------------------------------------------------------------------ */

enum intp_value_kind {
	INTP_VALUE_NONE = 0,	/* metric unavailable on this host */
	INTP_VALUE_DOUBLE,
	INTP_VALUE_INT64,
	INTP_VALUE_UINT64,
};

struct intp_metric_value {
	const char         *name;	/* fully-qualified, e.g. "system.interference.cpu" */
	enum intp_unit      unit;
	enum intp_value_kind kind;
	union {
		double   d;
		int64_t  i64;
		uint64_t u64;
	};
};

/* ------------------------------------------------------------------------ */
/* Sample                                                                    */
/* ------------------------------------------------------------------------ */

/**
 * struct intp_sample — one sampling tick's worth of metric values.
 *
 * The values array is owned by the libintp context that produced the
 * sample; do not free it. It is stable until the next intp_sample()
 * call on the same context.
 */
struct intp_sample {
	struct timespec               ts;	/* wall-clock time of sample end */
	const char                   *host;	/* host identifier (hostname / nodename) */
	const struct intp_metric_value *values;
	size_t                         values_len;
};

/**
 * intp_sample_find — convenience lookup by metric name.
 *
 * Returns NULL if the metric is not present in the sample (or is
 * unavailable, i.e. kind == INTP_VALUE_NONE).
 */
const struct intp_metric_value *
intp_sample_find(const struct intp_sample *sample, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* INTP_METRICS_H */
