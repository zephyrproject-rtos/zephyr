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

/** Opaque handle to an attached mass storage logical unit. */
struct usbh_msc_lun;

/**
 * @brief Get the handle of an attached mass storage logical unit.
 *
 * One index is one attached device, up to @kconfig{CONFIG_USBH_MSC_LUN_COUNT}
 * of them. Indices are handed out in the order the devices are probed.
 *
 * The handle stays valid only while the device is attached, so callers should
 * re-check usbh_msc_is_ready() instead of caching it across a detach.
 *
 * An index is reused once the device that held it is gone, and a new device
 * can take it. usbh_msc_is_ready() reports the same for both, so it cannot
 * tell a medium that stayed in place from one that was swapped. A file system
 * mounted on an index has to be unmounted when the unit stops being ready,
 * not remounted on the assumption that the medium is unchanged.
 *
 * @param[in] idx Logical unit index
 * @return Handle, or NULL when no unit is attached at this index.
 */
struct usbh_msc_lun *usbh_msc_lun_get(unsigned int idx);

/**
 * @brief Read blocks from the medium.
 *
 * @param[in]  lun   Logical unit handle
 * @param[in]  lba   First logical block to read
 * @param[in]  count Number of blocks to read
 * @param[out] buf   Destination buffer of @p count blocks
 * @retval 0 On success
 * @retval -ENODEV No medium ready, or @p lun is NULL
 * @retval -EINVAL The requested range is outside the medium
 * @retval -EIO The medium rejected the command
 */
int usbh_msc_read(struct usbh_msc_lun *lun, uint32_t lba, uint32_t count, uint8_t *buf);

/**
 * @brief Write blocks to the medium.
 *
 * @param[in] lun   Logical unit handle
 * @param[in] lba   First logical block to write
 * @param[in] count Number of blocks to write
 * @param[in] buf   Source buffer of @p count blocks
 * @retval 0 On success
 * @retval -ENODEV No medium ready, or @p lun is NULL
 * @retval -EINVAL The requested range is outside the medium
 * @retval -EIO The medium rejected the command
 */
int usbh_msc_write(struct usbh_msc_lun *lun, uint32_t lba, uint32_t count, const uint8_t *buf);

/**
 * @brief Get the geometry of the medium.
 *
 * @param[in]  lun         Logical unit handle
 * @param[out] block_count Number of blocks on the medium
 * @param[out] block_size  Block size in bytes
 * @retval 0 On success
 * @retval -ENODEV No medium ready, or @p lun is NULL
 * @retval -EINVAL @p block_count or @p block_size is NULL
 */
int usbh_msc_get_capacity(struct usbh_msc_lun *lun, uint32_t *block_count, uint32_t *block_size);

/**
 * @brief Tell whether the medium is attached and ready for block access.
 *
 * @param[in] lun Logical unit handle
 * @return true when the medium can be read and written.
 */
bool usbh_msc_is_ready(struct usbh_msc_lun *lun);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBH_MSC_H_ */
