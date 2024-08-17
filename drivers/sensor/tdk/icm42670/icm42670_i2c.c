/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for ICM42670 accessed via I2C.
 */

#include "icm42670.h"
#include "icm42670_reg.h"

#if ICM42670_BUS_I2C
static int icm42670_bus_check_i2c(const union icm42670_bus *bus)
{
	return i2c_is_ready_dt(&bus->i2c) ? 0 : -ENODEV;
}

static int i2c_read_mreg(const union icm42670_bus *bus, uint8_t reg, uint8_t bank,
				uint8_t *buf, size_t len)
{
	int res = i2c_reg_write_byte_dt(&bus->i2c, REG_BLK_SEL_R, bank);

	if (res) {
		return res;
	}

	/* reads from MREG registers must be done byte-by-byte */
	for (size_t i = 0; i < len; i++) {
		uint8_t addr = reg + i;

		res = i2c_reg_write_byte_dt(&bus->i2c, REG_MADDR_R, addr);

		if (res) {
			return res;
		}

		k_usleep(MREG_R_W_WAIT_US);
		res = i2c_reg_read_byte_dt(&bus->i2c, REG_M_R, &buf[i]);

		if (res) {
			return res;
		}

		k_usleep(MREG_R_W_WAIT_US);
	}

	return 0;
}

static int icm42670_reg_read_i2c(const union icm42670_bus *bus, uint16_t reg, uint8_t *data,
				 size_t len)
{
	int res = 0;
	uint8_t bank = FIELD_GET(REG_BANK_MASK, reg);
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	if (bank) {
		res = i2c_read_mreg(bus, address, bank, data, len);
	} else {
		res = i2c_burst_read_dt(&bus->i2c, address, data, len);
	}

	return res;
}

static int i2c_write_mreg(const union icm42670_bus *bus, uint16_t reg, uint8_t bank,
				 uint8_t buf)
{
	int res = i2c_reg_write_byte_dt(&bus->i2c, REG_BLK_SEL_W, bank);

	if (res) {
		return res;
	}

	res = i2c_reg_write_byte_dt(&bus->i2c, REG_MADDR_W, reg);

	if (res) {
		return res;
	}

	res = i2c_reg_write_byte_dt(&bus->i2c, REG_M_W, buf);

	if (res) {
		return res;
	}

	k_usleep(MREG_R_W_WAIT_US);

	return 0;
}

static int icm42670_reg_write_i2c(const union icm42670_bus *bus,
				uint16_t reg, uint8_t data)
{
	int res = 0;
	uint8_t bank = FIELD_GET(REG_BANK_MASK, reg);
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	if (bank) {
		res = i2c_write_mreg(bus, address, bank, data);
	} else {
		res = i2c_reg_write_byte_dt(&bus->i2c, address, data);
	}

	return res;
}

static int icm42670_reg_update_i2c(const union icm42670_bus *bus, uint16_t reg, uint8_t mask,
				   uint8_t val)
{
	return i2c_reg_update_byte_dt(&bus->i2c, reg, mask, val);
}

const struct icm42670_bus_io icm42670_bus_io_i2c = {
	.check = icm42670_bus_check_i2c,
	.read = icm42670_reg_read_i2c,
	.write = icm42670_reg_write_i2c,
	.update = icm42670_reg_update_i2c,
};
#endif /* ICM42670_BUS_I2C */
