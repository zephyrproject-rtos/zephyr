/*
 * Copyright (c) 2025 ZARM, University of Bremen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include "icm42688_i2c.h"
#include "icm42688_dev_cfg.h"
#include "icm42688_reg.h"

int icm42688_i2c_read(const struct device *dev, uint16_t reg, uint8_t *data, size_t len)
{
	int res = 0;
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);
	const struct icm42688_dev_cfg *config = dev->config;

	res = i2c_burst_read_dt(&config->bus.i2c, address, data, len);
	return res;
}

int icm42688_i2c_update_register(const struct device *dev, uint16_t reg, uint8_t mask, uint8_t data)
{
	int res = 0;
	const struct icm42688_dev_cfg *config = dev->config;
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	res = i2c_reg_update_byte_dt(&config->bus.i2c, address, mask, data);
	return res;
}

int icm42688_i2c_single_write(const struct device *dev, uint16_t reg, uint8_t data)
{
	int res = 0;
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);
	const struct icm42688_dev_cfg *config = dev->config;

	res = i2c_reg_write_byte_dt(&config->bus.i2c, address, data);
	return res;
}
