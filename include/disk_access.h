/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#ifndef _DISK_ACCESS_H_
#define _DISK_ACCESS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Possible Cmd Codes for disk_ioctl() */

/* Get the number of sectors in the disk  */
#define DISK_IOCTL_GET_SECTOR_COUNT		1
/* Get the size of a disk SECTOR in bytes */
#define DISK_IOCTL_GET_SECTOR_SIZE		2
/* Get the size of the disk in bytes */
#define DISK_IOCTL_GET_DISK_SIZE		3
/* How many  sectors constitute a FLASH Erase block */
#define DISK_IOCTL_GET_ERASE_BLOCK_SZ		4
/* Commit any cached read/writes to disk */
#define DISK_IOCTL_CTRL_SYNC			5

/* Possible return bitmasks for disk_status() */
#define DISK_STATUS_OK			0x00
#define DISK_STATUS_UNINIT		0x01
#define DISK_STATUS_NOMEDIA		0x02
#define DISK_STATUS_WR_PROTECT		0x04

/*
 * @brief perform any intialization
 *
 * This call is made by the consumer before doing any IO calls so that the
 * disk or the backing device can do any initialization.
 *
 * @return 0 on success, negative errno code on fail
 */
int disk_access_init(void);

/*
 * @brief Get the status of disk
 *
 * This call is used to get the status of the disk
 *
 * @return DISK_STATUS_OK or other DISK_STATUS_*s
 */
int disk_access_status(void);

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
int disk_access_read(uint8_t *data_buf, uint32_t start_sector,
		     uint32_t num_sector);

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
int disk_access_write(const uint8_t *data_buf, uint32_t start_sector,
		      uint32_t num_sector);

/*
 * @brief Get/Configure disk parameters
 *
 * Function to get disk parameters and make any special device requests.
 *
 * @param[in] cmd  DISK_IOCTL_* code describing the request
 *
 * @return 0 on success, negative errno code on fail
 */
int disk_access_ioctl(uint8_t cmd, void *buff);


#ifdef __cplusplus
}
#endif

#endif /* _DISK_ACCESS_H_ */
