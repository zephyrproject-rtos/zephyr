/* ST Microelectronics LIS2DS12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2ds12.pdf
 */

#define DT_DRV_COMPAT st_lis2ds12

#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "lis2ds12.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

static uint16_t lis2ds12_i2c_slave_addr = DT_INST_REG_ADDR(0);

LOG_MODULE_DECLARE(LIS2DS12, CONFIG_SENSOR_LOG_LEVEL);

static int lis2ds12_i2c_read_data(struct lis2ds12_data *data, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	return i2c_burst_read(data->comm_master, lis2ds12_i2c_slave_addr,
			      reg_addr, value, len);
}

static int lis2ds12_i2c_write_data(struct lis2ds12_data *data, uint8_t reg_addr,
				  uint8_t *value, uint8_t len)
{
	return i2c_burst_write(data->comm_master, lis2ds12_i2c_slave_addr,
			       reg_addr, value, len);
}

static int lis2ds12_i2c_read_reg(struct lis2ds12_data *data, uint8_t reg_addr,
				uint8_t *value)
{
	return i2c_reg_read_byte(data->comm_master, lis2ds12_i2c_slave_addr,
				 reg_addr, value);
}

static int lis2ds12_i2c_write_reg(struct lis2ds12_data *data, uint8_t reg_addr,
				uint8_t value)
{
	return i2c_reg_write_byte(data->comm_master, lis2ds12_i2c_slave_addr,
				 reg_addr, value);
}

static int lis2ds12_i2c_update_reg(struct lis2ds12_data *data, uint8_t reg_addr,
				  uint8_t mask, uint8_t value)
{
	return i2c_reg_update_byte(data->comm_master, lis2ds12_i2c_slave_addr,
				   reg_addr, mask, value);
}

static const struct lis2ds12_transfer_function lis2ds12_i2c_transfer_fn = {
	.read_data = lis2ds12_i2c_read_data,
	.write_data = lis2ds12_i2c_write_data,
	.read_reg  = lis2ds12_i2c_read_reg,
	.write_reg  = lis2ds12_i2c_write_reg,
	.update_reg = lis2ds12_i2c_update_reg,
};

int lis2ds12_i2c_init(const struct device *dev)
{
	struct lis2ds12_data *data = dev->data;

	data->hw_tf = &lis2ds12_i2c_transfer_fn;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
