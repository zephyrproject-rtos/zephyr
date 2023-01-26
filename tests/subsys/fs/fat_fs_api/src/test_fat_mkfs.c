/*
 * Copyright (c) 2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "test_fat.h"
#include <ff.h>
#include <zephyr/storage/flash_map.h>

/* FatFs work area */
static FATFS fat_fs;

/* mounting info */
static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = "/NAND:",
	.fs_data = &fat_fs,
};

void test_fs_mkfs_simple(void);
void test_fs_mkfs_ops(void);

struct fs_mount_t *fs_mkfs_mp = &fatfs_mnt;
const int fs_mkfs_type = FS_FATFS;
uintptr_t fs_mkfs_dev_id = (uintptr_t) "NAND:";
int fs_mkfs_flags;
const char *some_file_path = "/NAND:/SOME";
const char *other_dir_path = "/NAND:/OTHER";

static int wipe_partition(void)
{
	/* In this test the first partition on flash device is used for FAT */
	unsigned int id = 0;
	const struct flash_area *pfa;
	int rc = flash_area_open(id, &pfa);

	if (rc < 0) {
		TC_PRINT("Error accessing flash area %u [%d]\n",
			 id, rc);
		return TC_FAIL;
	}

	TC_PRINT("Erasing %zu (0x%zx) bytes\n", pfa->fa_size, pfa->fa_size);
	rc = flash_area_erase(pfa, 0, pfa->fa_size);
	(void)flash_area_close(pfa);

	if (rc < 0) {
		TC_PRINT("Error wiping flash area %u [%d]\n",
			 id, rc);
		return TC_FAIL;
	}

	return TC_PASS;
}

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
