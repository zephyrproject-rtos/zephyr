/*
 * Copyright (c) 2024 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdlib.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel_version.h>
#include <zephyr/toolchain/common.h>

#include "app_version.h"

ZTEST(app_version, test_basic_ints)
{
	/* From VERSION */
	zassert_equal(5, APP_VERSION_MAJOR);
	zassert_equal(6, APP_VERSION_MINOR);
	zassert_equal(7, APP_PATCHLEVEL);
	zassert_equal(89, APP_TWEAK);
	zassert_equal(0x050607, APP_VERSION_NUMBER);
}

ZTEST(app_version, test_appversion)
{
	/* From the APPVERSION value */
	zassert_equal(5, SYS_KERNEL_VER_MAJOR(APPVERSION));
	zassert_equal(6, SYS_KERNEL_VER_MINOR(APPVERSION));
	zassert_equal(7, SYS_KERNEL_VER_PATCHLEVEL(APPVERSION));
}

ZTEST(app_version, test_basic_strings)
{
	zassert_equal(0, strcmp("5.6.7-development", APP_VERSION_STRING));
	zassert_equal(0, strcmp("5.6.7-development+89", APP_VERSION_EXTENDED_STRING));
	zassert_equal(0, strcmp("5.6.7+89", APP_VERSION_TWEAK_STRING));
}

ZTEST(app_version, test_git_hash)
{
	/*
	 * The git hashes in the two strings should be equal
	 *
	 * #define APP_GIT_COMMIT_HASH_SHORT    0x5c35660b
	 * #define APP_BUILD_VERSION            v3.7.0-rc3-76-g5c35660b97bb
	 *
	 * Extract the numeric value from `APP_BUILD_VERSION`
	 */
	char build_version[] = STRINGIFY(APP_BUILD_VERSION);
	int v_len = strlen(build_version);
	unsigned int hash_from_string;
	int g_idx = v_len;

	/* Find the index of the `g` character, starting from the back */
	while (g_idx-- > 0) {
		if (build_version[g_idx] == 'g') {
			break;
		}
	}
	/* Should have terminated the loop early */
	zassert_not_equal(0, g_idx);
	/* Should be more than 8 characters between 'g' and end */
	zassert_true((v_len - g_idx) > 8);
	/* Modify buffer to only contain 8 character hex string */
	g_idx++;
	build_version[g_idx + 8] = '\x00';
	zassert_equal(8, strlen(build_version + g_idx));
	/* Convert hex string to integer */
	hash_from_string = strtoul(build_version + g_idx, NULL, 16);
	/* Compare to define from build system */
	zassert_equal(APP_GIT_COMMIT_HASH_SHORT, hash_from_string);
}

ZTEST_SUITE(app_version, NULL, NULL, NULL, NULL, NULL);
