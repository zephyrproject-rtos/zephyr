/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <fs/littlefs.h>
#include <storage/flash_map.h>
#include "testfs_lfs.h"

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(small);
struct fs_mount_t testfs_small_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &small,
	.storage_dev = (void *)DT_FLASH_AREA_SMALL_ID,
	.mnt_point = TESTFS_MNT_POINT_SMALL,
};

FS_LITTLEFS_DECLARE_CUSTOM_CONFIG(medium, MEDIUM_IO_SIZE, MEDIUM_IO_SIZE,
				  MEDIUM_CACHE_SIZE, MEDIUM_LOOKAHEAD_SIZE);
struct fs_mount_t testfs_medium_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &medium,
	.storage_dev = (void *)DT_FLASH_AREA_MEDIUM_ID,
	.mnt_point = TESTFS_MNT_POINT_MEDIUM,
};

static u8_t large_read_buffer[LARGE_CACHE_SIZE];
static u8_t large_prog_buffer[LARGE_CACHE_SIZE];
static u32_t large_lookahead_buffer[LARGE_LOOKAHEAD_SIZE / 4U];
static struct fs_littlefs large = {
	.cfg = {
		.read_size = LARGE_IO_SIZE,
		.prog_size = LARGE_IO_SIZE,
		.cache_size = LARGE_CACHE_SIZE,
		.lookahead_size = LARGE_LOOKAHEAD_SIZE,
		.block_size = 32768, /* increase erase size */
		.read_buffer = large_read_buffer,
		.prog_buffer = large_prog_buffer,
		.lookahead_buffer = large_lookahead_buffer,
	},
};
struct fs_mount_t testfs_large_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &large,
	.storage_dev = (void *)DT_FLASH_AREA_LARGE_ID,
	.mnt_point = TESTFS_MNT_POINT_LARGE,
};

int testfs_lfs_wipe_partition(const struct fs_mount_t *mp)
{
	unsigned int id = (uintptr_t)mp->storage_dev;
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

	TC_PRINT("Wiped flash area %u for %s\n", id, mp->mnt_point);
	return TC_PASS;
}
