/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM45686_I2C_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM45686_I2C_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

/**
@brief perform a single I2C write to a ICM45686 mreg register
 *
 * this functions wraps all logic necessary to write to any of the ICM45686 mreg registers,
 * regardless of which memory bank the register belongs to.
 *
 * @param bus I2C bus pointer
 * @param reg address of ICM45686 register to write to
 * @param data data byte to write to register
 * @return int 0 on success, negative error code otherwise
 */
int icm45686_write_mreg(const struct i2c_dt_spec *bus, uint16_t reg, uint8_t data);

/**
@brief update a single ICM45686 mreg register value
 *
 * this functions wraps all logic necessary to update any of the ICM45686 registers, regardless
 * of which memory bank the register belongs to.
 *
 * @param bus I2C bus pointer
 * @param reg address of ICM45686 register to update
 * @param mask bitmask defining which bits of the register to update
 * @param data new value to update register with, respecting the bitmask
 * @return int 0 on success, negative error code otherwise
 */
int icm45686_update_mreg(const struct i2c_dt_spec *bus, uint16_t reg, uint8_t mask, uint8_t data);

/**
@brief read from one or more ICM45686 mreg registers
 *
 * this functions wraps all logic necessary to read from any of the ICM45686 mreg registers,
 * regardless of which memory bank the register belongs to.
 *
 * @param bus I2C bus pointer
 * @param reg start address of ICM45686 register(s) to read from
 * @param data pointer to byte array to read register values to
 * @param len number of bytes to read from the device
 * @return int 0 on success, negative error code otherwise
 */
int icm45686_read_mreg(const struct i2c_dt_spec *bus, uint16_t reg, uint8_t *data, size_t len);

/**
@brief perform a single I2C write to a ICM45686 register
 *
 * this functions wraps all logic necessary to write to any of the ICM45686 registers, regardless
 * of which memory bank the register belongs to.
 *
 * @param bus I2C bus pointer
 * @param reg address of ICM45686 register to write to
 * @param data data byte to write to register
 * @return int 0 on success, negative error code otherwise
 */
int icm45686_single_write(const struct i2c_dt_spec *bus, uint16_t reg, uint8_t data);

/**
@brief update a single ICM45686 register value
 *
 * this functions wraps all logic necessary to update any of the ICM45686 registers, regardless
 * of which memory bank the register belongs to.
 *
 * @param bus I2C bus pointer
 * @param reg address of ICM45686 register to update
 * @param mask bitmask defining which bits of the register to update
 * @param data new value to update register with, respecting the bitmask
 * @return int 0 on success, negative error code otherwise
 */
int icm45686_update_register(const struct i2c_dt_spec *bus, uint16_t reg, uint8_t mask,
			     uint8_t data);

/**
@brief read from one or more ICM45686 registers
 *
 * this functions wraps all logic necessary to read from any of the ICM45686 registers, regardless
 * of which memory bank the register belongs to.
 *
 * @param bus I2C bus pointer
 * @param reg start address of ICM45686 register(s) to read from
 * @param data pointer to byte array to read register values to
 * @param len number of bytes to read from the device
 * @return int 0 on success, negative error code otherwise
 */
int icm45686_read(const struct i2c_dt_spec *bus, uint16_t reg, uint8_t *data, size_t len);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM45686_I2C_H_ */
