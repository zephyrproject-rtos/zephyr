/* ST Microelectronics LSM6DSO 6-axis IMU sensor driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dso.pdf
 */

#include <string.h>
#include <i2c.h>
#include <logging/log.h>

#include "lsm6dso.h"

#ifdef DT_ST_LSM6DSO_BUS_I2C

static u16_t lsm6dso_i2c_slave_addr = DT_ST_LSM6DSO_0_BASE_ADDRESS;

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_DECLARE(LSM6DSO);

static int lsm6dso_i2c_read_data(struct lsm6dso_data *data, u8_t reg_addr,
				 u8_t *value, u8_t len)
{
	return i2c_burst_read(data->bus, lsm6dso_i2c_slave_addr,
			      reg_addr, value, len);
}

static int lsm6dso_i2c_write_data(struct lsm6dso_data *data, u8_t reg_addr,
				  u8_t *value, u8_t len)
{
	return i2c_burst_write(data->bus, lsm6dso_i2c_slave_addr,
			       reg_addr, value, len);
}

static int lsm6dso_i2c_read_reg(struct lsm6dso_data *data, u8_t reg_addr,
				u8_t *value)
{
	return i2c_reg_read_byte(data->bus, lsm6dso_i2c_slave_addr,
				 reg_addr, value);
}

static int lsm6dso_i2c_write_reg(struct lsm6dso_data *data, u8_t reg_addr,
				u8_t value)
{
	return i2c_reg_write_byte(data->bus, lsm6dso_i2c_slave_addr,
				 reg_addr, value);
}

static int lsm6dso_i2c_update_reg(struct lsm6dso_data *data, u8_t reg_addr,
				  u8_t mask, u8_t value)
{
	return i2c_reg_update_byte(data->bus, lsm6dso_i2c_slave_addr,
				   reg_addr, mask,
				   value << __builtin_ctz(mask));
}

static const struct lsm6dso_tf lsm6dso_i2c_transfer_fn = {
	.read_data = lsm6dso_i2c_read_data,
	.write_data = lsm6dso_i2c_write_data,
	.read_reg  = lsm6dso_i2c_read_reg,
	.write_reg  = lsm6dso_i2c_write_reg,
	.update_reg = lsm6dso_i2c_update_reg,
};

int lsm6dso_i2c_init(struct device *dev)
{
	struct lsm6dso_data *data = dev->driver_data;

	data->hw_tf = &lsm6dso_i2c_transfer_fn;

	return 0;
}
#endif /* DT_ST_LSM6DSO_BUS_I2C */
