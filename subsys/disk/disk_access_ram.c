/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdint.h>
#include <misc/__assert.h>
#include <disk_access.h>
#include <errno.h>

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
static uint8_t ramdisk_buf[RAMDISK_VOLUME_SIZE];
#endif

static void *lba_to_address(uint32_t lba)
{
	__ASSERT(((lba * RAMDISK_SECTOR_SIZE) < RAMDISK_VOLUME_SIZE),
		 "FS bound error");

	return &ramdisk_buf[(lba * RAMDISK_SECTOR_SIZE)];
}

int disk_access_status(void)
{
	return DISK_STATUS_OK;
}

int disk_access_init(void)
{
	return 0;
}

int disk_access_read(uint8_t *buff, uint32_t sector, uint32_t count)
{
	memcpy(buff, lba_to_address(sector), count * RAMDISK_SECTOR_SIZE);

	return 0;
}

int disk_access_write(const uint8_t *buff, uint32_t sector, uint32_t count)
{
	memcpy(lba_to_address(sector), buff, count * RAMDISK_SECTOR_SIZE);

	return 0;
}

int disk_access_ioctl(uint8_t cmd, void *buff)
{
	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		break;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(uint32_t *)buff = RAMDISK_VOLUME_SIZE / RAMDISK_SECTOR_SIZE;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(uint32_t *)buff = RAMDISK_SECTOR_SIZE;
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		*(uint32_t *)buff  = 1;
		break;
	case DISK_IOCTL_GET_DISK_SIZE:
		*(uint32_t *)buff  = RAMDISK_VOLUME_SIZE;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
