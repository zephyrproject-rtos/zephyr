/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2021 SILA Embedded Solutions GmbH <office@embedded-solutions.at>
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
	.storage_dev = (void *)FLASH_AREA_ID(small),
	.mnt_point = TESTFS_MNT_POINT_SMALL,
};

#if CONFIG_APP_TEST_CUSTOM
FS_LITTLEFS_DECLARE_CUSTOM_CONFIG(medium, MEDIUM_IO_SIZE, MEDIUM_IO_SIZE,
				  MEDIUM_CACHE_SIZE, MEDIUM_LOOKAHEAD_SIZE);
struct fs_mount_t testfs_medium_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &medium,
	.storage_dev = (void *)FLASH_AREA_ID(medium),
	.mnt_point = TESTFS_MNT_POINT_MEDIUM,
};

/*
 * The large version is created intentionally not via a macro,
 * as this simulates the possible use of littlefs without the
 * initialization macros.
 */
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
	.storage_dev = (void *)FLASH_AREA_ID(large),
	.mnt_point = TESTFS_MNT_POINT_LARGE,
};

static int ram_disk_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
	void *buffer, lfs_size_t size);
static int ram_disk_program(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
	const void *buffer, lfs_size_t size);
static int ram_disk_erase(const struct lfs_config *c, lfs_block_t block);
static int ram_disk_sync(const struct lfs_config *c);
static int ram_disk_open(void **context, uintptr_t area_id);
static void ram_disk_close(void *context);
static size_t ram_disk_get_block_size(void *context);
static size_t ram_disk_get_size(void *context);

FS_LITTLEFS_DECLARE_CUSTOM_CONFIG_CUSTOM_OPS(ram_disk,
	CONFIG_FS_LITTLEFS_READ_SIZE, CONFIG_FS_LITTLEFS_PROG_SIZE,
	CONFIG_FS_LITTLEFS_CACHE_SIZE, CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE,
	ram_disk_read, ram_disk_program, ram_disk_erase, ram_disk_sync,
	ram_disk_open, ram_disk_close,
	ram_disk_get_block_size, ram_disk_get_size);

struct fs_mount_t testfs_ram_disk_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &ram_disk,
	.storage_dev = 0,
	.mnt_point = TESTFS_MNT_POINT_RAM_DISK,
};

/* only here to test the macro */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG_CUSTOM_OPS(ram_disk_default,
	ram_disk_read, ram_disk_program, ram_disk_erase, ram_disk_sync,
	ram_disk_open, ram_disk_close,
	ram_disk_get_block_size, ram_disk_get_size);

struct fs_mount_t testfs_ram_disk_default_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &ram_disk_default,
	.storage_dev = 0,
	.mnt_point = TESTFS_MNT_POINT_RAM_DISK,
};

#define RAM_DISK_BLOCK_SIZE 4096

static uint8_t ram_disk_storage[16*RAM_DISK_BLOCK_SIZE];
static uintptr_t ram_disk_context;

int ram_disk_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
	void *buffer, lfs_size_t size)
{
	size_t complete_offset = block * c->block_size + off;

	if (complete_offset + size > sizeof(ram_disk_storage)) {
		return LFS_ERR_NOSPC;
	}

	memcpy(buffer, ram_disk_storage + complete_offset, size);

	return LFS_ERR_OK;
}

int ram_disk_program(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
	const void *buffer, lfs_size_t size)
{
	size_t complete_offset = block * c->block_size + off;

	if (complete_offset + size > sizeof(ram_disk_storage)) {
		return LFS_ERR_NOSPC;
	}

	memcpy(ram_disk_storage + complete_offset, buffer, size);

	return LFS_ERR_OK;
}

int ram_disk_erase(const struct lfs_config *c, lfs_block_t block)
{
	size_t complete_offset = block * c->block_size;

	if (complete_offset + c->block_size > sizeof(ram_disk_storage)) {
		return LFS_ERR_NOSPC;
	}

	memset(ram_disk_storage + complete_offset, 0, c->block_size);

	return LFS_ERR_OK;
}

int ram_disk_sync(const struct lfs_config *c)
{
	return LFS_ERR_OK;
}

int ram_disk_open(void **context, uintptr_t area_id)
{
	ram_disk_context = area_id;
	*context = &ram_disk_context;
	return 0;
}

void ram_disk_close(void *context)
{
}

size_t ram_disk_get_block_size(void *context)
{
	return RAM_DISK_BLOCK_SIZE;
}

size_t ram_disk_get_size(void *context)
{
	return sizeof(ram_disk_storage);
}

int testfs_lfs_wipe_ram_disk(void)
{
	memset(ram_disk_storage, 0, sizeof(ram_disk_storage));
	TC_PRINT("ram disk wiped\n");
	return TC_PASS;
}

#else /* CONFIG_APP_TEST_CUSTOM */

int testfs_lfs_wipe_ram_disk(void)
{
	TC_PRINT("cannot wipe ram disk, build with wrong config\n");
	return TC_FAIL;
}

#endif /* !CONFIG_APP_TEST_CUSTOM */

int testfs_lfs_wipe_flash_partition(const struct fs_mount_t *mp)
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
