/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <misc/__assert.h>
#include <disk_access.h>
#include <errno.h>
#include <init.h>
#include <device.h>

#define RAMDISK_SECTOR_SIZE 512

#if defined(CONFIG_USB_MASS_STORAGE)
/* A 16KB initialized RAMdisk which will fit on most target's RAM. It
 * is initialized with a valid file system for validating USB mass storage.
 */
#include "fat12_ramdisk.h"
#else
/* A 96KB RAM Disk, which meets ELM FAT fs's minimum block requirement. Fit for
 * qemu testing (as it may exceed target's RAM limits).
 */
#define RAMDISK_VOLUME_SIZE (192 * RAMDISK_SECTOR_SIZE)
static u8_t ramdisk_buf[RAMDISK_VOLUME_SIZE];
#endif

static void *lba_to_address(u32_t lba)
{
	__ASSERT(((lba * RAMDISK_SECTOR_SIZE) < RAMDISK_VOLUME_SIZE),
		 "FS bound error");

	return &ramdisk_buf[(lba * RAMDISK_SECTOR_SIZE)];
}

static int disk_ram_access_status(struct disk_info *disk)
{
	return DISK_STATUS_OK;
}

static int disk_ram_access_init(struct disk_info *disk)
{
	return 0;
}

static int disk_ram_access_read(struct disk_info *disk, u8_t *buff,
				u32_t sector, u32_t count)
{
	memcpy(buff, lba_to_address(sector), count * RAMDISK_SECTOR_SIZE);

	return 0;
}

static int disk_ram_access_write(struct disk_info *disk, const u8_t *buff,
				 u32_t sector, u32_t count)
{
	memcpy(lba_to_address(sector), buff, count * RAMDISK_SECTOR_SIZE);

	return 0;
}

static int disk_ram_access_ioctl(struct disk_info *disk, u8_t cmd, void *buff)
{
	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		break;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(u32_t *)buff = RAMDISK_VOLUME_SIZE / RAMDISK_SECTOR_SIZE;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(u32_t *)buff = RAMDISK_SECTOR_SIZE;
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(u32_t *)buff  = 1;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct disk_operations ram_disk_ops = {
	.init = disk_ram_access_init,
	.status = disk_ram_access_status,
	.read = disk_ram_access_read,
	.write = disk_ram_access_write,
	.ioctl = disk_ram_access_ioctl,
};

static struct disk_info ram_disk = {
	.name = CONFIG_DISK_RAM_VOLUME_NAME,
	.ops = &ram_disk_ops,
};

static int disk_ram_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return disk_access_register(&ram_disk);
}

SYS_INIT(disk_ram_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
