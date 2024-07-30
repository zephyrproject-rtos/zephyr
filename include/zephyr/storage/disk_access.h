/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Disk Access layer API
 *
 * This file contains APIs for disk access.
 */

#ifndef ZEPHYR_INCLUDE_STORAGE_DISK_ACCESS_H_
#define ZEPHYR_INCLUDE_STORAGE_DISK_ACCESS_H_

/**
 * @brief Storage APIs
 * @defgroup storage_apis Storage APIs
 * @ingroup os_services
 * @{
 * @}
 */

/**
 * @brief Disk Access APIs
 * @defgroup disk_access_interface Disk Access Interface
 * @ingroup storage_apis
 * @{
 */

#include <zephyr/drivers/disk.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief perform any initialization
 *
 * This call is made by the consumer before doing any IO calls so that the
 * disk or the backing device can do any initialization. Although still
 * supported for legacy compatibility, users should instead call
 * @ref disk_access_ioctl with the IOCTL @ref DISK_IOCTL_CTRL_INIT.
 *
 * Disk initialization is reference counted, so only the first successful call
 * to initialize a uninitialized (or previously de-initialized) disk will
 * actually initialize the disk
 *
 * @param[in] pdrv          Disk name
 *
 * @return 0 on success, negative errno code on fail
 */
int disk_access_init(const char *pdrv);

/**
 * @brief Get the status of disk
 *
 * This call is used to get the status of the disk
 *
 * @param[in] pdrv          Disk name
 *
 * @return DISK_STATUS_OK or other DISK_STATUS_*s
 */
int disk_access_status(const char *pdrv);

/**
 * @brief read data from disk
 *
 * Function to read data from disk to a memory buffer.
 *
 * Note: if he disk is of NVMe type, user will need to ensure data_buf
 *       pointer is 4-bytes aligned.
 *
 * @param[in] pdrv          Disk name
 * @param[in] data_buf      Pointer to the memory buffer to put data.
 * @param[in] start_sector  Start disk sector to read from
 * @param[in] num_sector    Number of disk sectors to read
 *
 * @return 0 on success, negative errno code on fail
 */
int disk_access_read(const char *pdrv, uint8_t *data_buf,
		     uint32_t start_sector, uint32_t num_sector);

/**
 * @brief write data to disk
 *
 * Function write data from memory buffer to disk.
 *
 * Note: if he disk is of NVMe type, user will need to ensure data_buf
 *       pointer is 4-bytes aligned.
 *
 * @param[in] pdrv          Disk name
 * @param[in] data_buf      Pointer to the memory buffer
 * @param[in] start_sector  Start disk sector to write to
 * @param[in] num_sector    Number of disk sectors to write
 *
 * @return 0 on success, negative errno code on fail
 */
int disk_access_write(const char *pdrv, const uint8_t *data_buf,
		      uint32_t start_sector, uint32_t num_sector);

/**
 * @brief Get/Configure disk parameters
 *
 * Function to get disk parameters and make any special device requests.
 *
 * @param[in] pdrv          Disk name
 * @param[in] cmd           DISK_IOCTL_* code describing the request
 * @param[in] buff          Command data buffer
 *
 * @return 0 on success, negative errno code on fail
 */
int disk_access_ioctl(const char *pdrv, uint8_t cmd, void *buff);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_STORAGE_DISK_ACCESS_H_ */
