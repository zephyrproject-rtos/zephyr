/*
 * Copyright (c) 2024 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel_version.h>
#include <zephyr/app_version.h>

ZTEST(app_version, test_basic_ints)
{
	/* From VERSION */
	zassert_equal(5, APP_VERSION_MAJOR);
	zassert_equal(6, APP_VERSION_MINOR);
	zassert_equal(7, APP_PATCHLEVEL);
	zassert_equal(0, APP_TWEAK);
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
	zassert_str_equal("rc.8", STRINGIFY(APP_EXTRAVERSION));
	zassert_str_equal("5.6.7-rc.8", APP_VERSION_STRING);
	zassert_str_equal("5.6.7-rc.8+7936cf0", APP_VERSION_EXTENDED_STRING);
	zassert_str_equal("5.6.7+0", APP_VERSION_TWEAK_STRING);
}

ZTEST_SUITE(app_version, NULL, NULL, NULL, NULL, NULL);
