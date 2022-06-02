/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BMP180s accessed via I2C.
 */

#include "bmp180.h"

#if BMP180_BUS_I2C
static int bmp180_bus_check_i2c(const union bmp180_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static int bmp180_reg_read_i2c(const union bmp180_bus *bus,
			       uint8_t start, uint8_t *buf, int size)
{
	return i2c_burst_read_dt(&bus->i2c, start, buf, size);
}

static int bmp180_reg_write_i2c(const union bmp180_bus *bus,
				uint8_t reg, uint8_t val)
{
	return i2c_reg_write_byte_dt(&bus->i2c, reg, val);
}

const struct bmp180_bus_io bmp180_bus_io_i2c = {
	.check = bmp180_bus_check_i2c,
	.read = bmp180_reg_read_i2c,
	.write = bmp180_reg_write_i2c,
};
#endif /* BMP180_BUS_I2C */
