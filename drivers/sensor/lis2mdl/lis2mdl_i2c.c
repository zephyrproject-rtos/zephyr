/* ST Microelectronics LIS2MDL 3-axis magnetometer sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2mdl.pdf
 */

#define DT_DRV_COMPAT st_lis2mdl

#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "lis2mdl.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_DECLARE(LIS2MDL);

static int lis2mdl_i2c_read(struct lis2mdl_data *data, uint8_t reg_addr,
			    uint8_t *value, uint16_t len)
{
	const struct lis2mdl_config *cfg = data->dev->config;

	return i2c_burst_read(data->bus, cfg->i2c_slv_addr,
			      reg_addr, value, len);
}

static int lis2mdl_i2c_write(struct lis2mdl_data *data, uint8_t reg_addr,
			     uint8_t *value, uint16_t len)
{
	const struct lis2mdl_config *cfg = data->dev->config;

	return i2c_burst_write(data->bus, cfg->i2c_slv_addr,
			       reg_addr, value, len);
}

int lis2mdl_i2c_init(const struct device *dev)
{
	struct lis2mdl_data *data = dev->data;

	data->ctx_i2c.read_reg = (stmdev_read_ptr) lis2mdl_i2c_read;
	data->ctx_i2c.write_reg = (stmdev_write_ptr) lis2mdl_i2c_write;

	data->ctx = &data->ctx_i2c;
	data->ctx->handle = data;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
