/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @filesystem
 * @brief test Zephyr file system generic features
 * to demonstrates the ZEPHYR File System APIs
 */

#include "test_fat.h"

static int test_statvfs(void)
{
	struct fs_statvfs stat;
	int res;

	/* Verify fs_statvfs() */
	res = fs_statvfs(FATFS_MNTP, &stat);
	if (res) {
		TC_PRINT("Error getting volume stats [%d]\n", res);
		return res;
	}

	TC_PRINT("\n");
	TC_PRINT("Optimal transfer block size   = %lu\n", stat.f_bsize);
	TC_PRINT("Allocation unit size          = %lu\n", stat.f_frsize);
	TC_PRINT("Volume size in f_frsize units = %lu\n", stat.f_blocks);
	TC_PRINT("Free space in f_frsize units  = %lu\n", stat.f_bfree);

	return TC_PASS;
}

void test_fat_fs(void)
{
	zassert_true(test_statvfs() == TC_PASS, NULL);
}
