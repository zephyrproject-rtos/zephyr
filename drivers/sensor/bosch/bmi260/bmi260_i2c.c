/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BMI260s accessed via I2C.
 */

#include <string.h>
#include "bmi260.h"

static int bmi260_bus_check_i2c(const union bmi260_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static int bmi260_reg_read_i2c(const union bmi260_bus *bus,
			       uint8_t start, uint8_t *data, uint16_t len)
{
	return i2c_burst_read_dt(&bus->i2c, start, data, len);
}

static int bmi260_reg_write_i2c(const union bmi260_bus *bus, uint8_t start,
				const uint8_t *data, uint16_t len)
{
	/* Combine register address and data into a single buffer and
	 * use i2c_write_dt() instead of i2c_burst_write_dt() which
	 * may not be supported on all I2C devices.
	 * Maximum write length is BMI260_WR_LEN + 1 byte for address.
	 */
	uint8_t buf[1 + BMI260_WR_LEN];

	if (len > BMI260_WR_LEN) {
		return -EINVAL;
	}

	buf[0] = start;
	if (len > 0) {
		memcpy(&buf[1], data, len);
	}

	return i2c_write_dt(&bus->i2c, buf, 1 + len);
}

static int bmi260_bus_init_i2c(const union bmi260_bus *bus)
{
	/* I2C is used by default
	 */
	return 0;
}

const struct bmi260_bus_io bmi260_bus_io_i2c = {
	.check = bmi260_bus_check_i2c,
	.read = bmi260_reg_read_i2c,
	.write = bmi260_reg_write_i2c,
	.init = bmi260_bus_init_i2c,
};
