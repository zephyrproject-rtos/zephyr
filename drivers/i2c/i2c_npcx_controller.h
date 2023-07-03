/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2C_I2C_NPCX_CONTROLLER_H_
#define ZEPHYR_DRIVERS_I2C_I2C_NPCX_CONTROLLER_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Lock the mutex of npcx i2c controller.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 */
void npcx_i2c_ctrl_mutex_lock(const struct device *i2c_dev);

/**
 * @brief Unlock the mutex of npcx i2c controller.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 */
void npcx_i2c_ctrl_mutex_unlock(const struct device *i2c_dev);

/**
 * @brief Configure operation of a npcx i2c controller.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 * @param dev_config Bit-packed 32-bit value to the device runtime configuration
 * for the I2C controller.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 * @retval -ERANGE Out of supported i2c frequency.
 */
int npcx_i2c_ctrl_configure(const struct device *i2c_dev, uint32_t dev_config);

/**
 * @brief Get I2C controller speed.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 * @param speed Pointer to store the I2C speed.
 *
 * @retval 0 If successful.
 * @retval -ERANGE Stored I2C frequency out of supported range.
 * @retval -EIO    Controller is not configured.
 */
int npcx_i2c_ctrl_get_speed(const struct device *i2c_dev, uint32_t *speed);

/**
 * @brief Perform data transfer to via npcx i2c controller.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 * @param msgs Array of messages to transfer.
 * @param num_msgs Number of messages to transfer.
 * @param addr Address of the I2C target device.
 * @param port Port index of selected i2c port.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ENXIO No slave address match.
 * @retval -ETIMEDOUT Timeout occurred for a i2c transaction.
 */
int npcx_i2c_ctrl_transfer(const struct device *i2c_dev, struct i2c_msg *msgs,
			      uint8_t num_msgs, uint16_t addr, uint8_t port);

/**
 * @brief Toggle the SCL to generate maxmium 9 clocks until the target release
 * the SDA line and send a STOP condition.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 *
 * @retval 0 If successful.
 * @retval -EBUSY fail to recover the bus.
 */
int npcx_i2c_ctrl_recover_bus(const struct device *dev);

/**
 * @brief Registers the provided config as Target device of a npcx i2c controller.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 * @param target_cfg Config struct used by the i2c target driver
 * @param port Port index of selected i2c port.
 *
 * @retval 0 Is successful
 * @retval -EBUSY If i2c transaction is proceeding.
 */
int npcx_i2c_ctrl_target_register(const struct device *i2c_dev,
				 struct i2c_target_config *target_cfg, uint8_t port);

/**
 * @brief Unregisters the provided config as Target device of a npcx i2c controller.
 *
 * @param i2c_dev Pointer to the device structure for i2c controller instance.
 * @param target_cfg Config struct used by the i2c target driver
 *
 * @retval 0 Is successful
 * @retval -EBUSY If i2c transaction is proceeding.
 * @retval -EINVAL If parameters are invalid
 */
int npcx_i2c_ctrl_target_unregister(const struct device *i2c_dev,
				   struct i2c_target_config *target_cfg);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2C_I2C_NPCX_CONTROLLER_H_ */
