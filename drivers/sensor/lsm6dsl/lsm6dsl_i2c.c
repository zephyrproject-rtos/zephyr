/* lsm6dsl_i2c.c - I2C routines for LSM6DSL driver
 */

/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <i2c.h>
#include "lsm6dsl.h"

static u16_t lsm6dsl_i2c_slave_addr = CONFIG_LSM6DSL_I2C_ADDR;

static int lsm6dsl_i2c_read_data(struct lsm6dsl_data *data, u8_t reg_addr,
				 u8_t *value, u8_t len)
{
	return i2c_burst_read(data->comm_master, lsm6dsl_i2c_slave_addr,
			      reg_addr, value, len);
}

static int lsm6dsl_i2c_write_data(struct lsm6dsl_data *data, u8_t reg_addr,
				  u8_t *value, u8_t len)
{
	return i2c_burst_write(data->comm_master, lsm6dsl_i2c_slave_addr,
			       reg_addr, value, len);
}

static int lsm6dsl_i2c_read_reg(struct lsm6dsl_data *data, u8_t reg_addr,
				u8_t *value)
{
	return i2c_reg_read_byte(data->comm_master, lsm6dsl_i2c_slave_addr,
				 reg_addr, value);
}

static int lsm6dsl_i2c_update_reg(struct lsm6dsl_data *data, u8_t reg_addr,
				  u8_t mask, u8_t value)
{
	return i2c_reg_update_byte(data->comm_master, lsm6dsl_i2c_slave_addr,
				   reg_addr, mask, value);
}

static const struct lsm6dsl_transfer_function lsm6dsl_i2c_transfer_fn = {
	.read_data = lsm6dsl_i2c_read_data,
	.write_data = lsm6dsl_i2c_write_data,
	.read_reg  = lsm6dsl_i2c_read_reg,
	.update_reg = lsm6dsl_i2c_update_reg,
};

int lsm6dsl_i2c_init(struct device *dev)
{
	struct lsm6dsl_data *data = dev->driver_data;

	data->hw_tf = &lsm6dsl_i2c_transfer_fn;

	return 0;
}
