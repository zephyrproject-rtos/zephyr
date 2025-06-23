/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common functions for Cirrus Logic CS35L family of smart amplifiers
 */

#include "cs35lxx.h"

bool cs35lxx_is_bus_ready(const struct cs35lxx_io_bus *const io_bus)
{
	return io_bus->io->is_ready(&io_bus->bus);
}

const struct device *cs35lxx_get_control_port(const struct cs35lxx_io_bus *const io_bus)
{
	return io_bus->io->get_device(&io_bus->bus);
}

int cs35lxx_burst_read(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr,
		       uint32_t *const rx, const uint32_t len)
{
	return io_bus->io->read(&io_bus->bus, addr, rx, len);
}

int cs35lxx_read(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr, uint32_t *const rx)
{
	return cs35lxx_burst_read(io_bus, addr, rx, 1);
}

int cs35lxx_burst_write(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr,
			uint32_t *const tx, const uint32_t len)
{
	return io_bus->io->write(&io_bus->bus, addr, tx, len);
}

int cs35lxx_write(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr,
		  const uint32_t val)
{
	uint32_t tx = val;

	return cs35lxx_burst_write(io_bus, addr, &tx, 1);
}

int cs35lxx_raw_burst_write(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr,
			    const uint32_t *const tx, const uint32_t len)
{
	return io_bus->io->raw_write(&io_bus->bus, addr, tx, len);
}

int cs35lxx_raw_write(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr,
		      const uint32_t val)
{
	return cs35lxx_raw_burst_write(io_bus, addr, &val, 1);
}

int cs35lxx_update(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr,
		   const uint32_t mask, const uint32_t val)
{
	uint32_t orig, tmp;
	int ret;

	ret = cs35lxx_read(io_bus, addr, &orig);
	if (ret < 0) {
		return ret;
	}

	tmp = orig & ~mask;
	tmp |= val & mask;

	return cs35lxx_write(io_bus, addr, tmp);
}
