/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "test_fat.h"

/* mounting info */
static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.fs_data = &fat_fs,
};

static int test_mount_no_format(void)
{
	int ret = 0;

	fatfs_mnt.flags = FS_MOUNT_FLAG_NO_FORMAT;
	ret = fs_mount(&fatfs_mnt);

	if (ret >= 0) {
		TC_PRINT("Expected failure\n");
		return TC_FAIL;
	}
	fatfs_mnt.flags = 0;

	return TC_PASS;
}

static int test_mount_rd_only_no_sys(void)
{
	int ret = 0;

	fatfs_mnt.flags = FS_MOUNT_FLAG_READ_ONLY;
	ret = fs_mount(&fatfs_mnt);

	if (ret >= 0) {
		TC_PRINT("Expected failure\n");
		return TC_FAIL;
	}
	fatfs_mnt.flags = 0;

	return TC_PASS;
}

static int test_mount_use_disk_access(void)
{
	int res;

	fatfs_mnt.flags = FS_MOUNT_FLAG_USE_DISK_ACCESS;
	res = fs_mount(&fatfs_mnt);
	if (res < 0) {
		TC_PRINT("Error mounting fs [%d]\n", res);
		return TC_FAIL;
	}
	return ((fatfs_mnt.flags & FS_MOUNT_FLAG_USE_DISK_ACCESS) ? TC_PASS : TC_FAIL);
}

static int test_mount(void)
{
	int res;

	fatfs_mnt.flags = 0;
	res = fs_mount(&fatfs_mnt);
	if (res < 0) {
		TC_PRINT("Error mounting fs [%d]\n", res);
		return TC_FAIL;
	}
	return ((fatfs_mnt.flags & FS_MOUNT_FLAG_USE_DISK_ACCESS) ? TC_PASS : TC_FAIL);
}

static int test_unmount(void)
{
	return (fs_unmount(&fatfs_mnt) >= 0 ? TC_PASS : TC_FAIL);
}

void test_fat_unmount(void)
{
	zassert_true(test_unmount() == TC_PASS);
}

void test_fat_mount(void)
{
	zassert_false(test_unmount() == TC_PASS);
	zassert_true(test_mount_no_format() == TC_PASS);
	zassert_true(test_mount_rd_only_no_sys() == TC_PASS);
	zassert_true(test_mount_use_disk_access() == TC_PASS);
	zassert_true(test_unmount() == TC_PASS);
	zassert_true(test_mount() == TC_PASS);
	zassert_false(test_mount() == TC_PASS);
}
