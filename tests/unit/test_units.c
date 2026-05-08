/* SPDX-License-Identifier: MIT */
/*
 * test_units — verify intp_unit_name() returns sensible values
 * for every defined unit.
 */

#include <string.h>

#include <intp/metrics.h>

#include "intp_test.h"

int main(void)
{
	INTP_TEST_BEGIN("unit-names");

	INTP_EXPECT_STREQ(intp_unit_name(INTP_UNIT_UNSPECIFIED), "unspecified");
	INTP_EXPECT_STREQ(intp_unit_name(INTP_UNIT_RATIO),       "ratio");
	INTP_EXPECT_STREQ(intp_unit_name(INTP_UNIT_PERCENT),     "percent");
	INTP_EXPECT_STREQ(intp_unit_name(INTP_UNIT_BYTES),       "bytes");
	INTP_EXPECT_STREQ(intp_unit_name(INTP_UNIT_COUNT),       "count");

	/* Out-of-range values fall through to "unspecified". */
	INTP_EXPECT_STREQ(intp_unit_name((enum intp_unit)999),   "unspecified");

	return INTP_TEST_END();
}
