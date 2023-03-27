/*
 * Copyright (c) 2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "test_fat.h"
#include <ff.h>
#ifdef CONFIG_DISK_DRIVER_FLASH
#include <zephyr/storage/flash_map.h>
#else
#include <zephyr/storage/disk_access.h>
#endif

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

#ifdef CONFIG_DISK_DRIVER_FLASH
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
#else
static uint8_t erase_buffer[4096] = { 0 };

static int wipe_partition(void)
{
	uint32_t sector_size;
	uint32_t sector_count;
	uint32_t sector_wr_jmp;
	uint32_t sector_wr_size;

	if (disk_access_init(DISK_NAME)) {
		TC_PRINT("Failed to init disk "DISK_NAME"\n");
		return TC_FAIL;
	}
	if (disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count)) {
		TC_PRINT("Failed to get disk "DISK_NAME" sector count\n");
		return TC_FAIL;
	}
	if (disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_SIZE, &sector_size)) {
		TC_PRINT("Failed to get disk "DISK_NAME" sector size\n");
		return TC_FAIL;
	}

	if (sector_size > ARRAY_SIZE(erase_buffer)) {
		TC_PRINT("Predefined \"erase_buffer\" to small to handle single sector\n");
		return TC_FAIL;
	}

	sector_wr_size = MIN(sector_size, ARRAY_SIZE(erase_buffer));
	sector_wr_jmp = sector_wr_size / sector_wr_size;
	TC_PRINT("For "DISK_NAME" using sector write size "PRIu32" to write "PRIu32" at once\n",
		 sector_wr_size, sector_wr_jmp);

	for (uint32_t sector_idx = 0; sector_idx < sector_count; sector_idx += sector_wr_jmp) {
		if (disk_access_write(DISK_NAME, erase_buffer, sector_idx, 1)) {
			TC_PRINT("Faield to \"erase\" sector "PRIu32" to "DISK_NAME"\n",
				 sector_idx);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}
#endif

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
