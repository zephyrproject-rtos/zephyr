/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Disk Access layer APIs and defines
 *
 * This file contains APIs for disk access. Apart from disks, various
 * other storage media like Flash and RAM disks may implement this interface to
 * be used by various higher layers(consumers) like USB Mass storage
 * and Filesystems.
 */

#ifndef ZEPHYR_INCLUDE_DISK_DISK_ACCESS_H_
#define ZEPHYR_INCLUDE_DISK_DISK_ACCESS_H_

#include <kernel.h>
#include <zephyr/types.h>
#include <sys/dlist.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Possible Cmd Codes for disk_ioctl() */

/* Get the number of sectors in the disk  */
#define DISK_IOCTL_GET_SECTOR_COUNT		1
/* Get the size of a disk SECTOR in bytes */
#define DISK_IOCTL_GET_SECTOR_SIZE		2
/* How many  sectors constitute a FLASH Erase block */
#define DISK_IOCTL_GET_ERASE_BLOCK_SZ		4
/* Commit any cached read/writes to disk */
#define DISK_IOCTL_CTRL_SYNC			5

/* 3 is reserved.  It used to be DISK_IOCTL_GET_DISK_SIZE */

/* Possible return bitmasks for disk_status() */
#define DISK_STATUS_OK			0x00
#define DISK_STATUS_UNINIT		0x01
#define DISK_STATUS_NOMEDIA		0x02
#define DISK_STATUS_WR_PROTECT		0x04

struct disk_operations;

struct disk_info {
	sys_dnode_t node;
	char *name;
	const struct disk_operations *ops;
	/* Disk device associated to this disk.
	 */
	struct device *dev;
};

struct disk_operations {
	int (*init)(struct disk_info *disk);
	int (*status)(struct disk_info *disk);
	int (*read)(struct disk_info *disk, u8_t *data_buf,
		    u32_t start_sector, u32_t num_sector);
	int (*write)(struct disk_info *disk, const u8_t *data_buf,
		     u32_t start_sector, u32_t num_sector);
	int (*ioctl)(struct disk_info *disk, u8_t cmd, void *buff);
};

/*
 * @brief perform any initialization
 *
 * This call is made by the consumer before doing any IO calls so that the
 * disk or the backing device can do any initialization.
 *
 * @return 0 on success, negative errno code on fail
 */
int disk_access_init(const char *pdrv);

/*
 * @brief Get the status of disk
 *
 * This call is used to get the status of the disk
 *
 * @return DISK_STATUS_OK or other DISK_STATUS_*s
 */
int disk_access_status(const char *pdrv);

/*
 * @brief read data from disk
 *
 * Function to read data from disk to a memory buffer.
 *
 * @param[in] data_buf      Pointer to the memory buffer to put data.
 * @param[in] start_sector  Start disk sector to read from
 * @param[in] num_sector    Number of disk sectors to read
 *
 * @return 0 on success, negative errno code on fail
 */
int disk_access_read(const char *pdrv, u8_t *data_buf,
		     u32_t start_sector, u32_t num_sector);

/*
 * @brief write data to disk
 *
 * Function write data from memory buffer to disk.
 *
 * @param[in] data_buf      Pointer to the memory buffer
 * @param[in] start_sector  Start disk sector to write to
 * @param[in] num_sector    Number of disk sectors to write
 *
 * @return 0 on success, negative errno code on fail
 */
int disk_access_write(const char *pdrv, const u8_t *data_buf,
		      u32_t start_sector, u32_t num_sector);

/*
 * @brief Get/Configure disk parameters
 *
 * Function to get disk parameters and make any special device requests.
 *
 * @param[in] cmd  DISK_IOCTL_* code describing the request
 *
 * @return 0 on success, negative errno code on fail
 */
int disk_access_ioctl(const char *pdrv, u8_t cmd, void *buff);

int disk_access_register(struct disk_info *disk);

int disk_access_unregister(struct disk_info *disk);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DISK_DISK_ACCESS_H_ */
