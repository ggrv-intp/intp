/* SPDX-License-Identifier: MIT */
/*
 * fixture.h — load text fixtures from tests/fixtures/.
 *
 * Stage 1.5 scaffold: a thin wrapper around fopen() so tests can
 * portably reference fixture files without hard-coding paths
 * relative to the build directory. The Meson harness sets the
 * INTP_TEST_FIXTURE_DIR environment variable to point at
 * tests/fixtures/ in the source tree.
 */

#ifndef INTP_TEST_FIXTURE_H
#define INTP_TEST_FIXTURE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline FILE *intp_test_fixture_open(const char *relpath)
{
	const char *root = getenv("INTP_TEST_FIXTURE_DIR");
	char buf[1024];

	if (!root)
		root = "tests/fixtures";

	if (snprintf(buf, sizeof(buf), "%s/%s", root, relpath)
	    >= (int)sizeof(buf))
		return NULL;
	return fopen(buf, "r");
}

#endif /* INTP_TEST_FIXTURE_H */
