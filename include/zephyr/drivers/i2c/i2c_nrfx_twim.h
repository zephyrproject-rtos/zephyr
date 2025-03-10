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

/** @brief Type of callback that finishes an asynchronous transfer.
 *
 * @param  dev  Pointer to the device structure for an I2C controller driver.
 * @param  res  Result of a transfer.
 * @param  ctx  Opaque pointer for providing a context passed to
 *              @ref i2c_nrfx_twim_async_transfer_begin .
 */
typedef void (*i2c_nrfx_twim_async_transfer_handler_t)(const struct device *dev, int res,
						       void *ctx);

/** @brief Begins an asynchronous write transfer on a I2C bus.
 *
 * @note This function may be called only when the exclusive access to the bus is granted.
 *       See @ref i2c_nrfx_twim_exclusive_access_acquire .
 *
 * @param dev      Pointer to the device structure for an I2C controller
 *                 driver configured in controller mode.
 * @param msg      Message to transfer.
 * @param addr     Address of the I2C target device.
 * @param handler  Pointer to a callback function that will be called from an I2C
 *                 interrupt handler that notifies end of the transfer.
 * @param ctx      Opaque pointer passed to the @p handler for passing context.
 *
 * @return @c 0 on success, the @p handler will be called in future.
 *         Other values indicate an error, the @p handler will not be called.
 */
int i2c_nrfx_twim_async_transfer_begin(const struct device *dev, struct i2c_msg *msg, uint16_t addr,
				       i2c_nrfx_twim_async_transfer_handler_t handler, void *ctx);

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_NRFX_TWIM_H */
