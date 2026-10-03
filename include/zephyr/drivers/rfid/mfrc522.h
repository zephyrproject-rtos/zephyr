/*
 * Copyright (c) 2026 Luke Bugbee <lbugbee@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RFID_MFRC522_H_
#define ZEPHYR_DRIVERS_RFID_MFRC522_H_

#include <zephyr/kernel.h>

/**
 * @brief Turn on/off RF field of RFID reader device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param enable True to enable RF power, false to disable.
 *
 * @return 0 on success, negative on error.
 */
int mfrc522_enable(const struct device *dev, bool enable);

/**
 * @brief Send/receive message between RFID reader device and target.
 *
 * @note This function is synchronous.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param tx_buf Buffer for data transmitted from NFC reader to target.
 * @param tx_len Number of bytes to transmit.
 * @param rx_buf Buffer for data received from target
 * @param rx_size Size of rx buffer.
 * @param valid_bits Number of valid bits of last byte transmitted.
 *
 * @return Number of bytes received on success, negative on error.
 * @retval -ENOBUFS Received message overflows rx_buf.
 * @retval -ETIMEDOUT No target device response.
 * @retval -EBADMSG Invalid message received.
 */
int mfrc522_transceive(const struct device *dev, uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf,
		       size_t rx_size, uint8_t valid_bits);

#endif /* ZEPHYR_DRIVERS_RFID_MFRC522_H_ */
