/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Cherrence Sarip <Cherrence.sarip@analog.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>

#include <adi_tmc_i2c.h>
#include "tmc522x.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tmc522x, CONFIG_STEPPER_LOG_LEVEL);

static int tmc522x_bus_check_i2c(const union tmc522x_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static int tmc522x_bus_init_i2c(const union tmc522x_bus *bus)
{
	/* I2C is initialized by the I2C driver, nothing special needed */
	return tmc522x_bus_check_i2c(bus);
}

static int tmc522x_reg_read_i2c(const union tmc522x_bus *bus, uint8_t reg_addr, uint32_t *reg_val)
{
	int ret;

	if (reg_val == NULL) {
		return -EINVAL;
	}

	ret = tmc_i2c_read_register(&bus->i2c, reg_addr, reg_val);
	if (ret < 0) {
		LOG_ERR("I2C read failed: reg=0x%02X, err=%d", reg_addr, ret);
		return ret;
	}

	return 0;
}

static int tmc522x_reg_write_i2c(const union tmc522x_bus *bus, uint8_t reg_addr, uint32_t reg_val)
{
	int ret;

	ret = tmc_i2c_write_register(&bus->i2c, reg_addr, reg_val);
	if (ret < 0) {
		LOG_ERR("I2C write failed: reg=0x%02X val=0x%08X, err=%d", reg_addr, reg_val, ret);
		return ret;
	}

	return 0;
}

const struct tmc522x_bus_io tmc522x_bus_io_i2c = {
	.check = tmc522x_bus_check_i2c,
	.init = tmc522x_bus_init_i2c,
	.read = tmc522x_reg_read_i2c,
	.write = tmc522x_reg_write_i2c,
};
