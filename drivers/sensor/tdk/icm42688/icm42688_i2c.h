/*
 * Copyright (c) 2025 ZARM, University of Bremen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42688_I2C_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42688_I2C_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

/**
 * @brief perform a single I2C write to a ICM42688 register
 *
 * this functions wraps all logic necessary to write to any of the ICM42688 registers, regardless
 * of which memory bank the register belongs to.
 *
 * @param dev ICM42688 device pointer
 * @param reg address of ICM42688 register to write to
 * @param data data byte to write to register
 * @return int 0 on success, negative error code otherwise
 */
int icm42688_i2c_single_write(const struct device *dev, uint16_t reg, uint8_t data);

/**
 * @brief update a single ICM42688 register value
 *
 * this functions wraps all logic necessary to update any of the ICM42688 registers, regardless
 * of which memory bank the register belongs to.
 *
 * @param dev ICM42688 device pointer
 * @param reg address of ICM42688 register to update
 * @param mask bitmask defining which bits of the register to update
 * @param data new value to update register with, respecting the bitmask
 * @return int 0 on success, negative error code otherwise
 */
int icm42688_i2c_update_register(const struct device *dev, uint16_t reg, uint8_t mask,
				 uint8_t data);

/**
 * @brief read from one or more ICM42688 registers
 *
 * this functions wraps all logic necessary to read from any of the ICM42688 registers, regardless
 * of which memory bank the register belongs to.
 *
 * @param dev ICM42688 device pointer
 * @param reg start address of ICM42688 register(s) to read from
 * @param data pointer to byte array to read register values to
 * @param len number of bytes to read from the device
 * @return int 0 on success, negative error code otherwise
 */
int icm42688_i2c_read(const struct device *dev, uint16_t reg, uint8_t *data, size_t len);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42688_I2C_H_ */
