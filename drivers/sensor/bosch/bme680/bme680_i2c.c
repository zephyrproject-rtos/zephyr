/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2022, Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BME680s accessed via I2C.
 */

#include "bme680.h"

#if BME680_BUS_I2C
static int bme680_bus_check_i2c(const union bme680_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static int bme680_reg_read_i2c(const struct device *dev,
			       uint8_t start, uint8_t *buf, int size)
{
	const struct bme680_config *config = dev->config;

	return i2c_burst_read_dt(&config->bus.i2c, start, buf, size);
}

static int bme680_reg_write_i2c(const struct device *dev,
				uint8_t reg, uint8_t val)
{
	const struct bme680_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->bus.i2c, reg, val);
}

const struct bme680_bus_io bme680_bus_io_i2c = {
	.check = bme680_bus_check_i2c,
	.read = bme680_reg_read_i2c,
	.write = bme680_reg_write_i2c,
};
#endif /* BME680_BUS_I2C */
