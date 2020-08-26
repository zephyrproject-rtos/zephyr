/* ST Microelectronics LSM6DSO 6-axis IMU sensor driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dso.pdf
 */

#define DT_DRV_COMPAT st_lsm6dso

#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "lsm6dso.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(LSM6DSO, CONFIG_SENSOR_LOG_LEVEL);

static int lsm6dso_i2c_read(struct device *dev, uint8_t reg_addr,
			    uint8_t *value, uint8_t len)
{
	struct lsm6dso_data *data = dev->data;
	const struct lsm6dso_config *cfg = dev->config;

	return i2c_burst_read(data->bus, cfg->i2c_slv_addr,
			      reg_addr, value, len);
}

static int lsm6dso_i2c_write(struct device *dev, uint8_t reg_addr,
			     uint8_t *value, uint8_t len)
{
	struct lsm6dso_data *data = dev->data;
	const struct lsm6dso_config *cfg = dev->config;

	return i2c_burst_write(data->bus, cfg->i2c_slv_addr,
			       reg_addr, value, len);
}

int lsm6dso_i2c_init(struct device *dev)
{
	struct lsm6dso_data *data = dev->data;

	data->ctx_i2c.read_reg = (stmdev_read_ptr) lsm6dso_i2c_read,
	data->ctx_i2c.write_reg = (stmdev_write_ptr) lsm6dso_i2c_write,

	data->ctx = &data->ctx_i2c;
	data->ctx->handle = dev;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
