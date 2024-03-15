/*
 * Copyright (c) 2024 Gustavo Silva
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sciosense_ens160

#include <zephyr/drivers/i2c.h>

#include "ens160.h"

LOG_MODULE_DECLARE(ENS160, CONFIG_SENSOR_LOG_LEVEL);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

static int ens160_read_reg_i2c(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct ens160_config *config = dev->config;

	return i2c_reg_read_byte_dt(&config->i2c, reg, val);
}

static int ens160_read_data_i2c(const struct device *dev, uint8_t reg, uint8_t *data, size_t len)
{
	const struct ens160_config *config = dev->config;

	return i2c_burst_read_dt(&config->i2c, reg, data, len);
}

static int ens160_write_reg_i2c(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct ens160_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c, reg, val);
}

static int ens160_write_data_i2c(const struct device *dev, uint8_t reg, uint8_t *data, size_t len)
{
	const struct ens160_config *config = dev->config;

	__ASSERT(len == 2, "Only 2 byte write are supported");

	uint8_t buff[] = {reg, data[0], data[1]};

	return i2c_write_dt(&config->i2c, buff, sizeof(buff));
}

const struct ens160_transfer_function ens160_i2c_transfer_function = {
	.read_reg = ens160_read_reg_i2c,
	.read_data = ens160_read_data_i2c,
	.write_reg = ens160_write_reg_i2c,
	.write_data = ens160_write_data_i2c,
};

int ens160_i2c_init(const struct device *dev)
{
	const struct ens160_config *config = dev->config;
	struct ens160_data *data = dev->data;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_DBG("I2C bus device not ready");
		return -ENODEV;
	}

	data->tf = &ens160_i2c_transfer_function;

	return 0;
}

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
