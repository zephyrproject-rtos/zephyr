/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPI functions for Cirrus Logic CS35L family audio drivers
 */

#if CONFIG_AUDIO_CODEC_CS35LXX_SPI
#include "cs35lxx.h"

static bool cs35lxx_is_ready_spi(const union cs35lxx_bus *const bus)
{
	return spi_is_ready_dt(&bus->spi);
}

static const struct device *cs35lxx_get_device_spi(const union cs35lxx_bus *const bus)
{
	return bus->spi.bus;
}

static int cs35lxx_read_spi(const union cs35lxx_bus *const bus, const uint32_t addr,
			    uint32_t *const rx, const uint32_t len)
{
	return -ENOTSUP;
}

static int cs35lxx_write_spi(const union cs35lxx_bus *const bus, const uint32_t addr,
			     uint32_t *const tx, const uint32_t len)
{
	return -ENOTSUP;
}

static int cs35lxx_raw_write_spi(const union cs35lxx_bus *const bus, const uint32_t addr,
				 const uint32_t *const tx, const uint32_t len)
{
	return -ENOTSUP;
}

const struct cs35lxx_io cs35lxx_io_spi = {
	.is_ready = cs35lxx_is_ready_spi,
	.get_device = cs35lxx_get_device_spi,
	.read = cs35lxx_read_spi,
	.write = cs35lxx_write_spi,
	.raw_write = cs35lxx_raw_write_spi,
};
#endif /* CONFIG_AUDIO_CODEC_CS35LXX_SPI */
