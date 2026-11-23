/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPI functions for Cirrus Logic haptic drivers
 */

#if CONFIG_HAPTICS_CS40LXX_SPI
#include "cs40lxx.h"

static bool cs40lxx_is_ready_spi(const union cs40lxx_bus *const bus)
{
	return spi_is_ready_dt(&bus->spi);
}

static const struct device *cs40lxx_get_device_spi(const union cs40lxx_bus *const bus)
{
	return bus->spi.bus;
}

static int cs40lxx_read_spi(const union cs40lxx_bus *const bus, uint32_t addr, uint32_t *const rx,
			    const uint32_t len)
{
	return -ENOTSUP;
}

static int cs40lxx_write_spi(const union cs40lxx_bus *const bus, const uint32_t addr,
			     uint32_t *const tx, const uint32_t len)
{
	return -ENOTSUP;
}

static int cs40lxx_raw_write_spi(const union cs40lxx_bus *const bus, const uint32_t addr,
				 const uint32_t *const tx, const uint32_t len)
{
	return -ENOTSUP;
}

const struct cs40lxx_io cs40lxx_io_spi = {
	.is_ready = cs40lxx_is_ready_spi,
	.get_device = cs40lxx_get_device_spi,
	.read = cs40lxx_read_spi,
	.write = cs40lxx_write_spi,
	.raw_write = cs40lxx_raw_write_spi,
};
#endif /* CONFIG_HAPTICS_CS40LXX_SPI */
