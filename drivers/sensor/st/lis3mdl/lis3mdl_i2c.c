/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* adapted from lsm6dsl_i2c.c - I2C routines for LSM6DSL driver
 */
#define DT_DRV_COMPAT st_lis3mdl_magn

#include <string.h>
#include <zephyr/logging/log.h>

#include "lis3mdl.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(LIS3MDL, CONFIG_SENSOR_LOG_LEVEL);

static int lis3mdl_i2c_read_data(const struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	const struct lis3mld_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->bus_cfg.i2c, reg_addr, value, len);
}

static int lis3mdl_i2c_write_data(const struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint8_t len)
{
	const struct lis3mdl_config *cfg = dev->config;

	return i2c_burst_write_dt(&cfg->bus_cfg.i2c, reg_addr, value, len);
}

static int lis3mdl_i2c_read_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t *value)
{
	const struct lis3mdl_config *cfg = dev->config;

	return i2c_reg_read_byte_dt(&cfg->bus_cfg.i2c, reg_addr, value);
}

static int lis3mdl_i2c_update_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t mask, uint8_t value)
{
	const struct lis3mdl_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->bus_cfg.i2c, reg_addr, mask, value);
}


static const struct lis3mdl_transfer_function lis3mdl_i2c_transfer_fn = {
	.read_data = lis3mdl_i2c_read_data,
	.write_data = lis3mdl_i2c_write_data,
	.read_reg  = lis3mdl_i2c_read_reg,
	.update_reg = lis3mdl_i2c_update_reg,
};

int lis3mdl_i2c_init(const struct device *dev)
{
	struct lis3mdl_data *data = dev->data;
	const struct lis3mdl_config *cfg = dev->config;

	data->hw_tf = &lis3mdl_i2c_transfer_fn;

	if (!device_is_ready(cfg->bus_cfg.i2c.bus)) {
		return -ENODEV;
	}

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */