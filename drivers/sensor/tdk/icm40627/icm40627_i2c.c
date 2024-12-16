/*
 * Copyright (c) 2025 PHYTEC America LLC
 * Author: Florijan Plohl <florijan.plohl@norik.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for ICM40627 accessed via I2C.
 */

#include "icm40627.h"
#include "icm40627_reg.h"

#if ICM40627_BUS_I2C
static inline int icm40627_bus_check_i2c(const union icm40627_bus *bus)
{
	return i2c_is_ready_dt(&bus->i2c) ? 0 : -ENODEV;
}

static inline int i2c_read_bank(const union icm40627_bus *bus, uint8_t reg, uint8_t bank,
				uint8_t *buf, size_t len)
{
	int res = i2c_reg_write_byte_dt(&bus->i2c, REG_REG_BANK_SEL, bank);

	if (res) {
		return res;
	}

	for (size_t i = 0; i < len; i++) {
		uint8_t addr = reg + i;

		res = i2c_reg_read_byte_dt(&bus->i2c, addr, &buf[i]);

		if (res) {
			return res;
		}
	}

	return 0;
}

static inline int icm40627_reg_read_i2c(const union icm40627_bus *bus, uint16_t reg, uint8_t *data,
					size_t len)
{
	int res = 0;
	uint8_t bank = FIELD_GET(REG_BANK_MASK, reg);
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	res = i2c_read_bank(bus, address, bank, data, len);

	return res;
}

static inline int i2c_write_bank(const union icm40627_bus *bus, uint16_t reg, uint8_t bank,
				 uint8_t buf)
{
	int res = i2c_reg_write_byte_dt(&bus->i2c, REG_REG_BANK_SEL, bank);

	if (res) {
		return res;
	}

	res = i2c_reg_write_byte_dt(&bus->i2c, reg, buf);

	if (res) {
		return res;
	}

	return 0;
}

static inline int icm40627_reg_write_i2c(const union icm40627_bus *bus, uint16_t reg, uint8_t data)
{
	int res = 0;
	uint8_t bank = FIELD_GET(REG_BANK_MASK, reg);
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	res = i2c_write_bank(bus, address, bank, data);

	return res;
}

static inline int icm40627_reg_update_i2c(const union icm40627_bus *bus, uint16_t reg, uint8_t mask,
					  uint8_t val)
{
	return i2c_reg_update_byte_dt(&bus->i2c, reg, mask, val);
}

const struct icm40627_bus_io icm40627_bus_io_i2c = {
	.check = icm40627_bus_check_i2c,
	.read = icm40627_reg_read_i2c,
	.write = icm40627_reg_write_i2c,
	.update = icm40627_reg_update_i2c,
};
#endif /* ICM40627_BUS_I2C */
