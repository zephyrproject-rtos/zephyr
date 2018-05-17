/*
 * STMicroelectronics LIS2DW12 I2C interface
 *
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <i2c.h>
#include "lis2dw12.h"

/**
 * Read single register from sensor
 */
static int lis2dw12_i2c_read8(struct lis2dw12_data *lis2dw12,
			       u8_t reg_addr, u8_t *value)
{
	return i2c_reg_read_byte(lis2dw12->bus, CONFIG_LIS2DW12_I2C_ADDR,
				 reg_addr, value);
}

/**
 * Write single register to sensor
 */
static int lis2dw12_i2c_write8(struct lis2dw12_data *lis2dw12,
				u8_t reg_addr, u8_t value)
{
	return i2c_reg_write_byte(lis2dw12->bus, CONFIG_LIS2DW12_I2C_ADDR,
				  reg_addr, value);
}

/**
 * Read n bytes from sensor
 */
static int lis2dw12_i2c_read_n(struct lis2dw12_data *lis2dw12,
				u8_t start_addr, u8_t *buf, u8_t num_bytes)
{
	return i2c_burst_read(lis2dw12->bus, CONFIG_LIS2DW12_I2C_ADDR,
			      start_addr, buf, num_bytes);
}

/**
 * Write to sensor register with mask
 */
static int lis2dw12_write_i2c_reg_mask(struct lis2dw12_data *lis2dw12,
					u8_t reg_addr, u8_t mask, u8_t data)
{
	return i2c_reg_update_byte(lis2dw12->bus, CONFIG_LIS2DW12_I2C_ADDR,
				   reg_addr, mask, data << __builtin_ctz(mask));
}

static const struct lis2dw12_tf lis2dw12_i2c_tf = {
	.write_reg = lis2dw12_i2c_write8,
	.read_reg  = lis2dw12_i2c_read8,
	.read_data = lis2dw12_i2c_read_n,
	.update_reg = lis2dw12_write_i2c_reg_mask,
};

int lis2dw12_i2c_init(struct device *dev)
{
	struct lis2dw12_data *data = dev->driver_data;

	data->hw_tf = &lis2dw12_i2c_tf;

	return 0;
}
