/*
 * Copyright (c) 2025, Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief I2C Driver for Cirrus Logic CS40L5x Haptic Devices
 */

#define DT_DRV_COMPAT cirrus_cs40l5x

#include <zephyr/drivers/haptics/cs40l5x.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(CS40L5X_I2C, CONFIG_HAPTICS_LOG_LEVEL);

#define CS40L5X_REG_WIDTH  4
#define CS40L5X_ADDR_WIDTH CS40L5X_REG_WIDTH

static bool cs40l5x_is_ready_i2c(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;

	return i2c_is_ready_dt(&config->bus.i2c);
}

static struct device *cs40l5x_get_device_i2c(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;

	return (struct device *)&config->bus.i2c.bus;
}

static int cs40l5x_read_i2c(const struct device *const dev, uint32_t addr, uint32_t *const rx,
			    const uint32_t len)
{
	const struct cs40l5x_config *const config = dev->config;
	int error;

	(void)sys_put_be32(addr, (uint8_t *)&addr);

	error = i2c_write_read_dt(&config->bus.i2c, (uint8_t *)&addr, CS40L5X_ADDR_WIDTH,
				  (uint8_t *)rx, len * CS40L5X_REG_WIDTH);
	if (error < 0) {
		return error;
	}

	for (int i = 0; i < len; i++) {
		rx[i] = sys_get_be32((uint8_t *)&rx[i]);
	}

	return error;
}

static int cs40l5x_write_i2c(const struct device *const dev, uint32_t *const tx, const uint32_t len)
{
	const struct cs40l5x_config *const config = dev->config;

	for (int i = 0; i < len; i++) {
		(void)sys_put_be32(tx[i], (uint8_t *)&tx[i]);
	}

	return i2c_write_dt(&config->bus.i2c, (uint8_t *)tx, len * CS40L5X_REG_WIDTH);
}

const struct cs40l5x_bus_io cs40l5x_bus_io_i2c = {
	.is_ready = cs40l5x_is_ready_i2c,
	.get_device = cs40l5x_get_device_i2c,
	.read = cs40l5x_read_i2c,
	.write = cs40l5x_write_i2c,
};
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
