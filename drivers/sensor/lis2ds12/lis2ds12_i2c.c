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

static int lis2ds12_i2c_read(struct lis2ds12_data *data, uint8_t reg_addr,
				 uint8_t *value, uint16_t len)
{
	return i2c_burst_read(data->comm_master, lis2ds12_i2c_slave_addr,
			      reg_addr, value, len);
}

static int lis2ds12_i2c_write(struct lis2ds12_data *data, uint8_t reg_addr,
				  uint8_t *value, uint16_t len)
{
	return i2c_burst_write(data->comm_master, lis2ds12_i2c_slave_addr,
			       reg_addr, value, len);
}

stmdev_ctx_t lis2ds12_i2c_ctx = {
	.read_reg = (stmdev_read_ptr) lis2ds12_i2c_read,
	.write_reg = (stmdev_write_ptr) lis2ds12_i2c_write,
};

int lis2ds12_i2c_init(const struct device *dev)
{
	struct lis2ds12_data *data = dev->data;

	data->ctx = &lis2ds12_i2c_ctx;
	data->ctx->handle = data;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
