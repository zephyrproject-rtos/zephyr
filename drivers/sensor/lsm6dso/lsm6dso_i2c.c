/* ST Microelectronics LSM6DSO 6-axis IMU sensor driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dso.pdf
 */

#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "lsm6dso.h"

#ifdef DT_ST_LSM6DSO_BUS_I2C

LOG_MODULE_DECLARE(LSM6DSO, CONFIG_SENSOR_LOG_LEVEL);

static int lsm6dso_i2c_read(struct device *dev, u8_t reg_addr,
			    u8_t *value, u8_t len)
{
	struct lsm6dso_data *data = dev->driver_data;
	const struct lsm6dso_config *cfg = dev->config->config_info;

	return i2c_burst_read(data->bus, cfg->i2c_slv_addr,
			      reg_addr, value, len);
}

static int lsm6dso_i2c_write(struct device *dev, u8_t reg_addr,
			     u8_t *value, u8_t len)
{
	struct lsm6dso_data *data = dev->driver_data;
	const struct lsm6dso_config *cfg = dev->config->config_info;

	return i2c_burst_write(data->bus, cfg->i2c_slv_addr,
			       reg_addr, value, len);
}

int lsm6dso_i2c_init(struct device *dev)
{
	struct lsm6dso_data *data = dev->driver_data;

	data->ctx_i2c.read_reg = (stmdev_read_ptr) lsm6dso_i2c_read,
	data->ctx_i2c.write_reg = (stmdev_write_ptr) lsm6dso_i2c_write,

	data->ctx = &data->ctx_i2c;
	data->ctx->handle = dev;

	return 0;
}
#endif /* DT_ST_LSM6DSO_BUS_I2C */
