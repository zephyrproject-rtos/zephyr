/*
 * Copyright (c) 2026 Microchip Technologies Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_MCHP_XEC_I2C_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_MCHP_XEC_I2C_H_

/**
 * @file
 * @brief Microchip XEC I2C v3 byte-mode (BM) state-capture API.
 */

#include <zephyr/device.h>

#ifdef CONFIG_I2C_MCHP_XEC_V3_BM_STATE_CAPTURE
/**
 * @brief Clear the byte-mode state-capture buffer for an I2C port.
 *
 * @param port_dev I2C port device whose parent controller's capture buffer
 *                 is cleared.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p port_dev is NULL.
 * @retval -EBUSY if the controller is not idle.
 */
int mchp_xec_i2c_bm_clear_capture(const struct device *port_dev);

/**
 * @brief Copy the byte-mode state-capture buffer for an I2C port.
 *
 * @param port_dev I2C port device whose parent controller's capture buffer
 *                 is copied.
 * @param capdest Destination buffer.
 * @param capdest_size Size of @p capdest, in bytes.
 *
 * @retval 0 on success (including when @p capdest_size is 0).
 * @retval -EINVAL if @p port_dev or @p capdest is NULL.
 * @retval -EBUSY if the controller is not idle.
 */
int mchp_xec_i2c_bm_copy_capture(const struct device *port_dev, uint8_t *capdest,
				 size_t capdest_size);
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_MCHP_XEC_I2C_H_ */
