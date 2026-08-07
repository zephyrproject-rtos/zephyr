/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2023 Husqvarna AB
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fat.h"
#ifdef CONFIG_DISK_DRIVER_FLASH
#include <zephyr/storage/flash_map.h>
#else
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/byteorder.h>
#endif

/* FatFs work area */
FATFS fat_fs;
struct fs_file_t filep;
const char test_str[] = "hello world!";

#if defined(CONFIG_FS_FATFS_MULTI_PARTITION)
/*
 * FatFS multi-partition mapping.
 *
 * Map logical drive 0 ("SD:") -> physical disk 0, MBR partition #1.
 * This allows formatting/mounting within the partition rather than using
 * super-floppy formatting on the whole card.
 */
PARTITION VolToPart[] = {
	{0, 1},
};
#endif

/* For large disks, we only send 1024 erase requests
 * This assumption relies on the fact that any filesystem headers will be
 * stored within this range, and is made to improve execution time of this
 * test
 */
#define MAX_ERASES 1024

int check_file_dir_exists(const char *path)
{
	int res;
	struct fs_dirent entry;

	/* Verify fs_stat() */
	res = fs_stat(path, &entry);

	return !res;
}

#ifdef CONFIG_DISK_DRIVER_FLASH
int wipe_partition(void)
{
	/* In this test the first partition on flash device is used for FAT */
	unsigned int id = 0;
	const struct flash_area *pfa;
	int rc = flash_area_open(id, &pfa);

	if (rc < 0) {
		TC_PRINT("Error accessing flash area %u [%d]\n", id, rc);
		return TC_FAIL;
	}

	TC_PRINT("Erasing %zu (0x%zx) bytes\n", pfa->fa_size, pfa->fa_size);
	rc = flash_area_flatten(pfa, 0, pfa->fa_size);
	(void)flash_area_close(pfa);

	if (rc < 0) {
		TC_PRINT("Error wiping flash area %u [%d]\n", id, rc);
		return TC_FAIL;
	}

	return TC_PASS;
}
#else
static uint8_t erase_buffer[4096] __aligned(32) = {0};

int wipe_partition(void)
{
	uint32_t sector_size;
	uint32_t sector_count;
	uint32_t sector_wr_jmp;
	uint32_t sector_wr_size;

	/*
	 * When using FatFS multi-partition, preserve the partition table (MBR) and
	 * only wipe the beginning of the first partition.
	 */
#if defined(CONFIG_FS_FATFS_MULTI_PARTITION)
	static uint8_t mbr[512] __aligned(32);
	uint32_t part_lba;
	uint32_t part_sectors;
#endif

	if (disk_access_init(DISK_NAME)) {
		TC_PRINT("Failed to init disk " DISK_NAME "\n");
		return TC_FAIL;
	}

#if defined(CONFIG_FS_FATFS_MULTI_PARTITION)
	if (disk_access_read(DISK_NAME, mbr, 0, 1)) {
		TC_PRINT("Failed to read MBR from disk " DISK_NAME "\n");
		return TC_FAIL;
	}

	/* Verify MBR signature 0x55AA */
	if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
		TC_PRINT("Invalid MBR signature on disk " DISK_NAME "\n");
		return TC_FAIL;
	}

	/* First partition entry starts at offset 446. Start LBA is at +8, size at +12 */
	part_lba = sys_get_le32(&mbr[446 + 8]);
	part_sectors = sys_get_le32(&mbr[446 + 12]);

	if (part_lba == 0U || part_sectors == 0U) {
		TC_PRINT("MBR partition #1 not found/invalid on disk " DISK_NAME "\n");
		return TC_FAIL;
	}
#endif

	if (disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count)) {
		TC_PRINT("Failed to get disk " DISK_NAME " sector count\n");
		return TC_FAIL;
	}
	if (disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_SIZE, &sector_size)) {
		TC_PRINT("Failed to get disk " DISK_NAME " sector size\n");
		return TC_FAIL;
	}

	if (sector_size > ARRAY_SIZE(erase_buffer)) {
		TC_PRINT("Predefined \"erase_buffer\" to small to handle single sector\n");
		return TC_FAIL;
	}

#if defined(CONFIG_FS_FATFS_MULTI_PARTITION)
	/* Wipe only within the first partition (cap to 64 sectors for speed) */
	sector_count = MIN(part_sectors, 64U);
#else
	if (sector_count > 64) {
		/* partition only in first 32k */
		sector_count = 64;
	}
#endif
	sector_wr_size = MIN(sector_size, ARRAY_SIZE(erase_buffer));
	sector_wr_jmp = sector_wr_size / sector_wr_size;
	TC_PRINT("For " DISK_NAME " using sector write size %" PRIu32 " to write %" PRIu32
		 " at once\n",
		 sector_wr_size, sector_wr_jmp);

	for (uint32_t sector_idx = 0; sector_idx < sector_count; sector_idx += sector_wr_jmp) {
#if defined(CONFIG_FS_FATFS_MULTI_PARTITION)
		uint32_t phys_sector = part_lba + sector_idx;
#else
		uint32_t phys_sector = sector_idx;
#endif
		if (disk_access_write(DISK_NAME, erase_buffer, phys_sector, 1)) {
			TC_PRINT("Failed to \"erase\" sector %" PRIu32 " to " DISK_NAME "\n",
				 phys_sector);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}
#endif
