/*
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

static void test_prepare(void)
{
	struct fs_file_t fs;

	fs_file_t_init(&fs);
	zassert_equal(fs_mount(&fatfs_mnt), 0);
	zassert_equal(fs_open(&fs, FATFS_MNTP"/testfile.txt", FS_O_CREATE),
		      0, NULL);
	zassert_equal(fs_close(&fs), 0);
	zassert_equal(fs_unmount(&fatfs_mnt), 0);
}

static void test_unmount(void)
{
	zassert_true(fs_unmount(&fatfs_mnt) >= 0);
}

static void test_ops_on_rd(void)
{
	struct fs_file_t fs;
	int ret;

	fs_file_t_init(&fs);
	/* Check fs operation on volume mounted with FS_MOUNT_FLAG_READ_ONLY */
	fatfs_mnt.flags = FS_MOUNT_FLAG_READ_ONLY;
	TC_PRINT("Mount as read-only\n");
	ret = fs_mount(&fatfs_mnt);
	zassert_equal(ret, 0, "Expected success", ret);

	/* Attempt creating new file */
	ret = fs_open(&fs, FATFS_MNTP"/nosome", FS_O_CREATE);
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_mkdir(FATFS_MNTP"/another");
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_rename(FATFS_MNTP"/testfile.txt", FATFS_MNTP"/bestfile.txt");
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_unlink(FATFS_MNTP"/testfile.txt");
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_open(&fs, FATFS_MNTP"/testfile.txt", FS_O_RDWR);
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_open(&fs, FATFS_MNTP"/testfile.txt", FS_O_READ);
	zassert_equal(ret, 0, "Expected success", ret);
	fs_close(&fs);
}

ZTEST(fat_fs_basic, test_fat_mount_rd_only)
{
	test_prepare();
	test_ops_on_rd();
	test_unmount();
}
