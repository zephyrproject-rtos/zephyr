/*
 * Copyright (c) 2025, Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief I2C Driver for Cirrus Logic CS40L5x Haptic Devices
 */

#if CONFIG_HAPTICS_CS40L5X_I2C
#include "cs40l5x.h"
#include <zephyr/sys/byteorder.h>

static bool cs40l5x_is_ready_i2c(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;

	return i2c_is_ready_dt(&config->bus.i2c);
}

static const struct device *const cs40l5x_get_device_i2c(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;

	return (const struct device *const)&config->bus.i2c.bus;
}

static int cs40l5x_read_i2c(const struct device *const dev, const uint32_t addr, uint32_t *const rx,
			    const uint32_t len)
{
	const struct cs40l5x_config *const config = dev->config;
	uint8_t addr_be32[4];
	int ret;

	sys_put_be32(addr, addr_be32);

	ret = i2c_write_read_dt(&config->bus.i2c, addr_be32, CS40L5X_ADDR_WIDTH, (uint8_t *)rx,
				len * CS40L5X_REG_WIDTH);
	if (ret < 0) {
		return ret;
	}

	for (int i = 0; i < len; i++) {
		rx[i] = sys_get_be32((uint8_t *)&rx[i]);
	}

	return 0;
}

static int cs40l5x_raw_write_i2c(const struct device *const dev, const uint32_t addr,
				 uint32_t *const tx, const uint32_t len)
{
	const struct cs40l5x_config *const config = dev->config;
	struct i2c_msg msgs[2];
	uint8_t addr_be32[4];

	sys_put_be32(addr, addr_be32);

	msgs[0].buf = addr_be32;
	msgs[0].len = CS40L5X_ADDR_WIDTH;
	msgs[0].flags = I2C_MSG_WRITE;
	msgs[1].buf = (uint8_t *)tx;
	msgs[1].len = len * CS40L5X_REG_WIDTH;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer_dt(&config->bus.i2c, msgs, ARRAY_SIZE(msgs));
}

static int cs40l5x_write_i2c(const struct device *const dev, const uint32_t addr,
			     uint32_t *const tx, const uint32_t len)
{
	for (int i = 0; i < len; i++) {
		sys_put_be32(tx[i], (uint8_t *)&tx[i]);
	}

	return cs40l5x_raw_write_i2c(dev, addr, tx, len);
}

const struct cs40l5x_bus_io cs40l5x_bus_io_i2c = {
	.is_ready = cs40l5x_is_ready_i2c,
	.get_device = cs40l5x_get_device_i2c,
	.read = cs40l5x_read_i2c,
	.write = cs40l5x_write_i2c,
	.raw_write = cs40l5x_raw_write_i2c,
};
#endif /* CONFIG_HAPTICS_CS40L5X_I2C */
