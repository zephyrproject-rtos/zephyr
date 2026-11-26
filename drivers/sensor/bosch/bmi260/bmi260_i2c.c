/*
 * Copyright (c) 2026 Shreyas Acharya
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C implementation of the BMI260 bus interface.
 */

#include <string.h>

#include <zephyr/drivers/i2c.h>

#include "bmi260.h"
#include "bmi260_reg_map.h"

static int bmi260_bus_check_i2c(const union bmi260_bus *bus)
{
	return i2c_is_ready_dt(&bus->i2c) ? 0 : -ENODEV;
}

static int bmi260_reg_read_i2c(const union bmi260_bus *bus, uint8_t start, uint8_t *data,
			       uint16_t len)
{
	return i2c_burst_read_dt(&bus->i2c, start, data, len);
}

static int bmi260_reg_write_i2c(const union bmi260_bus *bus, uint8_t start, const uint8_t *data,
				uint16_t len)
{
	uint8_t buf[1 + BMI260_WR_LEN];

	if (len > BMI260_WR_LEN) {
		return -EINVAL;
	}

	/*
	 * The address and the payload go out as one message. i2c_burst_write_dt()
	 * would express this directly but is not supported by every controller,
	 * so the message is assembled here instead.
	 */
	buf[0] = start;
	memcpy(&buf[1], data, len);

	return i2c_write_dt(&bus->i2c, buf, len + 1U);
}

static int bmi260_bus_init_i2c(const union bmi260_bus *bus)
{
	ARG_UNUSED(bus);

	/* The device powers up listening on I2C, so there is nothing to do. */
	return 0;
}

const struct bmi260_bus_io bmi260_bus_io_i2c = {
	.check = bmi260_bus_check_i2c,
	.read = bmi260_reg_read_i2c,
	.write = bmi260_reg_write_i2c,
	.init = bmi260_bus_init_i2c,
};
