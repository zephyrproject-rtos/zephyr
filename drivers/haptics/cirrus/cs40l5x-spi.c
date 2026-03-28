/*
 * Copyright (c) 2025, Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPI Driver for Cirrus Logic CS40L5x Haptic Devices
 */

#define DT_DRV_COMPAT cirrus_cs40l5x

#include <zephyr/drivers/haptics/cs40l5x.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CS40L5X_SPI, CONFIG_HAPTICS_LOG_LEVEL);

static bool cs40l5x_is_ready_spi(const struct device *const dev)
{
	const struct cs40l5x_config *config = dev->config;

	return spi_is_ready_dt(&config->bus.spi);
}

static struct device *cs40l5x_get_device_spi(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;

	return (struct device *)&config->bus.spi.bus;
}

static int cs40l5x_read_spi(const struct device *const dev, const uint32_t addr, uint32_t *const rx,
			    const uint32_t len)
{
	const struct cs40l5x_config *const config = dev->config;

	LOG_INST_ERR(config->log, "SPI not currently supported (%d)", -EPERM);

	return -EPERM;
}

static int cs40l5x_write_spi(const struct device *const dev, const uint32_t *const tx,
			     const uint32_t len)
{
	const struct cs40l5x_config *const config = dev->config;

	LOG_INST_ERR(config->log, "SPI not currently supported (%d)", -EPERM);

	return -EPERM;
}

const struct cs40l5x_bus_io cs40l5x_bus_io_spi = {
	.is_ready = cs40l5x_is_ready_spi,
	.get_device = cs40l5x_get_device_spi,
	.read = cs40l5x_read_spi,
	.write = cs40l5x_write_spi,
};
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
