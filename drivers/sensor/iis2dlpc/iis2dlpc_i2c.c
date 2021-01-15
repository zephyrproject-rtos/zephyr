/* ST Microelectronics IIS2DLPC 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2dlpc.pdf
 */

#define DT_DRV_COMPAT st_iis2dlpc

#include <string.h>
#include <logging/log.h>

#include "iis2dlpc.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(IIS2DLPC, CONFIG_SENSOR_LOG_LEVEL);

static int iis2dlpc_i2c_read(struct iis2dlpc_data *data, uint8_t reg_addr,
				 uint8_t *value, uint16_t len)
{
	const struct device *dev = data->dev;
	const struct iis2dlpc_dev_config *cfg = dev->config;

	return i2c_burst_read(data->bus, cfg->bus_cfg.i2c_slv_addr,
			      reg_addr, value, len);
}

static int iis2dlpc_i2c_write(struct iis2dlpc_data *data, uint8_t reg_addr,
				  uint8_t *value, uint16_t len)
{
	const struct device *dev = data->dev;
	const struct iis2dlpc_dev_config *cfg = dev->config;

	return i2c_burst_write(data->bus, cfg->bus_cfg.i2c_slv_addr,
			       reg_addr, value, len);
}

int iis2dlpc_i2c_init(const struct device *dev)
{
	struct iis2dlpc_data *data = dev->data;

	data->ctx_i2c.read_reg = (stmdev_read_ptr) iis2dlpc_i2c_read,
	data->ctx_i2c.write_reg = (stmdev_write_ptr) iis2dlpc_i2c_write,

	data->ctx = &data->ctx_i2c;
	data->ctx->handle = data;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
