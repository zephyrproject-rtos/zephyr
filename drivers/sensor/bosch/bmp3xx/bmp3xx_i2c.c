/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BMP3xx accessed via I2C.
 */

#include "bmp3xx.h"

#ifdef BMP3XX_USE_I2C_BUS
static int bmp3xx_bus_check_i2c(const union bmp3xx_bus *bus)
{
	return i2c_is_ready_dt(&bus->i2c) ? 0 : -ENODEV;
}

static int bmp3xx_reg_read_i2c(const union bmp3xx_bus *bus, uint8_t start, uint8_t *buf, int size)
{
	return i2c_burst_read_dt(&bus->i2c, start, buf, size);
}

static int bmp3xx_reg_write_i2c(const union bmp3xx_bus *bus, uint8_t reg, uint8_t val)
{
	return i2c_reg_write_byte_dt(&bus->i2c, reg, val);
}

const struct bmp3xx_bus_io bmp3xx_bus_io_i2c = {
	.check = bmp3xx_bus_check_i2c,
	.read = bmp3xx_reg_read_i2c,
	.write = bmp3xx_reg_write_i2c,
};
#endif /* BMP3XX_USE_I2C_BUS */
