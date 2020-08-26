/* ST Microelectronics ISM330DHCX 6-axis IMU sensor driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/ism330dhcx.pdf
 */

#define DT_DRV_COMPAT st_ism330dhcx

#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "ism330dhcx.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(ISM330DHCX, CONFIG_SENSOR_LOG_LEVEL);

static int ism330dhcx_i2c_read(struct device *dev, uint8_t reg_addr,
			    uint8_t *value, uint8_t len)
{
	struct ism330dhcx_data *data = dev->data;
	const struct ism330dhcx_config *cfg = dev->config;

	return i2c_burst_read(data->bus, cfg->i2c_slv_addr,
			      reg_addr, value, len);
}

static int ism330dhcx_i2c_write(struct device *dev, uint8_t reg_addr,
			     uint8_t *value, uint8_t len)
{
	struct ism330dhcx_data *data = dev->data;
	const struct ism330dhcx_config *cfg = dev->config;

	return i2c_burst_write(data->bus, cfg->i2c_slv_addr,
			       reg_addr, value, len);
}

int ism330dhcx_i2c_init(struct device *dev)
{
	struct ism330dhcx_data *data = dev->data;

	data->ctx_i2c.read_reg = (stmdev_read_ptr) ism330dhcx_i2c_read,
	data->ctx_i2c.write_reg = (stmdev_write_ptr) ism330dhcx_i2c_write,

	data->ctx = &data->ctx_i2c;
	data->ctx->handle = dev;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
