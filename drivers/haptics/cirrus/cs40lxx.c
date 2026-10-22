/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common functions for Cirrus Logic haptic drivers
 */

#include "cs40lxx.h"

bool cs40lxx_is_bus_ready(const struct cs40lxx_io_bus *const io_bus)
{
	return io_bus->io->is_ready(&io_bus->bus);
}

const struct device *cs40lxx_get_control_port(const struct cs40lxx_io_bus *const io_bus)
{
	return io_bus->io->get_device(&io_bus->bus);
}

int cs40lxx_burst_read(const struct cs40lxx_io_bus *const io_bus, const uint32_t addr,
		       uint32_t *const rx, const uint32_t len)
{
	return io_bus->io->read(&io_bus->bus, addr, rx, len);
}

int cs40lxx_read(const struct cs40lxx_io_bus *const io_bus, const uint32_t addr, uint32_t *const rx)
{
	return cs40lxx_burst_read(io_bus, addr, rx, 1);
}

int cs40lxx_burst_write(const struct cs40lxx_io_bus *const io_bus, const uint32_t addr,
			uint32_t *const tx, const uint32_t len)
{
	return io_bus->io->write(&io_bus->bus, addr, tx, len);
}

int cs40lxx_write(const struct cs40lxx_io_bus *const io_bus, const uint32_t addr, uint32_t val)
{
	return cs40lxx_burst_write(io_bus, addr, &val, 1);
}

int cs40lxx_raw_burst_write(const struct cs40lxx_io_bus *const io_bus, const uint32_t addr,
			    const uint32_t *const tx, const uint32_t len)
{
	return io_bus->io->raw_write(&io_bus->bus, addr, tx, len);
}

int cs40lxx_raw_write(const struct cs40lxx_io_bus *const io_bus, const uint32_t addr,
		      const uint32_t val)
{
	return cs40lxx_raw_burst_write(io_bus, addr, &val, 1);
}

int cs40lxx_multi_write(const struct cs40lxx_io_bus *const io_bus,
			const struct cs40lxx_multi_write *const multi_write, const uint32_t len)
{
	int ret;

	for (int i = 0; i < len; i++) {
		ret = cs40lxx_burst_write(io_bus, multi_write[i].addr, multi_write[i].buf,
					  multi_write[i].len);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

int cs40lxx_raw_multi_write(const struct cs40lxx_io_bus *const io_bus,
			    const struct cs40lxx_multi_write *const multi_write, const uint32_t len)
{
	int ret;

	for (int i = 0; i < len; i++) {
		ret = cs40lxx_raw_burst_write(io_bus, multi_write[i].addr, multi_write[i].buf,
					      multi_write[i].len);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
