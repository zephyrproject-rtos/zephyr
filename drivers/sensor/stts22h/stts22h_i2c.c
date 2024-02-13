/* ST Microelectronics STTS22H temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/stts22h.pdf
 */

#define DT_DRV_COMPAT st_stts22h

#include <string.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "stts22h.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(STTS22H, CONFIG_SENSOR_LOG_LEVEL);

static int stts22h_i2c_read(const struct device *dev, uint8_t reg_addr, uint8_t *value,
			    uint16_t len)
{
	const struct stts22h_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->i2c, reg_addr, value, len);
}

static int stts22h_i2c_write(const struct device *dev, uint8_t reg_addr, uint8_t *value,
			     uint16_t len)
{
	const struct stts22h_config *cfg = dev->config;

	return i2c_burst_write_dt(&cfg->i2c, reg_addr, value, len);
}

int stts22h_i2c_init(const struct device *dev)
{
	struct stts22h_data *data = dev->data;

	data->ctx_i2c.read_reg = (stmdev_read_ptr)stts22h_i2c_read;
	data->ctx_i2c.write_reg = (stmdev_write_ptr)stts22h_i2c_write;
	data->ctx_i2c.mdelay = (stmdev_mdelay_ptr)stmemsc_mdelay,

	data->ctx = &data->ctx_i2c;
	data->ctx->handle = (void *)dev;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
