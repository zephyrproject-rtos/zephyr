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

void *test_mount(void)
{
	int res;

	res = fs_mount(&fatfs_mnt);
	if (res < 0) {
		TC_ERROR("Error mounting fs [%d]\n", res);
		/* FIXME: restructure tests as per #46897 */
		__ASSERT_NO_MSG(res == 0);
	}

	return NULL;
}

void test_unmount(void *unused)
{
	int res;

	ARG_UNUSED(unused);
	res = fs_unmount(&fatfs_mnt);
	if (res < 0) {
		TC_ERROR("Error unmounting fs [%d]\n", res);
		/* FIXME: restructure tests as per #46897 */
		__ASSERT_NO_MSG(res == 0);
	}
}

/**
 * @brief Test for File System mount operation
 *
 * @details Test initializes the fs_mount_t data structure with FatFs
 * related info and calls the fs_mount API for mount the file system.
 */
ZTEST(posix_fs_test, test_fs_mount)
{
	/* FIXME: restructure tests as per #46897 */
	zassert_equal(fatfs_mnt.flags, FS_MOUNT_FLAG_USE_DISK_ACCESS);
}
