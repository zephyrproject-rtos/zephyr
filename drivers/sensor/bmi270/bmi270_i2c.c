/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BMI270s accessed via I2C.
 */

#include "bmi270.h"

static int bmi270_bus_check_i2c(const union bmi270_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static int bmi270_reg_read_i2c(const union bmi270_bus *bus,
			       uint8_t start, uint8_t *data, uint16_t len)
{
	return i2c_burst_read_dt(&bus->i2c, start, data, len);
}

static int bmi270_reg_write_i2c(const union bmi270_bus *bus, uint8_t start,
				const uint8_t *data, uint16_t len)
{
	return i2c_burst_write_dt(&bus->i2c, start, data, len);
}

static int bmi270_bus_init_i2c(const union bmi270_bus *bus)
{
	/* I2C is used by default
	 */
	return 0;
}

const struct bmi270_bus_io bmi270_bus_io_i2c = {
	.check = bmi270_bus_check_i2c,
	.read = bmi270_reg_read_i2c,
	.write = bmi270_reg_write_i2c,
	.init = bmi270_bus_init_i2c,
};
