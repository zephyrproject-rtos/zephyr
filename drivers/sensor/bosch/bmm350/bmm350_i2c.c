/*
 * Copyright (c) 2024 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bmm350.h"

static int bmm350_bus_check_i2c(const struct bmm350_bus *bus)
{
	return i2c_is_ready_dt(&bus->i2c) ? 0 : -ENODEV;
}

static int bmm350_reg_read_i2c(const struct bmm350_bus *bus, uint8_t start, uint8_t *buf, int size)
{
	return i2c_burst_read_dt(&bus->i2c, start, buf, size);
}

static int bmm350_reg_write_i2c(const struct bmm350_bus *bus, uint8_t reg, uint8_t val)
{
	return i2c_reg_write_byte_dt(&bus->i2c, reg, val);
}

const struct bmm350_bus_io bmm350_bus_io_i2c = {
	.check = bmm350_bus_check_i2c,
	.read = bmm350_reg_read_i2c,
	.write = bmm350_reg_write_i2c,
};
