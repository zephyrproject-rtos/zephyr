/*
 * Copyright (c) 2024 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel_version.h>
#include <zephyr/toolchain/common.h>

#include "app_version.h"

ZTEST(mcuboot_version, test_sign_version)
{
#ifdef CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION_GIT_BUILD
	char expected[32] = {0};

	/* Generate expected value */
	snprintf(expected, sizeof(expected), "%d.%d.%d+%d", APP_VERSION_MAJOR, APP_VERSION_MINOR,
		 APP_PATCHLEVEL, APP_GIT_COMMIT_HASH_SHORT);
	zassert_mem_equal(CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION, expected, strlen(expected));
#else
	/* Values straight from VERSION */
	zassert_mem_equal(CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION, "10.20.30+40",
			  strlen(CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION));
#endif /* CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION_GIT_BUILD */
}

ZTEST_SUITE(mcuboot_version, NULL, NULL, NULL, NULL, NULL);
