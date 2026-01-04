/*
 * Copyright (c) 2025, Bojan Sofronievski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bhi2xy.h"

static int bhi2xy_bus_check_i2c(const union bhi2xy_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static int bhi2xy_reg_read_i2c(const union bhi2xy_bus *bus, uint8_t reg, uint8_t *data,
			       uint16_t len)
{
	return i2c_burst_read_dt(&bus->i2c, reg, data, len);
}

static int bhi2xy_reg_write_i2c(const union bhi2xy_bus *bus, uint8_t reg, const uint8_t *data,
				uint16_t len)
{
	return i2c_burst_write_dt(&bus->i2c, reg, data, len);
}

static enum bhy2_intf bhi2xy_get_intf_i2c(const union bhi2xy_bus *bus)
{
	return BHY2_I2C_INTERFACE;
}

const struct bhi2xy_bus_io bhi2xy_bus_io_i2c = {.check = bhi2xy_bus_check_i2c,
						.read = bhi2xy_reg_read_i2c,
						.write = bhi2xy_reg_write_i2c,
						.get_intf = bhi2xy_get_intf_i2c};
