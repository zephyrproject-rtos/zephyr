/*
 * Copyright (c) 2023 NXP Semiconductors
 * Copyright (c) 2023 Cognipilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for IST8310 accessed via I2C.
 */

#include "ist8310.h"

static int ist8310_bus_check_i2c(const union ist8310_bus *bus)
{
	return i2c_is_ready_dt(&bus->i2c) ? 0 : -ENODEV;
}

static int ist8310_reg_read_i2c(const union ist8310_bus *bus, uint8_t start, uint8_t *buf, int size)
{
	return i2c_burst_read_dt(&bus->i2c, start, buf, size);
}

static int ist8310_reg_write_i2c(const union ist8310_bus *bus, uint8_t reg, uint8_t val)
{
	return i2c_reg_write_byte_dt(&bus->i2c, reg, val);
}

const struct ist8310_bus_io ist8310_bus_io_i2c = {
	.check = ist8310_bus_check_i2c,
	.read = ist8310_reg_read_i2c,
	.write = ist8310_reg_write_i2c,
};
