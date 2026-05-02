/*
 * Copyright (c) 2025 James Roy <rruuaanng@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_fstab_fatfs
#include "test_fs.h"

#define TEST_NODELABEL DT_NODELABEL(test_disk)

ZTEST(test_fs_dt_api, test_dt_api_mount_point)
{
	const char *mnt_point1, *mnt_point2;

	mnt_point1 = FSTAB_ENTRY_DT_MOUNT_POINT(TEST_NODELABEL);
	zassert_str_equal(mnt_point1, "/TEST:");

	mnt_point2 = FSTAB_ENTRY_DT_INST_MOUNT_POINT(0);
	zassert_str_equal(mnt_point2, "/TEST:");
}

ZTEST_SUITE(test_fs_dt_api, NULL, NULL, NULL, NULL, NULL);
