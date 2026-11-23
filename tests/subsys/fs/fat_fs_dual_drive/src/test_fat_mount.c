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
static FATFS fat_fs1;

/* mounting info */
static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.fs_data = &fat_fs,
};

/* mounting info */
static struct fs_mount_t fatfs_mnt1 = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP1,
	.fs_data = &fat_fs1,
};

static int test_mount(struct fs_mount_t *mnt)
{
	int res;

	res = fs_mount(mnt);
	if (res < 0) {
		TC_PRINT("Error mounting fs [%d]\n", res);
		return TC_FAIL;
	}

	return TC_PASS;
}

void test_fat_mount(void)
{
	TC_PRINT("Mounting %s\n", FATFS_MNTP);
	zassert_true(test_mount(&fatfs_mnt) == TC_PASS);

	TC_PRINT("Mounting %s\n", FATFS_MNTP1);
	zassert_true(test_mount(&fatfs_mnt1) == TC_PASS);
}
