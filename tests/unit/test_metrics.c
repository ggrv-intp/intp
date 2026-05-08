/* SPDX-License-Identifier: MIT */
/*
 * test_metrics — verify the seven core metric name constants exist,
 * are non-NULL, and follow the system.interference.* convention.
 */

#include <string.h>

#include <intp/metrics.h>

#include "intp_test.h"

int main(void)
{
	INTP_TEST_BEGIN("core-metric-names");

	INTP_EXPECT_EQ(INTP_CORE_METRICS_COUNT, 7);

	for (size_t i = 0; i < INTP_CORE_METRICS_COUNT; i++) {
		const char *n = intp_core_metric_names[i];
		INTP_EXPECT_NOT_NULL(n);
		INTP_EXPECT(strncmp(n, "system.interference.", 20) == 0);
	}

	/* Spot-check the named constants are wired correctly. */
	INTP_EXPECT_STREQ(INTP_METRIC_NETP,    "system.interference.netp");
	INTP_EXPECT_STREQ(INTP_METRIC_NETS,    "system.interference.nets");
	INTP_EXPECT_STREQ(INTP_METRIC_BLK,     "system.interference.blk");
	INTP_EXPECT_STREQ(INTP_METRIC_MBW,     "system.interference.mbw");
	INTP_EXPECT_STREQ(INTP_METRIC_LLCMR,   "system.interference.llcmr");
	INTP_EXPECT_STREQ(INTP_METRIC_LLCOCC,  "system.interference.llcocc");
	INTP_EXPECT_STREQ(INTP_METRIC_CPU,     "system.interference.cpu");

	return INTP_TEST_END();
}
