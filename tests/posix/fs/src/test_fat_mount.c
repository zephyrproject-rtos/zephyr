/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fs/fs.h>
#include <ff.h>
#include "test_fs.h"

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

/**
 * @brief Test for File System mount operation
 *
 * @details Test initializes the fs_mount_t data structure with FatFs
 * related info and calls the fs_mount API for mount the file system.
 */
void test_fs_mount(void)
{
	zassert_true(test_mount() == TC_PASS, NULL);
}
