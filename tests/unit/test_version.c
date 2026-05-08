/* SPDX-License-Identifier: MIT */
/*
 * test_version — sanity-check the libintp version string.
 *
 * The exact version is set by Meson and is not stable across
 * branches, so we only assert structural properties: non-NULL,
 * non-empty, looks like X.Y.Z (with optional prerelease suffix).
 */

#include <ctype.h>
#include <string.h>

#include <intp/intp.h>

#include "intp_test.h"

static int looks_like_semver(const char *s)
{
	int saw_digit = 0;
	int dot_count = 0;

	while (*s) {
		if (isdigit((unsigned char)*s)) {
			saw_digit = 1;
		} else if (*s == '.') {
			dot_count++;
			if (dot_count > 3)
				return 0;
		} else if (*s == '-' || *s == '+' ||
			   isalpha((unsigned char)*s)) {
			break;	/* prerelease/build suffix */
		} else {
			return 0;
		}
		s++;
	}

	return saw_digit && dot_count >= 2;
}

int main(void)
{
	INTP_TEST_BEGIN("version");

	const char *v = intp_version_string();
	INTP_EXPECT_NOT_NULL(v);
	INTP_EXPECT(strlen(v) > 0);
	INTP_EXPECT(looks_like_semver(v));

	return INTP_TEST_END();
}
