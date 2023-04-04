/**
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  usbh_msc.h
 * @brief USB Host Mass Storage Class public header
 *
 * Integrates Mass Storage Class APIs with file system disk access APIs
 */

#ifndef ZEPHYR_INCLUDE_USBH_MSC_H_
#define ZEPHYR_INCLUDE_USBH_MSC_H_

#include <zephyr/device.h>

/**
 * @brief Disk access function to read from the drive
 *
 * @param[in]  lun          Logical Unit Number
 * @param[out] data_buf     pointer to buffer to store read data
 * @param[in]  start_sector starting sector to read from the drive
 * @param[in]  num_sector   num of sectors to read
 * @retval     0            read success
 * @retval    -EIO          read failed
 * @retval    -EINVAL       invalid data buffer
 */
int usbh_disk_access_read(const uint8_t lun, uint8_t *data_buf, uint32_t start_sector,
			  uint32_t num_sector);

/**
 * @brief Disk access function to write to the drive
 *
 * @param[in]  lun          Logical Unit Number
 * @param[in]  data_buf     pointer to buffer to load data from
 * @param[in]  start_sector starting sector to write to the drive
 * @param[in]  num_sector   num of sectors to write
 * @retval     0            write success
 * @retval    -EIO          write failed
 * @retval    -EINVAL       invalid data buffer
 */
int usbh_disk_access_write(const uint8_t lun, const uint8_t *data_buf, uint32_t start_sector,
			   uint32_t num_sector);

/**
 * @brief Disk access function to read status of the drive
 *
 * @param[in]  pdrv         drive logical unit number
 * @retval     0            drive ready
 * @retval    -EIO          drive not ready
 */
int usbh_disk_access_status(uint8_t pdrv);

/**
 * @brief Disk access function to initialize the drive
 *
 * @param[in]  pdrv         drive logical unit number
 * @retval     0            initialization successful
 * @retval    -EIO          initialization failed
 */
int usbh_disk_access_init(uint8_t pdrv);

/**
 * @brief Disk access function to perform ioctl operations on the drive
 *
 * @param[in]  pdrv         drive logical unit number
 * @param[in]  cmd          operation to perform
 * @param[out] buf          pointer to buffer to store data for an applicable command
 * @retval     0            operation successful
 * @retval    -EIO          operation failed
 */
int usbh_disk_access_ioctl(uint8_t pdrv, uint8_t cmd, void *buf);

/**
 * @brief Get maximum LUN of the drive
 *
 * @param[out]  max_lun     pointer to store max LUN value of the drive
 * @retval      0           operation successful
 * @retval     -EIO         operation failed
 */
int usbh_msc_scsi_get_max_lun(uint8_t *max_lun);

/**
 * @brief Get inquired details of the drive
 *
 * @param[out]  t10_vendor_id       pointer to store drive vendor id
 * @param[out]  product_id          pointer to store drive product id
 * @param[out]  product_revision    pointer to store drive product revision code
 * @retval      0                   operation successful
 * @retval     -EIO                 operation failed
 */
int usbh_msc_scsi_get_inquiry(uint8_t *t10_vendor_id, uint8_t *product_id,
			      uint8_t *product_revision);

/**
 * @brief Get capacity of the drive
 *
 * @param[out]  total_blocks        pointer to store total blocks on the drive
 * @param[out]  block_size          pointer to store block size of the drive
 * @retval      0                   operation successful
 * @retval     -EIO                 operation failed
 */
int usbh_msc_scsi_get_read_capacity(uint32_t *total_blocks, uint32_t *block_size);

#endif /* ZEPHYR_INCLUDE_USBH_MSC_H_ */
