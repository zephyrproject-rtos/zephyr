/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_MCHP_XEC_I2C_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_MCHP_XEC_I2C_H_

#include <zephyr/device.h>

/** @brief Maximum number of I2C ports supported by Microchip XEC I2C controllers
 *
 * Current Microchip XEC I2C hardware can support up to 16 ports and implement
 * a hardware port mux field in the I2C controller's configuration register.
 * The port selection exists only in the Microchip XEC V3 driver and must be
 * enabled by Kconfig item I2C_XEC_PORT_MUX.
 * We clain unused bits[19:16] of the I2C configuration word.
 */
#define MCHP_I2C_NUM_PORTS (16)

/** @brief Bit mask representing maximum number of ports */
#define MCHP_I2C_PORT_MSK 0xf

/** @brief Get I2C controller's port configuration
 *
 * @param dev Pointer to I2C controller's device structure
 * @param port Pointer to port variable to fill in with port number
 * @return 0 on success else -EBUSY, -EINVAL, or -EIO.
 */
int i2c_xec_v3_get_port(const struct device *dev, uint8_t *port);

/** @brief Configure I2C controller for specified port and frequency
 *
 * @param dev pointer to I2C controller's device structure
 * @param config I2C configuration 32-bit word containing encoded frequency
 * @param port I2C hardware port number
 * @return 0 on success else -EBUSY, -EINVAL, or -EIO.
 */
int i2c_xec_v3_config(const struct device *dev, uint32_t config, uint8_t port);

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_MCHP_XEC_I2C_H_ */
