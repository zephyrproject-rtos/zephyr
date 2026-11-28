/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB host Mass Storage Class public header
 *
 * Header exposes block access to an attached mass storage medium.
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBH_MSC_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBH_MSC_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief USB host Mass Storage Class API
 * @defgroup usbh_msc_class USB host Mass Storage Class API
 * @ingroup usb
 * @since 4.5
 * @version 0.1.0
 * @{
 */

/** Opaque handle to the first logical unit of an attached mass storage device. */
struct usbh_msc_lun;

/**
 * @brief Get the handle of an attached mass storage device.
 *
 * There are @kconfig{CONFIG_USBH_MSC_DEVICE_COUNT} indices. An attached
 * device takes the lowest index that is free, and the index is freed again
 * when the device is removed. Only the first logical unit of a device is
 * used.
 *
 * The handle stays valid only while the device is attached. Use
 * usbh_msc_get_generation() to tell whether the medium behind an index was
 * changed since it was last used.
 *
 * @param[in] idx Device index
 *
 * @return Handle, or NULL when no device is attached at this index.
 */
struct usbh_msc_lun *usbh_msc_lun_get(unsigned int idx);

/**
 * @brief Read blocks from the medium.
 *
 * @param[in]  lun   Logical unit handle
 * @param[in]  lba   First logical block to read
 * @param[in]  count Number of blocks to read
 * @param[out] buf   Destination buffer of @p count blocks
 *
 * @retval 0 On success
 * @retval -ENODEV No medium ready, the medium was removed or changed, or @p lun is NULL
 * @retval -EINVAL The requested range is outside the medium
 * @retval -ETIMEDOUT The device did not answer
 * @retval -ENOMEM No transfer buffer available
 * @retval -EIO The device or the transport reported an error
 */
int usbh_msc_read(struct usbh_msc_lun *lun, uint32_t lba, uint32_t count, uint8_t *buf);

/**
 * @brief Write blocks to the medium.
 *
 * @param[in] lun   Logical unit handle
 * @param[in] lba   First logical block to write
 * @param[in] count Number of blocks to write
 * @param[in] buf   Source buffer of @p count blocks
 *
 * @retval 0 On success
 * @retval -ENODEV No medium ready, the medium was removed or changed, or @p lun is NULL
 * @retval -EINVAL The requested range is outside the medium
 * @retval -EROFS The medium is write protected
 * @retval -ETIMEDOUT The device did not answer
 * @retval -ENOMEM No transfer buffer available
 * @retval -EIO The device or the transport reported an error
 */
int usbh_msc_write(struct usbh_msc_lun *lun, uint32_t lba, uint32_t count, const uint8_t *buf);

/**
 * @brief Write the cached data of the device to the medium.
 *
 * Devices that do not support SYNCHRONIZE CACHE report success.
 *
 * @param[in] lun Logical unit handle
 *
 * @retval 0 On success
 * @retval -ENODEV No medium ready, or @p lun is NULL
 * @retval -ETIMEDOUT The device did not answer
 * @retval -EIO The device or the transport reported an error
 */
int usbh_msc_sync(struct usbh_msc_lun *lun);

/**
 * @brief Get the geometry of the medium.
 *
 * @param[in]  lun         Logical unit handle
 * @param[out] block_count Number of blocks on the medium
 * @param[out] block_size  Block size in bytes
 *
 * @retval 0 On success
 * @retval -ENODEV No medium ready, or @p lun is NULL
 * @retval -EINVAL @p block_count or @p block_size is NULL
 */
int usbh_msc_get_capacity(struct usbh_msc_lun *lun, uint32_t *block_count, uint32_t *block_size);

/**
 * @brief Tell whether the medium is ready for block access.
 *
 * @param[in] lun Logical unit handle
 *
 * @return true when the medium can be read.
 */
bool usbh_msc_is_ready(struct usbh_msc_lun *lun);

/**
 * @brief Tell whether the medium is write protected.
 *
 * @param[in] lun Logical unit handle
 *
 * @return true when the medium is ready and write protected.
 */
bool usbh_msc_is_write_protected(struct usbh_msc_lun *lun);

/**
 * @brief Get the generation of the medium.
 *
 * The generation changes every time a medium becomes ready, when a device is
 * attached and when the medium in a card reader is changed. A file system
 * mounted on a previous generation must be mounted again.
 *
 * @param[in] lun Logical unit handle
 *
 * @return Generation of the ready medium, or 0 when no medium is ready.
 */
uint32_t usbh_msc_get_generation(struct usbh_msc_lun *lun);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBH_MSC_H_ */
