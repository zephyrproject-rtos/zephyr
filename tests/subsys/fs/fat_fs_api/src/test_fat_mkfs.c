/*
 * Copyright (c) 2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "test_fat.h"
#include <ff.h>

/* mounting info */
static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = "/"DISK_NAME":",
	.fs_data = &fat_fs,
};

void test_fs_mkfs_simple(void);
void test_fs_mkfs_ops(void);

struct fs_mount_t *fs_mkfs_mp = &fatfs_mnt;
const int fs_mkfs_type = FS_FATFS;
uintptr_t fs_mkfs_dev_id = (uintptr_t) DISK_NAME":";
int fs_mkfs_flags;
const char *some_file_path = "/"DISK_NAME":/SOME";
const char *other_dir_path = "/"DISK_NAME":/OTHER";

ZTEST(fat_fs_mkfs, test_mkfs_simple)
{
	int ret;

	ret = wipe_partition();
	zassert_equal(ret, TC_PASS, "wipe partition failed %d", ret);

	test_fs_mkfs_simple();
}

ZTEST(fat_fs_mkfs, test_mkfs_ops)
{
	int ret;

	ret = wipe_partition();
	zassert_equal(ret, TC_PASS, "wipe partition failed %d", ret);

	test_fs_mkfs_ops();
}

static MKFS_PARM custom_cfg = {
	.fmt = FM_ANY | FM_SFD,	/* Any suitable FAT */
	.n_fat = 1,		/* One FAT fs table */
	.align = 0,		/* Get sector size via diskio query */
	.n_root = CONFIG_FS_FATFS_MAX_ROOT_ENTRIES,
	.au_size = 0		/* Auto calculate cluster size */
};

ZTEST(fat_fs_mkfs, test_mkfs_custom)
{
	int ret;
	struct fs_mount_t mp = fatfs_mnt;
	struct fs_statvfs sbuf;

	ret = wipe_partition();
	zassert_equal(ret, 0, "wipe partition failed %d", ret);

	ret = fs_mkfs(FS_FATFS, fs_mkfs_dev_id, &custom_cfg, 0);
	zassert_equal(ret, 0, "mkfs failed %d", ret);


	mp.flags = FS_MOUNT_FLAG_NO_FORMAT;
	ret = fs_mount(&mp);
	zassert_equal(ret, 0, "mount failed %d", ret);

	ret = fs_statvfs(mp.mnt_point, &sbuf);
	zassert_equal(ret, 0, "statvfs failed %d", ret);

	TC_PRINT("statvfs: %lu %lu %lu %lu",
			sbuf.f_bsize, sbuf.f_frsize, sbuf.f_blocks, sbuf.f_bfree);

	ret = fs_unmount(&mp);
	zassert_equal(ret, 0, "unmount failed %d", ret);
}

ZTEST_SUITE(fat_fs_mkfs, NULL, NULL, NULL, NULL, NULL);
