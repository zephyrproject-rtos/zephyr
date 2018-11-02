/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zffs/zffs.h>
#include <fs.h>
#include <flash_map.h>
#include <misc/printk.h>
#include <zephyr.h>
#include <string.h>

#define ZFFS_ROOT_DIR "/zffs"

struct zffs_data zd;
struct zffs_area *area[3][ZFFS_CONFIG_AREA_MAX + 1];
struct fs_mount_t fs_mnt;

void main(void)
{
	int rc;

	if (flash_area_open(FLASH_AREA_EXTERNAL_ID,
			    (const struct flash_area **)&fs_mnt.storage_dev)) {
		return;
	}

	zd.area = area[0];
	zd.swap_area = area[1];
	zd.data_area = area[2];

	fs_mnt.type = FS_ZFFS;
	fs_mnt.mnt_point = ZFFS_ROOT_DIR;
	fs_mnt.fs_data = &zd;

	if ((rc = fs_mount(&fs_mnt)) != 0) {
		flash_area_close(fs_mnt.storage_dev);
		return;
	}

	fs_unmount(&fs_mnt);
	flash_area_close(fs_mnt.storage_dev);
}
