/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BMM150s accessed via I2C.
 */

#include "bmm150.h"

#if BMM150_BUS_I2C

static int bmm150_bus_check_i2c(const union bmm150_bus *bus)
{
	return i2c_is_ready_dt(&bus->i2c) ? 0 : -ENODEV;
}

static int bmm150_reg_read_i2c(const union bmm150_bus *bus,
			       uint8_t start, uint8_t *buf, int size)
{
	return i2c_burst_read_dt(&bus->i2c, start, buf, size);
}

static int bmm150_reg_write_i2c(const union bmm150_bus *bus,
				uint8_t reg, uint8_t val)
{
	return i2c_reg_write_byte_dt(&bus->i2c, reg, val);
}

const struct bmm150_bus_io bmm150_bus_io_i2c = {
	.check = bmm150_bus_check_i2c,
	.read = bmm150_reg_read_i2c,
	.write = bmm150_reg_write_i2c,
};

#endif /* BMM150_BUS_I2C */
