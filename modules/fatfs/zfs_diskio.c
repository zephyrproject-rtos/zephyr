/*
 * Copyright (c) 2018-2021 Zephyr contributors
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* The file is based on template file by (C)ChaN, 2019, as
 * available from FAT FS module source:
 * https://github.com/zephyrproject-rtos/fatfs/blob/master/diskio.c
 * and has been previously available from directory for that module
 * under name zfs_diskio.c.
 */
#include <ff.h>
#include <diskio.h>	/* FatFs lower layer API */
#include <zfs_diskio.h> /* Zephyr specific FatFS API */
#include <zephyr/storage/disk_access.h>

#if CONFIG_FS_FATFS_CUSTOM_MOUNT_POINT_COUNT
#define PDRV_STR_ARRAY VolumeStr
#else
static const char *const pdrv_str[] = {FF_VOLUME_STRS};
#define PDRV_STR_ARRAY pdrv_str
#endif /* CONFIG_FS_FATFS_CUSTOM_MOUNT_POINT_COUNT */

/* Get Drive Status */
DSTATUS disk_status(BYTE pdrv)
{
	__ASSERT(pdrv < ARRAY_SIZE(PDRV_STR_ARRAY), "pdrv out-of-range\n");

	if (disk_access_status(PDRV_STR_ARRAY[pdrv]) != 0) {
		return STA_NOINIT;
	} else {
		return RES_OK;
	}
}

/* Initialize a Drive */
DSTATUS disk_initialize(BYTE pdrv)
{
	uint8_t param = DISK_IOCTL_POWER_ON;

	return disk_ioctl(pdrv, CTRL_POWER, &param);
}

/* Read Sector(s) */
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
	__ASSERT(pdrv < ARRAY_SIZE(PDRV_STR_ARRAY), "pdrv out-of-range\n");

	if (disk_access_read(PDRV_STR_ARRAY[pdrv], buff, sector, count) != 0) {
		return RES_ERROR;
	} else {
		return RES_OK;
	}
}

/* Write Sector(s) */
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
	__ASSERT(pdrv < ARRAY_SIZE(PDRV_STR_ARRAY), "pdrv out-of-range\n");

	if (disk_access_write(PDRV_STR_ARRAY[pdrv], buff, sector, count) != 0) {
		return RES_ERROR;
	} else {
		return RES_OK;
	}
}

/* Miscellaneous Functions */
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
	int ret = RES_OK;
	uint32_t sector_size = 0;

	__ASSERT(pdrv < ARRAY_SIZE(PDRV_STR_ARRAY), "pdrv out-of-range\n");

	switch (cmd) {
	case CTRL_SYNC:
		if (disk_access_ioctl(PDRV_STR_ARRAY[pdrv], DISK_IOCTL_CTRL_SYNC, buff) != 0) {
			ret = RES_ERROR;
		}
		break;

	case GET_SECTOR_COUNT:
		if (disk_access_ioctl(PDRV_STR_ARRAY[pdrv], DISK_IOCTL_GET_SECTOR_COUNT, buff) !=
		    0) {
			ret = RES_ERROR;
		}
		break;

	case GET_SECTOR_SIZE:
		/* Zephyr's DISK_IOCTL_GET_SECTOR_SIZE returns sector size as a
		 * 32-bit number while FatFS's GET_SECTOR_SIZE is supposed to
		 * return a 16-bit number.
		 */
		if ((disk_access_ioctl(PDRV_STR_ARRAY[pdrv], DISK_IOCTL_GET_SECTOR_SIZE,
				       &sector_size) == 0) &&
		    (sector_size == (uint16_t)sector_size)) {
			*(uint16_t *)buff = (uint16_t)sector_size;
		} else {
			ret = RES_ERROR;
		}
		break;

	case GET_BLOCK_SIZE:
		if (disk_access_ioctl(PDRV_STR_ARRAY[pdrv], DISK_IOCTL_GET_ERASE_BLOCK_SZ, buff) !=
		    0) {
			ret = RES_ERROR;
		}
		break;

	/* Optional IOCTL command used by Zephyr fs_unmount implementation,
	 * not called by FATFS
	 */
	case CTRL_POWER:
		if (((*(uint8_t *)buff)) == DISK_IOCTL_POWER_OFF) {
			/* Power disk off */
			if (disk_access_ioctl(PDRV_STR_ARRAY[pdrv], DISK_IOCTL_CTRL_DEINIT, NULL) !=
			    0) {
				ret = RES_ERROR;
			}
		} else {
			/* Power disk on */
			if (disk_access_ioctl(PDRV_STR_ARRAY[pdrv], DISK_IOCTL_CTRL_INIT, NULL) !=
			    0) {
				ret = STA_NOINIT;
			}
		}
		break;

	default:
		ret = RES_PARERR;
		break;
	}
	return ret;
}
