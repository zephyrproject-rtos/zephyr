/*
 * Copyright (c) 2026 Richard Osterloh <richard.osterloh@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm20948.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

static int icm20948_i2c_check(const union icm20948_bus *bus)
{
	return i2c_is_ready_dt(&bus->i2c) ? 0 : -ENODEV;
}

static int icm20948_i2c_read(const union icm20948_bus *bus, uint8_t addr, uint8_t *buf, size_t len)
{
	return i2c_burst_read_dt(&bus->i2c, addr, buf, len);
}

static int icm20948_i2c_write(const union icm20948_bus *bus, uint8_t addr, uint8_t val)
{
	return i2c_reg_write_byte_dt(&bus->i2c, addr, val);
}

const struct icm20948_bus_io icm20948_bus_io_i2c = {
	.check = icm20948_i2c_check,
	.read = icm20948_i2c_read,
	.write = icm20948_i2c_write,
};

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
