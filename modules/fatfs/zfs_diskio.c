/*
 * Copyright (c) 2018-2021 Zephyr contributors
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* The file is based on template file by (C)ChaN, 2019, as
 * available from FAT FS module source:
 * https://github.com/zephyrproject-rtos/fatfs/blob/master/diskio.c
 * and has been previously avaialble from directory for that module
 * under name zfs_diskio.c.
 */
#include <ff.h>
#include <diskio.h>	/* FatFs lower layer API */
#include <zephyr/storage/disk_access.h>

#if defined(CONFIG_FF_VOLUME_STRS_OVERRIDE)
extern char *VolumeStr[FF_VOLUMES];
#define pdrv_str VolumeStr
#else
static const char * const pdrv_str[] = {FF_VOLUME_STRS};
#endif

/* Get Drive Status */
DSTATUS disk_status(BYTE pdrv)
{
	__ASSERT(pdrv < ARRAY_SIZE(pdrv_str), "pdrv out-of-range\n");

	if (disk_access_status(pdrv_str[pdrv]) != 0) {
		return STA_NOINIT;
	} else {
		return RES_OK;
	}
}

/* Initialize a Drive */
DSTATUS disk_initialize(BYTE pdrv)
{
	__ASSERT(pdrv < ARRAY_SIZE(pdrv_str), "pdrv out-of-range\n");

	if (disk_access_init(pdrv_str[pdrv]) != 0) {
		return STA_NOINIT;
	} else {
		return RES_OK;
	}
}

/* Read Sector(s) */
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
	__ASSERT(pdrv < ARRAY_SIZE(pdrv_str), "pdrv out-of-range\n");

	if (disk_access_read(pdrv_str[pdrv], buff, sector, count) != 0) {
		return RES_ERROR;
	} else {
		return RES_OK;
	}

}

/* Write Sector(s) */
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
	__ASSERT(pdrv < ARRAY_SIZE(pdrv_str), "pdrv out-of-range\n");

	if (disk_access_write(pdrv_str[pdrv], buff, sector, count) != 0) {
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

	__ASSERT(pdrv < ARRAY_SIZE(pdrv_str), "pdrv out-of-range\n");

	switch (cmd) {
	case CTRL_SYNC:
		if (disk_access_ioctl(pdrv_str[pdrv],
				DISK_IOCTL_CTRL_SYNC, buff) != 0) {
			ret = RES_ERROR;
		}
		break;

	case GET_SECTOR_COUNT:
		if (disk_access_ioctl(pdrv_str[pdrv],
				DISK_IOCTL_GET_SECTOR_COUNT, buff) != 0) {
			ret = RES_ERROR;
		}
		break;

	case GET_SECTOR_SIZE:
		/* Zephyr's DISK_IOCTL_GET_SECTOR_SIZE returns sector size as a
		 * 32-bit number while FatFS's GET_SECTOR_SIZE is supposed to
		 * return a 16-bit number.
		 */
		if ((disk_access_ioctl(pdrv_str[pdrv],
				DISK_IOCTL_GET_SECTOR_SIZE, &sector_size) == 0) &&
			(sector_size == (uint16_t)sector_size)) {
			*(uint16_t *)buff = (uint16_t)sector_size;
		} else {
			ret = RES_ERROR;
		}
		break;

	case GET_BLOCK_SIZE:
		if (disk_access_ioctl(pdrv_str[pdrv],
				DISK_IOCTL_GET_ERASE_BLOCK_SZ, buff) != 0) {
			ret = RES_ERROR;
		}
		break;

	default:
		ret = RES_PARERR;
		break;
	}
	return ret;
}
