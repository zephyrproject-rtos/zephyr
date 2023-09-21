/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include "testfs_lfs.h"

#define SMALL_PARTITION		small_partition
#define SMALL_PARTITION_ID	FIXED_PARTITION_ID(SMALL_PARTITION)

#define MEDIUM_PARTITION	medium_partition
#define MEDIUM_PARTITION_ID	FIXED_PARTITION_ID(MEDIUM_PARTITION)

#define LARGE_PARTITION		large_partition
#define LARGE_PARTITION_ID	FIXED_PARTITION_ID(LARGE_PARTITION)

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(small);
struct fs_mount_t testfs_small_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &small,
	.storage_dev = (void *)SMALL_PARTITION_ID,
	.mnt_point = TESTFS_MNT_POINT_SMALL,
};

#if CONFIG_APP_TEST_CUSTOM
FS_LITTLEFS_DECLARE_CUSTOM_CONFIG(medium, 4, MEDIUM_IO_SIZE, MEDIUM_IO_SIZE,
				  MEDIUM_CACHE_SIZE, MEDIUM_LOOKAHEAD_SIZE);
struct fs_mount_t testfs_medium_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &medium,
	.storage_dev = (void *)MEDIUM_PARTITION_ID,
	.mnt_point = TESTFS_MNT_POINT_MEDIUM,
};

static uint8_t large_read_buffer[LARGE_CACHE_SIZE];
static uint8_t large_prog_buffer[LARGE_CACHE_SIZE];
static uint32_t large_lookahead_buffer[LARGE_LOOKAHEAD_SIZE / 4U];
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
	.storage_dev = (void *)LARGE_PARTITION_ID,
	.mnt_point = TESTFS_MNT_POINT_LARGE,
};

#endif /* CONFIG_APP_TEST_CUSTOM */

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
