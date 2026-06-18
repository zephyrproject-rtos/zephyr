/*
 * Copyright (c) 2026, Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPI Driver for Cirrus Logic CS40L26/27 Haptic Devices
 */

#if CONFIG_HAPTICS_CS40L26_SPI
#include "cs40l26.h"
#include <zephyr/logging/log.h>
#include <zephyr/toolchain.h>

LOG_MODULE_REGISTER(CS40L26_SPI, CONFIG_HAPTICS_LOG_LEVEL);

static bool cs40l26_is_ready_spi(const struct device *const dev)
{
	const struct cs40l26_config *config = dev->config;

	return spi_is_ready_dt(&config->bus.spi);
}

static const struct device *const cs40l26_get_device_spi(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;

	return (const struct device *const)&config->bus.spi.bus;
}

static int cs40l26_read_spi(const struct device *const dev, uint32_t addr, uint32_t *const rx,
			    const uint32_t len)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;

	LOG_INST_ERR(config->log, "SPI not currently supported (%d)", -ENOTSUP);

	return -ENOTSUP;
}

static int cs40l26_write_spi(const struct device *const dev, const uint32_t addr,
			     uint32_t *const tx, const uint32_t len)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;

	LOG_INST_ERR(config->log, "SPI not currently supported (%d)", -ENOTSUP);

	return -ENOTSUP;
}

static int cs40l26_raw_write_spi(const struct device *const dev, const uint32_t addr,
				 uint32_t *const tx, const uint32_t len)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;

	LOG_INST_ERR(config->log, "SPI not currently supported (%d)", -ENOTSUP);

	return -ENOTSUP;
}

const struct cs40l26_bus_io cs40l26_bus_io_spi = {
	.is_ready = cs40l26_is_ready_spi,
	.get_device = cs40l26_get_device_spi,
	.read = cs40l26_read_spi,
	.write = cs40l26_write_spi,
	.raw_write = cs40l26_raw_write_spi,
};
#endif /* CONFIG_HAPTICS_CS40L26_SPI */
