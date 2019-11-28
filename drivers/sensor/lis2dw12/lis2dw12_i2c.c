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
#include <drivers/i2c.h>
#include <logging/log.h>

#include "lis2dw12.h"

#ifdef DT_ST_LIS2DW12_BUS_I2C

static u16_t lis2dw12_i2c_slave_addr = DT_INST_0_ST_LIS2DW12_BASE_ADDRESS;

LOG_MODULE_DECLARE(LIS2DW12, CONFIG_SENSOR_LOG_LEVEL);

static int lis2dw12_i2c_read(struct lis2dw12_data *data, u8_t reg_addr,
				 u8_t *value, u16_t len)
{
	return i2c_burst_read(data->bus, lis2dw12_i2c_slave_addr,
			      reg_addr, value, len);
}

static int lis2dw12_i2c_write(struct lis2dw12_data *data, u8_t reg_addr,
				  u8_t *value, u16_t len)
{
	return i2c_burst_write(data->bus, lis2dw12_i2c_slave_addr,
			       reg_addr, value, len);
}

stmdev_ctx_t lis2dw12_i2c_ctx = {
	.read_reg = (stmdev_read_ptr) lis2dw12_i2c_read,
	.write_reg = (stmdev_write_ptr) lis2dw12_i2c_write,
};

int lis2dw12_i2c_init(struct device *dev)
{
	struct lis2dw12_data *data = dev->driver_data;

	data->ctx = &lis2dw12_i2c_ctx;
	data->ctx->handle = data;

	return 0;
}
#endif /* DT_ST_LIS2DW12_BUS_I2C */
