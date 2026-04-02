/*
 * Copyright (c) 2026 Maximilian Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup qemu_fwcfg_interface
 * @brief Header for the QEMU firmware configuration (fw_cfg) driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_QEMU_FWCFG_H_
#define ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_QEMU_FWCFG_H_

/**
 * @defgroup qemu_fwcfg_interface QEMU fw_cfg
 * @since 4.4.0
 * @version 0.1.0
 * @ingroup io_interfaces
 * @brief Interface for the QEMU firmware configuration (fw_cfg) driver
 * @{
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

/** @cond INTERNAL_HIDDEN */
/** fw_cfg signature selector key */
#define FW_CFG_SIGNATURE 0x0000
/** fw_cfg feature identifier selector key */
#define FW_CFG_ID        0x0001
/** fw_cfg file directory selector key */
#define FW_CFG_FILE_DIR  0x0019

/** fw_cfg feature bit: traditional interface available */
#define FW_CFG_ID_F_TRADITIONAL BIT(0)
/** fw_cfg feature bit: DMA interface available */
#define FW_CFG_ID_F_DMA         BIT(1)
/** @endcond */

/**
 * @brief Read data from an fw_cfg item
 *
 * @param dev fw_cfg device
 * @param key selector key for the fw_cfg item
 * @param buf destination buffer
 * @param len number of bytes to read
 *
 * @retval 0 if successful
 * @retval -EIO if the device reports an I/O error
 * @retval -ETIMEDOUT if DMA transfer does not complete in time
 */
int qemu_fwcfg_read_item(const struct device *dev, uint16_t key, void *buf, size_t len);

/**
 * @brief Write data to an fw_cfg item
 *
 * Writes are supported only when the fw_cfg DMA feature is available
 *
 * @param dev fw_cfg device
 * @param key selector key for the fw_cfg item
 * @param buf source buffer
 * @param len number of bytes to write
 *
 * @retval 0 if successful
 * @retval -ENOTSUP if DMA write is not supported
 * @retval -EIO if the device reports an I/O error
 * @retval -ETIMEDOUT if DMA transfer does not complete in time
 */
int qemu_fwcfg_write_item(const struct device *dev, uint16_t key, const void *buf, size_t len);

/**
 * @brief Get fw_cfg feature bits reported by the device
 *
 * @param dev fw_cfg device
 * @param features pointer to store feature bits
 *
 * @retval 0 if successful
 */
int qemu_fwcfg_get_features(const struct device *dev, uint32_t *features);

/**
 * @brief Look up a file entry in fw_cfg file directory
 *
 * @param dev fw_cfg device
 * @param file file name to search for
 * @param select pointer to store selector key for the file
 * @param size pointer to store file size in bytes
 *
 * @retval 0 if successful
 * @retval -ENOENT if the file does not exist
 */
int qemu_fwcfg_find_file(const struct device *dev, const char *file, uint16_t *select,
			 uint32_t *size);

/**
 * @brief Check whether fw_cfg DMA feature is available
 *
 * @param dev fw_cfg device
 *
 * @retval true if DMA feature is available
 * @retval false otherwise
 */
bool qemu_fwcfg_dma_supported(const struct device *dev);

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_QEMU_FWCFG_H_ */
