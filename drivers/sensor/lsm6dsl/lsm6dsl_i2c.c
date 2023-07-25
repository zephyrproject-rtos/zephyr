/* lsm6dsl_i2c.c - I2C routines for LSM6DSL driver
 */

/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lsm6dsl

#include <string.h>
#include <zephyr/logging/log.h>

#include "lsm6dsl.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(LSM6DSL, CONFIG_SENSOR_LOG_LEVEL);

static int lsm6dsl_i2c_read_data(const struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	const struct lsm6dsl_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->bus_cfg.i2c, reg_addr, value, len);
}

static int lsm6dsl_i2c_write_data(const struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint8_t len)
{
	const struct lsm6dsl_config *cfg = dev->config;

	return i2c_burst_write_dt(&cfg->bus_cfg.i2c, reg_addr, value, len);
}

static int lsm6dsl_i2c_read_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t *value)
{
	const struct lsm6dsl_config *cfg = dev->config;

	return i2c_reg_read_byte_dt(&cfg->bus_cfg.i2c, reg_addr, value);
}

static int lsm6dsl_i2c_update_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t mask, uint8_t value)
{
	const struct lsm6dsl_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->bus_cfg.i2c, reg_addr, mask, value);
}


static const struct lsm6dsl_transfer_function lsm6dsl_i2c_transfer_fn = {
	.read_data = lsm6dsl_i2c_read_data,
	.write_data = lsm6dsl_i2c_write_data,
	.read_reg  = lsm6dsl_i2c_read_reg,
	.update_reg = lsm6dsl_i2c_update_reg,
};

int lsm6dsl_i2c_init(const struct device *dev)
{
	struct lsm6dsl_data *data = dev->data;
	const struct lsm6dsl_config *cfg = dev->config;

	data->hw_tf = &lsm6dsl_i2c_transfer_fn;

	if (!device_is_ready(cfg->bus_cfg.i2c.bus)) {
		return -ENODEV;
	}

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
