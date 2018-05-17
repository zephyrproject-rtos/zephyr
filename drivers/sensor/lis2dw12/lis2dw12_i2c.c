/* ST Microelectronics LIS2DW12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dw12.pdf
 */

#include <string.h>
#include <i2c.h>
#include <logging/log.h>

#include "lis2dw12.h"

#ifdef DT_ST_LIS2DW12_BUS_I2C

static u16_t lis2dw12_i2c_slave_addr = DT_ST_LIS2DW12_0_BASE_ADDRESS;

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_DECLARE(LIS2DW12);

static int lis2dw12_i2c_read_data(struct lis2dw12_data *data, u8_t reg_addr,
				 u8_t *value, u8_t len)
{
	return i2c_burst_read(data->bus, lis2dw12_i2c_slave_addr,
			      reg_addr, value, len);
}

static int lis2dw12_i2c_write_data(struct lis2dw12_data *data, u8_t reg_addr,
				  u8_t *value, u8_t len)
{
	return i2c_burst_write(data->bus, lis2dw12_i2c_slave_addr,
			       reg_addr, value, len);
}

static int lis2dw12_i2c_read_reg(struct lis2dw12_data *data, u8_t reg_addr,
				u8_t *value)
{
	return i2c_reg_read_byte(data->bus, lis2dw12_i2c_slave_addr,
				 reg_addr, value);
}

static int lis2dw12_i2c_write_reg(struct lis2dw12_data *data, u8_t reg_addr,
				u8_t value)
{
	return i2c_reg_write_byte(data->bus, lis2dw12_i2c_slave_addr,
				 reg_addr, value);
}

static int lis2dw12_i2c_update_reg(struct lis2dw12_data *data, u8_t reg_addr,
				  u8_t mask, u8_t value)
{
	return i2c_reg_update_byte(data->bus, lis2dw12_i2c_slave_addr,
				   reg_addr, mask,
				   value << __builtin_ctz(mask));
}

static const struct lis2dw12_tf lis2dw12_i2c_transfer_fn = {
	.read_data = lis2dw12_i2c_read_data,
	.write_data = lis2dw12_i2c_write_data,
	.read_reg  = lis2dw12_i2c_read_reg,
	.write_reg  = lis2dw12_i2c_write_reg,
	.update_reg = lis2dw12_i2c_update_reg,
};

int lis2dw12_i2c_init(struct device *dev)
{
	struct lis2dw12_data *data = dev->driver_data;

	data->hw_tf = &lis2dw12_i2c_transfer_fn;

	return 0;
}
#endif /* DT_ST_LIS2DW12_BUS_I2C */
