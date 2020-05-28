/* ST Microelectronics LPS22HH pressure and temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22hh.pdf
 */

#define DT_DRV_COMPAT st_lps22hh

#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "lps22hh.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(LPS22HH, CONFIG_SENSOR_LOG_LEVEL);

static int lps22hh_i2c_read(struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint16_t len)
{
	struct lps22hh_data *data = dev->data;
	const struct lps22hh_config *cfg = dev->config;

	return i2c_burst_read(data->bus, cfg->i2c_slv_addr,
			      reg_addr, value, len);
}

static int lps22hh_i2c_write(struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint16_t len)
{
	struct lps22hh_data *data = dev->data;
	const struct lps22hh_config *cfg = dev->config;

	return i2c_burst_write(data->bus, cfg->i2c_slv_addr,
			       reg_addr, value, len);
}

int lps22hh_i2c_init(struct device *dev)
{
	struct lps22hh_data *data = dev->data;

	data->ctx_i2c.read_reg = (stmdev_read_ptr) lps22hh_i2c_read;
	data->ctx_i2c.write_reg = (stmdev_write_ptr) lps22hh_i2c_write;

	data->ctx = &data->ctx_i2c;
	data->ctx->handle = dev;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
