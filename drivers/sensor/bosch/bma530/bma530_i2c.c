/* Bosch bma530 3-axis accelerometer driver
 *
 * Copyright (c) 2024 Arrow Electronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma530

#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "bma530.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(bma530, CONFIG_SENSOR_LOG_LEVEL);

static int bma530_i2c_read_data(const struct device *dev, uint8_t reg_addr,
				uint8_t *value, uint8_t len)
{
	const struct bma530_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->bus_cfg.i2c, reg_addr, value, len);
}

static int bma530_i2c_write_data(const struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint8_t len)
{
	const struct bma530_config *cfg = dev->config;

	return i2c_burst_write_dt(&cfg->bus_cfg.i2c, reg_addr, value, len);
}

static int bma530_i2c_read_reg(const struct device *dev, uint8_t reg_addr,
			       uint8_t *value)
{
	const struct bma530_config *cfg = dev->config;

	return i2c_reg_read_byte_dt(&cfg->bus_cfg.i2c, reg_addr, value);
}

static int bma530_i2c_update_reg(const struct device *dev, uint8_t reg_addr,
				 uint8_t mask, uint8_t value)
{
	const struct bma530_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->bus_cfg.i2c, reg_addr, mask, value);
}

static const struct bma530_hw_operations i2c_ops = {
	.read_data  = bma530_i2c_read_data,
	.write_data = bma530_i2c_write_data,
	.read_reg   = bma530_i2c_read_reg,
	.update_reg = bma530_i2c_update_reg,
};

int bma530_i2c_init(const struct device *dev)
{
	struct bma530_data *data = dev->data;
	const struct bma530_config *cfg = dev->config;

	if (!device_is_ready(cfg->bus_cfg.i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	data->hw_ops = &i2c_ops;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
