/* ST Microelectronics LPS22HH pressure and temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22hh.pdf
 */

#include <string.h>
#include <i2c.h>
#include <logging/log.h>

#include "lps22hh.h"

#ifdef DT_ST_LPS22HH_BUS_I2C

static u16_t lps22hh_i2c_slave_addr = DT_ST_LPS22HH_0_BASE_ADDRESS;

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_DECLARE(LPS22HH);

static int lps22hh_i2c_read_data(struct lps22hh_data *data, u8_t reg_addr,
				 u8_t *value, u8_t len)
{
	return i2c_burst_read(data->bus, lps22hh_i2c_slave_addr,
			      reg_addr, value, len);
}

static int lps22hh_i2c_write_data(struct lps22hh_data *data, u8_t reg_addr,
				  u8_t *value, u8_t len)
{
	return i2c_burst_write(data->bus, lps22hh_i2c_slave_addr,
			       reg_addr, value, len);
}

static int lps22hh_i2c_read_reg(struct lps22hh_data *data, u8_t reg_addr,
				u8_t *value)
{
	return i2c_reg_read_byte(data->bus, lps22hh_i2c_slave_addr,
				 reg_addr, value);
}

static int lps22hh_i2c_write_reg(struct lps22hh_data *data, u8_t reg_addr,
				u8_t value)
{
	return i2c_reg_write_byte(data->bus, lps22hh_i2c_slave_addr,
				 reg_addr, value);
}

static int lps22hh_i2c_update_reg(struct lps22hh_data *data, u8_t reg_addr,
				  u8_t mask, u8_t value)
{
	return i2c_reg_update_byte(data->bus, lps22hh_i2c_slave_addr,
				   reg_addr, mask,
				   value << __builtin_ctz(mask));
}

static const struct lps22hh_tf lps22hh_i2c_transfer_fn = {
	.read_data = lps22hh_i2c_read_data,
	.write_data = lps22hh_i2c_write_data,
	.read_reg  = lps22hh_i2c_read_reg,
	.write_reg  = lps22hh_i2c_write_reg,
	.update_reg = lps22hh_i2c_update_reg,
};

int lps22hh_i2c_init(struct device *dev)
{
	struct lps22hh_data *data = dev->driver_data;

	data->hw_tf = &lps22hh_i2c_transfer_fn;

	return 0;
}
#endif /* DT_ST_LPS22HH_BUS_I2C */
