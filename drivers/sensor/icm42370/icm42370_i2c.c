/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include "icm42370_i2c.h"
#include "icm42370_reg.h"

static inline int i2c_write_register(const struct i2c_dt_spec *bus, uint8_t reg, uint8_t data)
{
	return i2c_reg_write_byte_dt(bus, reg, data);
}

static inline int i2c_read_register(const struct i2c_dt_spec *bus, uint8_t reg, uint8_t *data,
				    size_t len)
{
	return i2c_burst_read_dt(bus, reg, data, len);
}

static int set_idle(const struct i2c_dt_spec *bus)
{
	int res;
	uint8_t reg_pwr_mgmt0;
	uint8_t value;

	res = i2c_read_register(bus, REG_PWR_MGMT0, &reg_pwr_mgmt0, 1);

	if (res) {
		return res;
	}

	value = reg_pwr_mgmt0;
	value |= BIT_IDLE;

	if (reg_pwr_mgmt0 != value) {
		res = i2c_write_register(bus, REG_PWR_MGMT0, value);

		if (res) {
			return res;
		}

		k_usleep(20);
	}

	return 0;
}

int icm42370_read_mreg(const struct i2c_dt_spec *bus, uint16_t reg,
				uint8_t *buf, size_t len)
{
	int res;
	uint8_t reg_pwr_mgmt0;
	uint8_t bank = FIELD_GET(REG_BANK_MASK, reg);

	printk("mreg read bank %X\n", bank);
	res = i2c_read_register(bus, REG_PWR_MGMT0, &reg_pwr_mgmt0, 1);

	if (res) {
		return res;
	}

	res = set_idle(bus);

	if (res) {
		return res;
	}

	res = i2c_write_register(bus, REG_BLK_SEL_R, bank);
	k_usleep(10);
	if (res) {
		goto restore_bank;
	}

	/* reads from MREG registers must be done byte-by-byte */
	for (size_t i = 0; i < len; i++) {
		uint8_t addr = reg + i;

		res = i2c_write_register(bus, REG_MADDR_R, addr & 0xff);
		k_usleep(MREG_R_W_WAIT_US);
		if (res) {
			goto restore_bank;
		}

		res = i2c_read_register(bus, REG_M_R, &buf[i], 1);
		k_usleep(MREG_R_W_WAIT_US);
		if (res) {
			goto restore_bank;
		}
	}

restore_bank:
	res |= i2c_write_register(bus, REG_BLK_SEL_W, 0);
	k_usleep(10);
	res |= i2c_write_register(bus, REG_PWR_MGMT0, reg_pwr_mgmt0);

	return res;
}

int icm42370_write_mreg(const struct i2c_dt_spec *bus, uint16_t reg,
				 uint8_t buf)
{
	int res;
	uint8_t reg_pwr_mgmt0;
	uint8_t bank = FIELD_GET(REG_BANK_MASK, reg);

	printk("Mreg write bank %X\n", bank);

	res = i2c_read_register(bus, REG_PWR_MGMT0, &reg_pwr_mgmt0, 1);

	if (res) {
		return res;
	}

	res = set_idle(bus);

	if (res) {
		return res;
	}

	res = i2c_write_register(bus, REG_BLK_SEL_W, bank);
	k_usleep(10);
	if (res) {
		goto restore_bank;
	}
	res = i2c_write_register(bus, REG_MADDR_W, reg & 0xff);
	k_usleep(10);
	if (res) {
		goto restore_bank;
	}
	res = i2c_write_register(bus, REG_M_W, buf);
	k_usleep(MREG_R_W_WAIT_US);

	if (res) {
		goto restore_bank;
	}


restore_bank:
	res |= i2c_write_register(bus, REG_BLK_SEL_W, 0);
	k_usleep(10);
	res |= i2c_write_register(bus, REG_PWR_MGMT0, reg_pwr_mgmt0);

	return res;
}

int icm42370_update_mreg(const struct i2c_dt_spec *bus, uint16_t reg, uint8_t mask,
				 uint8_t data)
{
	uint8_t temp = 0;
	int res = icm42370_read_mreg(bus, reg, &temp, 1);

	if (res) {
		return res;
	}

	temp &= ~mask;
	temp |= FIELD_PREP(mask, data);

	return icm42370_write_mreg(bus, reg, temp);
}

int icm42370_read(const struct i2c_dt_spec *bus, uint16_t reg, uint8_t *data, size_t len)
{
	int res = 0;
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	res = i2c_read_register(bus, address, data, len);

	return res;
}

int icm42370_update_register(const struct i2c_dt_spec *bus, uint16_t reg, uint8_t mask,
			     uint8_t data)
{
	uint8_t temp = 0;
	int res = icm42370_read(bus, reg, &temp, 1);

	if (res) {
		return res;
	}

	temp &= ~mask;
	temp |= FIELD_PREP(mask, data);

	return icm42370_single_write(bus, reg, temp);
}

int icm42370_single_write(const struct i2c_dt_spec *bus, uint16_t reg, uint8_t data)
{
	int res = 0;
	uint8_t address = FIELD_GET(REG_ADDRESS_MASK, reg);

	res = i2c_write_register(bus, address, data);

	return res;
}
