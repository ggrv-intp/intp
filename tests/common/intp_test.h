/* SPDX-License-Identifier: MIT */
/*
 * intp_test.h — minimal in-tree assertion macros for libintp tests.
 *
 * We intentionally do not pull in a third-party test framework
 * (criterion, cmocka, …) at this stage. The matrix of distros we
 * build under in CI varies in which framework is packaged, and the
 * tests we need are simple. If the test surface grows enough to
 * justify it later, switching to criterion is one Meson dep + one
 * subdir change.
 *
 * Usage:
 *
 *   #include "intp_test.h"
 *
 *   int main(void)
 *   {
 *           INTP_TEST_BEGIN("my-suite");
 *           INTP_EXPECT_EQ(2 + 2, 4);
 *           INTP_EXPECT_STREQ(intp_unit_name(INTP_UNIT_RATIO), "ratio");
 *           return INTP_TEST_END();
 *   }
 */

#ifndef INTP_TEST_H
#define INTP_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int  intp_test_failures_;
static const char *intp_test_suite_;

#define INTP_TEST_BEGIN(name)						\
	do {								\
		intp_test_failures_ = 0;				\
		intp_test_suite_   = (name);				\
		fprintf(stderr, "# suite: %s\n", intp_test_suite_);	\
	} while (0)

#define INTP_TEST_END()							\
	(intp_test_failures_ == 0 ? 0 : 1)

#define INTP_FAIL_(fmt, ...)						\
	do {								\
		intp_test_failures_++;					\
		fprintf(stderr, "FAIL %s:%d: " fmt "\n",		\
			__FILE__, __LINE__, ##__VA_ARGS__);		\
	} while (0)

#define INTP_EXPECT(cond)						\
	do {								\
		if (!(cond))						\
			INTP_FAIL_("expected: %s", #cond);		\
	} while (0)

#define INTP_EXPECT_EQ(a, b)						\
	do {								\
		long long _a = (long long)(a);				\
		long long _b = (long long)(b);				\
		if (_a != _b)						\
			INTP_FAIL_("expected %s == %s; got %lld vs %lld", \
				   #a, #b, _a, _b);			\
	} while (0)

#define INTP_EXPECT_STREQ(a, b)						\
	do {								\
		const char *_a = (a);					\
		const char *_b = (b);					\
		if (!_a || !_b || strcmp(_a, _b) != 0)			\
			INTP_FAIL_("expected %s == %s; got \"%s\" vs \"%s\"", \
				   #a, #b,				\
				   _a ? _a : "(null)",			\
				   _b ? _b : "(null)");			\
	} while (0)

#define INTP_EXPECT_NOT_NULL(p)						\
	do {								\
		if ((p) == NULL)					\
			INTP_FAIL_("expected non-NULL: %s", #p);	\
	} while (0)

#endif /* INTP_TEST_H */
