/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mfd_interface_atecc608a
 * @brief Header file for the Microchip ATECC608A MFD driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_ATECC608A_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_ATECC608A_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mfd_interface_atecc608a MFD ATECC608A Interface
 * @ingroup mfd_interfaces
 * @brief Microchip ATECC608A CryptoAuthentication interface.
 * @since 4.6
 * @version 0.1.0
 *
 * The parent driver owns wake, command framing, CRC and the shared bus lock.
 * Sibling drivers call @ref mfd_atecc608a_execute().
 * @{
 */

/**
 * @brief Run one ATECC608A command and return the response payload.
 *
 * Wakes the device, sends a command group, waits for completion, verifies
 * the CRC, and copies the response data (without count and CRC) to @p rx.
 *
 * This helper is for sibling child drivers. Applications should use the
 * child class APIs (for example entropy_get_entropy()).
 *
 * @param dev Parent MFD device.
 * @param opcode Command opcode from the device datasheet.
 * @param param1 Command Param1.
 * @param param2 Command Param2.
 * @param tx Optional command data, or NULL.
 * @param tx_len Length of @p tx.
 * @param rx Buffer for response data, or NULL when no payload is expected.
 * @param rx_len Size of @p rx. When non-zero, the response payload length
 *               must match this value.
 * @param min_wait Minimum time to wait after the command before the first read.
 * @param max_wait Time allowed for the device to finish the command.
 *
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 * @retval -EIO bus, CRC, or device status error
 * @retval -ETIMEDOUT command did not complete in time
 */
int mfd_atecc608a_execute(const struct device *dev, uint8_t opcode, uint8_t param1,
			  uint16_t param2, const uint8_t *tx, size_t tx_len, uint8_t *rx,
			  size_t rx_len, k_timeout_t min_wait, k_timeout_t max_wait);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_ATECC608A_H_ */
