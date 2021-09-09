/*
 * Copyright (c) 2021 SILA Embedded Solutions GmbH <office@embedded-solutions.at>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <sys/__assert.h>
#include <ztest.h>
#include <fs/fs.h>
#include <fs/fs_sys.h>
#include <fs/littlefs.h>
#include "test_littlefs_custom.h"
#include "ram_disk.h"

#define FS_MNT_POINT_RAM_DISK "/ram"

FS_LITTLEFS_DECLARE_CUSTOM_CONFIG_CUSTOM_OPS(ram_disk,
	CONFIG_FS_LITTLEFS_READ_SIZE, CONFIG_FS_LITTLEFS_PROG_SIZE,
	CONFIG_FS_LITTLEFS_CACHE_SIZE, CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE,
	ram_disk_read, ram_disk_program, ram_disk_erase, ram_disk_sync,
	ram_disk_open, ram_disk_close,
	ram_disk_get_block_size, ram_disk_get_size);

struct fs_mount_t fs_ram_disk_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &ram_disk,
	.storage_dev = 0,
	.mnt_point = FS_MNT_POINT_RAM_DISK,
};

void test_custom_opendir(void)
{
	int ret;
	struct fs_dir_t directory;

	ret = fs_mount(&fs_ram_disk_mnt);
	zassert_equal(ret, 0, "failed to mount lfs");

	fs_dir_t_init(&directory);
	ret = fs_opendir(&directory, fs_ram_disk_mnt.mnt_point);
	zassert_equal(ret, 0, "failed to open directory of mount point");

	ret = fs_unmount(&fs_ram_disk_mnt);
	zassert_equal(ret, 0, "failed to unmount lfs");
}
