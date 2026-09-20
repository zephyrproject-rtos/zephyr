/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "icm4268x_bus_io.h"
#include "icm4268x_reg.h"

LOG_MODULE_DECLARE(ICM4268X, CONFIG_SENSOR_LOG_LEVEL);

#if ICM4268X_BUS_I2C
static int icm4268x_bus_check_i2c(const union icm4268x_bus_cfg *bus)
{
	if (!i2c_is_ready_dt(&bus->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(bus->i2c.bus);
		return -ENODEV;
	}

	return 0;
}

static int icm4268x_reg_read_i2c(const union icm4268x_bus_cfg *bus, uint16_t reg,
				 uint8_t *data, size_t len)
{
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	return i2c_burst_read_dt(&bus->i2c, address, data, len);
}

static int icm4268x_reg_write_i2c(const union icm4268x_bus_cfg *bus, uint16_t reg,
				  uint8_t data)
{
	uint8_t buffer[] = {
		FIELD_GET(REG_ADDRESS_MASK, reg),
		data,
	};

	return i2c_write_dt(&bus->i2c, buffer, sizeof(buffer));
}

static int icm4268x_reg_update_i2c(const union icm4268x_bus_cfg *bus, uint16_t reg,
				   uint8_t mask, uint8_t data)
{
	uint8_t value;
	int ret;

	ret = icm4268x_reg_read_i2c(bus, reg, &value, 1);
	if (ret != 0) {
		return ret;
	}

	value &= ~mask;
	value |= FIELD_PREP(mask, data);

	return icm4268x_reg_write_i2c(bus, reg, value);
}

const struct icm4268x_bus_io icm4268x_bus_io_i2c = {
	.check = icm4268x_bus_check_i2c,
	.read = icm4268x_reg_read_i2c,
	.write = icm4268x_reg_write_i2c,
	.update = icm4268x_reg_update_i2c,
};
#endif /* ICM4268X_BUS_I2C */
