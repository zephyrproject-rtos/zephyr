/* ST Microelectronics STTS751 temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/stts751.pdf
 */

#define DT_DRV_COMPAT st_stts751

#include <string.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "stts751.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(STTS751, CONFIG_SENSOR_LOG_LEVEL);

static int stts751_i2c_read(struct stts751_data *data, uint8_t reg_addr,
			    uint8_t *value, uint16_t len)
{
	const struct stts751_config *cfg = data->dev->config;

	return i2c_burst_read(data->bus, cfg->i2c_slv_addr,
			      reg_addr, value, len);
}

static int stts751_i2c_write(struct stts751_data *data, uint8_t reg_addr,
			     uint8_t *value, uint16_t len)
{
	const struct stts751_config *cfg = data->dev->config;

	return i2c_burst_write(data->bus, cfg->i2c_slv_addr,
			       reg_addr, value, len);
}

int stts751_i2c_init(const struct device *dev)
{
	struct stts751_data *data = dev->data;

	data->ctx_i2c.read_reg = (stmdev_read_ptr) stts751_i2c_read;
	data->ctx_i2c.write_reg = (stmdev_write_ptr) stts751_i2c_write;

	data->ctx = &data->ctx_i2c;
	data->ctx->handle = data;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
