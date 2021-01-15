/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BME280s accessed via I2C.
 */

#include "bme280.h"

#if BME280_BUS_I2C
static int bme280_bus_check_i2c(const struct device *bus,
				const union bme280_bus_config *bus_config)
{
	return device_is_ready(bus) ? 0 : -ENODEV;
}

static int bme280_reg_read_i2c(const struct device *bus,
			       const union bme280_bus_config *bus_config,
			       uint8_t start, uint8_t *buf, int size)
{
	return i2c_burst_read(bus, bus_config->i2c_addr,
			      start, buf, size);
}

static int bme280_reg_write_i2c(const struct device *bus,
				const union bme280_bus_config *bus_config,
				uint8_t reg, uint8_t val)
{
	return i2c_reg_write_byte(bus, bus_config->i2c_addr,
				  reg, val);
}

const struct bme280_bus_io bme280_bus_io_i2c = {
	.check = bme280_bus_check_i2c,
	.read = bme280_reg_read_i2c,
	.write = bme280_reg_write_i2c,
};
#endif	/* BME280_BUS_I2C */
