/*
 * Copyright (c) 2024 TDK Invensense
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for ICM42670 accessed via I2C.
 */

#include "icm42670.h"

#if CONFIG_I2C
static int icm42670_bus_check_i2c(const union icm42670_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static int icm42670_reg_read_i2c(const union icm42670_bus *bus, uint8_t reg, uint8_t *buf,
				 uint32_t size)
{
	return i2c_burst_read_dt(&bus->i2c, reg, buf, size);
}

static int icm42670_reg_write_i2c(const union icm42670_bus *bus, uint8_t reg, uint8_t *buf,
				  uint32_t size)
{
	return i2c_burst_write_dt(&bus->i2c, reg, buf, size);
}

const struct icm42670_bus_io icm42670_bus_io_i2c = {
	.check = icm42670_bus_check_i2c,
	.read = icm42670_reg_read_i2c,
	.write = icm42670_reg_write_i2c,
};
#endif /* CONFIG_I2C */
