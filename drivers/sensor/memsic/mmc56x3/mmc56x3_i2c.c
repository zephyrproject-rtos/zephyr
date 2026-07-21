/*
 * Copyright (c) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for MMC56X3s accessed via I2C.
 */

#include "mmc56x3.h"

static int mmc56x3_bus_check_i2c(const union mmc56x3_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static int mmc56x3_reg_read_i2c(const union mmc56x3_bus *bus, uint8_t reg, uint8_t *buf, int size)
{
	return i2c_burst_read_dt(&bus->i2c, reg, buf, size);
}

static int mmc56x3_reg_write_i2c(const union mmc56x3_bus *bus, uint8_t reg, uint8_t val)
{
	return i2c_reg_write_byte_dt(&bus->i2c, reg, val);
}

static int mmc56x3_raw_read_i2c(const union mmc56x3_bus *bus, uint8_t *buf, size_t size)
{
	return i2c_read_dt(&bus->i2c, buf, size);
}

static int mmc56x3_raw_write_i2c(const union mmc56x3_bus *bus, uint8_t *buf, size_t size)
{
	return i2c_write_dt(&bus->i2c, buf, size);
}

const struct mmc56x3_bus_io mmc56x3_bus_io_i2c = {
	.check = mmc56x3_bus_check_i2c,
	.read = mmc56x3_reg_read_i2c,
	.write = mmc56x3_reg_write_i2c,
	.raw_read = mmc56x3_raw_read_i2c,
	.raw_write = mmc56x3_raw_write_i2c,
};
