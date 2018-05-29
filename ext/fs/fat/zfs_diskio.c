/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/
/*----------------------------------------------------------------------------/
/  FatFs - Generic FAT file system module  R0.12a                             /
/-----------------------------------------------------------------------------/
/
/ Copyright (C) 2016, ChaN, all right reserved.
/
/ FatFs module is an open source software. Redistribution and use of FatFs in
/ source and binary forms, with or without modification, are permitted provided
/ that the following condition is met:

/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
/----------------------------------------------------------------------------*/

#include <diskio.h>	/* FatFs lower layer API */
#include <ffconf.h>
#include <disk_access.h>

static const char* const pdrv_str[] = {_VOLUME_STRS};

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv)
{
	__ASSERT(pdrv < ARRAY_SIZE(pdrv_str), "pdrv out-of-range\n");

	if (disk_access_status(pdrv_str[pdrv]) != 0) {
		return STA_NOINIT;
	} else {
		return RES_OK;
	}
}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv)
{
	__ASSERT(pdrv < ARRAY_SIZE(pdrv_str), "pdrv out-of-range\n");

	if (disk_access_init(pdrv_str[pdrv]) != 0) {
		return STA_NOINIT;
	} else {
		return RES_OK;
	}
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
	__ASSERT(pdrv < ARRAY_SIZE(pdrv_str), "pdrv out-of-range\n");

	if (disk_access_read(pdrv_str[pdrv], buff, sector, count) != 0) {
		return RES_ERROR;
	} else {
		return RES_OK;
	}

}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
	__ASSERT(pdrv < ARRAY_SIZE(pdrv_str), "pdrv out-of-range\n");

	if(disk_access_write(pdrv_str[pdrv], buff, sector, count) != 0) {
		return RES_ERROR;
	} else {
		return RES_OK;
	}
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
	int ret =  RES_OK;

	__ASSERT(pdrv < ARRAY_SIZE(pdrv_str), "pdrv out-of-range\n");

	switch (cmd) {
	case CTRL_SYNC:
		if(disk_access_ioctl(pdrv_str[pdrv],
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
