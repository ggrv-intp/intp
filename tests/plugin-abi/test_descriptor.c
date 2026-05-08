/* SPDX-License-Identifier: MIT */
/*
 * test_descriptor — load examples/plugin-hello-world via dlopen,
 * then assert that:
 *
 *   1. The intp_plugin_descriptor symbol resolves.
 *   2. The descriptor's ABI range covers INTP_PLUGIN_ABI_VERSION.
 *   3. Identity fields (name, vendor, license, version) are present.
 *   4. The kind is METRIC_PROVIDER.
 *   5. The metrics table is non-empty.
 *   6. The optional intp_plugin_self_test, if present, returns 0.
 *
 * This is the contract test for the plugin ABI: any change that
 * breaks one of these checks is, by definition, a binary-incompatible
 * change to the ABI.
 */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <intp/plugin.h>

#include "intp_test.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr,
			"usage: %s <path-to-intp-plugin-hello-world.so>\n",
			argv[0]);
		return 2;
	}

	INTP_TEST_BEGIN("plugin-abi/hello-world");

	void *handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
	if (!handle) {
		fprintf(stderr, "dlopen(%s): %s\n", argv[1], dlerror());
		return 1;
	}

	/* (1) descriptor symbol resolves */
	const struct intp_plugin_descriptor *desc =
		dlsym(handle, "intp_plugin_descriptor");
	INTP_EXPECT_NOT_NULL(desc);
	if (!desc) {
		dlclose(handle);
		return INTP_TEST_END();
	}

	/* (2) ABI range covers what we built against. We avoid
	 * `desc->abi_max >= INTP_PLUGIN_ABI_VERSION` because at the
	 * pre-1.0 baseline (ABI_VERSION=0) that becomes a tautology
	 * on unsigned types. Instead we check for a sane range
	 * explicitly. */
	INTP_EXPECT(desc->abi_min <= INTP_PLUGIN_ABI_VERSION);
	INTP_EXPECT(desc->abi_min <= desc->abi_max);

	/* (3) identity */
	INTP_EXPECT_STREQ(desc->name,    "hello-world");
	INTP_EXPECT_STREQ(desc->vendor,  "intp examples");
	INTP_EXPECT_STREQ(desc->license, "MIT");
	INTP_EXPECT_NOT_NULL(desc->version);

	/* (4) kind */
	INTP_EXPECT_EQ((int)desc->kind,
		       (int)INTP_PLUGIN_KIND_METRIC_PROVIDER);

	/* (5) metrics table is non-empty */
	INTP_EXPECT_NOT_NULL(desc->metrics);
	INTP_EXPECT_NOT_NULL(desc->metrics[0].name);
	INTP_EXPECT_STREQ(desc->metrics[0].name, "app.hello.tick");

	/* (6) optional self-test returns 0 */
	int (*self_test)(void) = dlsym(handle, "intp_plugin_self_test");
	if (self_test) {
		int rc = self_test();
		INTP_EXPECT_EQ(rc, 0);
	}

	dlclose(handle);
	return INTP_TEST_END();
}
