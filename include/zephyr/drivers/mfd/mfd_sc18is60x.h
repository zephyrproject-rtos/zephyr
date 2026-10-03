/*
 * SPDX-FileCopyrightText: Copyright tinyvision.ai
 * SPDX-FileCopyrightText: Copyright Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MFD interface for NXP SC18IS60x I2C-to-SPI bridges
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_SC18IS60X_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_SC18IS60X_H_

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Acquire the bridge lock.
 *
 * Hold this across a configure-then-transfer sequence so the shared
 * 0xF0 SPI configuration register cannot change between the two bus
 * operations. Do not call nxp_sc18is60x_transfer() while holding the
 * lock; use nxp_sc18is60x_transfer_unlocked() instead.
 *
 * @param dev SC18IS60x bridge
 *
 * @retval 0 Lock acquired
 * @retval -EWOULDBLOCK called from an ISR
 */
int nxp_sc18is60x_lock(const struct device *dev);

/**
 * Release the bridge lock.
 *
 * @param dev SC18IS60x bridge
 */
void nxp_sc18is60x_unlock(const struct device *dev);

/**
 * Transfer a complete command; caller already holds the bridge lock.
 *
 * @a tx_data is the full payload (function ID or command byte already
 * included). After a write the chip NACKs until the command finishes;
 * this routine waits (INT pin, or polls the I2C address until ACK).
 *
 * @param dev SC18IS60x bridge
 * @param tx_data Full write buffer, or NULL for read-only
 * @param tx_len Number of bytes in @a tx_data
 * @param rx_data Buffer for a following read, or NULL
 * @param rx_len Number of bytes to read into @a rx_data
 *
 * @retval 0 Transfer success
 * @retval -EIO bus error or ready-wait timed out
 */
int nxp_sc18is60x_transfer_unlocked(const struct device *dev, const uint8_t *tx_data,
				    uint16_t tx_len, uint8_t *rx_data, uint16_t rx_len);

/**
 * Transfer a complete command to or from the bridge
 *
 * Acquires the bridge lock for the duration of the transfer. Use
 * nxp_sc18is60x_lock() / nxp_sc18is60x_transfer_unlocked() when several
 * commands must not be interleaved.
 *
 * @param dev SC18IS60x bridge
 * @param tx_data Full write buffer, or NULL for read-only
 * @param tx_len Number of bytes in @a tx_data
 * @param rx_data Buffer for a following read, or NULL
 * @param rx_len Number of bytes to read into @a rx_data
 *
 * @retval 0 Transfer success
 * @retval -EWOULDBLOCK called from an ISR
 * @retval -EIO bus error or ready-wait timed out
 */
int nxp_sc18is60x_transfer(const struct device *dev, const uint8_t *tx_data, uint16_t tx_len,
			   uint8_t *rx_data, uint16_t rx_len);

/**
 * Write the SPI configuration register (command 0xF0) if @a cfg_byte
 * differs from the last value programmed on this bridge.
 *
 * Caller must hold nxp_sc18is60x_lock(). The cache is per-bridge, not
 * per-child, because the register is shared.
 *
 * @param dev SC18IS60x bridge
 * @param cfg_byte Value for the 0xF0 configuration register
 *
 * @retval 0 Register already matches, or write succeeded
 * @retval -EIO bus error or ready-wait timed out
 */
int nxp_sc18is60x_configure_spi(const struct device *dev, uint8_t cfg_byte);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_SC18IS60X_H_ */
