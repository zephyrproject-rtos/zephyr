/*
 * Copyright (c) 2020 Michael Pollind
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "icm20948.h"

#ifdef DT_TDK_ICM20948_BUS_I2C

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_DECLARE(ICM20948);

static int icm20948_bus_check_i2c(const union icm20948_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static inline int icm20948_change_bank(struct icm20948_bus *data,
				       uint16_t reg_bank_addr)
{
	uint8_t bank = (uint8_t)(reg_bank_addr >> 7);

	if (bank != data->active_bank) {
		data->active_bank = bank;
		return i2c_reg_write_byte(data->bus,
					  CONFIG_ICM20948_I2C_SLAVE_ADDR,
					  ICM20948_REG_BANK_SEL, (bank << 4));
	}
	return 0;
}

static int icm20948_i2c_read_data(struct icm20948_bus *data,
				  uint16_t reg_bank_addr, uint8_t *value, uint8_t len)
{
	uint8_t reg_addr = (uint8_t)(reg_bank_addr & 0xff);

	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	return i2c_burst_read(data->bus, CONFIG_ICM20948_I2C_SLAVE_ADDR,
			      reg_addr, value, len);
}

static int icm20948_i2c_write_data(struct icm20948_bus *data,
				   uint16_t reg_bank_addr, uint8_t *value, uint8_t len)
{
	uint8_t reg_addr = (uint8_t)(reg_bank_addr & 0xff);

	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	return i2c_burst_write(data->bus, CONFIG_ICM20948_I2C_SLAVE_ADDR,
			       reg_addr, value, len);
}

static int icm20948_i2c_read_reg(struct icm20948_bus *data,
				 uint16_t reg_bank_addr, uint8_t *value)
{
	uint8_t reg_addr = (uint8_t)(reg_bank_addr & 0xff);

	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	return i2c_reg_read_byte(data->bus, CONFIG_ICM20948_I2C_SLAVE_ADDR,
				 reg_addr, value);
}

static int icm20948_i2c_write_reg(struct icm20948_bus *data,
				  uint16_t reg_bank_addr, uint8_t value)
{
	uint8_t reg_addr = (uint8_t)(reg_bank_addr & 0xff);

	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	return i2c_reg_write_byte(data->bus, CONFIG_ICM20948_I2C_SLAVE_ADDR,
				  reg_addr, value);
}

static int icm20948_i2c_update_reg(struct icm20948_bus *data,
				   uint16_t reg_bank_addr, uint8_t mask, uint8_t value)
{
	uint8_t reg_addr = (uint8_t)(reg_bank_addr & 0xff);

	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	return i2c_reg_update_byte(data->bus, CONFIG_ICM20948_I2C_SLAVE_ADDR,
				   reg_addr, mask,
				   value << __builtin_ctz(mask));
}

static const struct icm20948_bus_io icm20948_bus_io_i2c = {
	.check = icm20948_bus_check_i2c,
	.read_data = icm20948_i2c_read_data,
	.write_data = icm20948_i2c_write_data,
	.read_reg = icm20948_i2c_read_reg,
	.write_reg = icm20948_i2c_write_reg,
	.update_reg = icm20948_i2c_update_reg
};


#endif