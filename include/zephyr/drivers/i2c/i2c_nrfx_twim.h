/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_NRFX_TWIM_H
#define ZEPHYR_INCLUDE_DRIVERS_I2C_NRFX_TWIM_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

/** @brief Acquires exclusive access to the i2c bus controller.
 *
 * @param dev      Pointer to the device structure for an I2C controller
 *                 driver configured in controller mode.
 * @param timeout  Timeout for waiting to acquire exclusive access.
 *
 * @retval 0       If successful.
 * @retval -EBUSY  Returned without waiting.
 * @retval -EAGAIN Waiting period timed out,
 *                 or the underlying semaphore was reset during the waiting period.
 */
int i2c_nrfx_twim_exclusive_access_acquire(const struct device *dev, k_timeout_t timeout);

/** @brief Releases exclusive access to the i2c bus controller.
 *
 * @param dev      Pointer to the device structure for an I2C controller
 *                 driver on which @ref i2c_nrfx_twim_exclusive_access_acquire
 *                 has been successfully called.
 */
void i2c_nrfx_twim_exclusive_access_release(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_NRFX_TWIM_H */
