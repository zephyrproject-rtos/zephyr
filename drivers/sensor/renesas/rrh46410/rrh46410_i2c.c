/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rrh46410

#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

#include "rrh46410.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(RRH46410, CONFIG_SENSOR_LOG_LEVEL);

static int rrh46410_i2c_read_data(const struct device *dev, uint8_t *rx_buff, size_t data_size)
{
	const struct rrh46410_config *cfg = dev->config;

	return i2c_read_dt(&cfg->bus_cfg.i2c, rx_buff, data_size);
}

static int rrh46410_i2c_write_data(const struct device *dev, uint8_t *command_data, size_t data_size)
{
	const struct rrh46410_config *cfg = dev->config;

	return i2c_write_dt(&cfg->bus_cfg.i2c, command_data, data_size);
}

static const struct rrh46410_transfer_function rrh46410_i2c_transfer_fn = {
	.read_data = rrh46410_i2c_read_data,
	.write_data = rrh46410_i2c_write_data,
};

int rrh46410_i2c_init(const struct device *dev)
{
	struct rrh46410_data *data = dev->data;
	const struct rrh46410_config *cfg = dev->config;

	data->hw_tf = &rrh46410_i2c_transfer_fn;

	if (!device_is_ready(cfg->bus_cfg.i2c.bus)) {
		return -ENODEV;
	}

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
