/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @filesystem
 * @brief test_filesystem
 * Demonstrates the ZEPHYR File System APIs
 */

#include "test_fat.h"
#include <ff.h>

/* FatFs work area */
static FATFS fat_fs;

/* mounting info */
static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.fs_data = &fat_fs,
};

static int test_mount(void)
{
	int res;

	res = fs_mount(&fatfs_mnt);
	if (res < 0) {
		TC_PRINT("Error mounting fs [%d]\n", res);
		return TC_FAIL;
	}

	return TC_PASS;
}

void test_fat_mount(void)
{
	zassert_true(test_mount() == TC_PASS, NULL);
}
