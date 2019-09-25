/* ST Microelectronics STTS751 temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/stts751.pdf
 */

#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "stts751.h"

#ifdef DT_ST_STTS751_BUS_I2C

LOG_MODULE_DECLARE(STTS751, CONFIG_SENSOR_LOG_LEVEL);

static int stts751_i2c_read(struct device *dev, u8_t reg_addr,
				 u8_t *value, u16_t len)
{
	struct stts751_data *data = dev->driver_data;
	const struct stts751_config *cfg = dev->config->config_info;

	return i2c_burst_read(data->bus, cfg->i2c_slv_addr,
			      reg_addr, value, len);
}

static int stts751_i2c_write(struct device *dev, u8_t reg_addr,
				  u8_t *value, u16_t len)
{
	struct stts751_data *data = dev->driver_data;
	const struct stts751_config *cfg = dev->config->config_info;

	return i2c_burst_write(data->bus, cfg->i2c_slv_addr,
			       reg_addr, value, len);
}

int stts751_i2c_init(struct device *dev)
{
	struct stts751_data *data = dev->driver_data;

	data->ctx_i2c.read_reg = (stmdev_read_ptr) stts751_i2c_read;
	data->ctx_i2c.write_reg = (stmdev_write_ptr) stts751_i2c_write;

	data->ctx = &data->ctx_i2c;
	data->ctx->handle = dev;

	return 0;
}
#endif /* DT_ST_STTS751_BUS_I2C */
