/*
 * Copyright (c) 2025 Jonas Berg
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "ina237.h"
#include "emul_ina228.h"

#define DT_DRV_COMPAT ti_ina228

LOG_MODULE_DECLARE(INA2XX, CONFIG_SENSOR_LOG_LEVEL);

/* Number of consecutive registers.
 * There are two additional read-only registers with constant contents.
 */
#define NUM_REGS 18

struct ina228_emul_data {
	uint64_t reg[NUM_REGS];
};

struct ina228_emul_cfg {
	uint16_t addr;
};

const uint8_t register_sizes[] = {2, 2, 2, 2, 3, 3, 2, 3, 3, 5, 5, 2, 2, 2, 2, 2, 2, 2};
const bool write_allowed[] = {true,  true,  true, true, false, false, false, false, false,
			      false, false, true, true, true,  true,  true,  true,  true};

BUILD_ASSERT(ARRAY_SIZE(register_sizes) == NUM_REGS, "Invalid size of register_sizes");
BUILD_ASSERT(ARRAY_SIZE(write_allowed) == NUM_REGS, "Invalid size of write_allowed");

void ina228_emul_set_reg_16(const struct emul *target, uint8_t reg_addr, uint16_t value)
{
	struct ina228_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr < NUM_REGS);

	LOG_DBG("Setting emulated INA228 16-bit register %u: Value 0x%04X", reg_addr, value);

	data->reg[reg_addr] = (uint64_t)value;
}

void ina228_emul_set_reg_24(const struct emul *target, uint8_t reg_addr, uint32_t value)
{
	struct ina228_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr < NUM_REGS);
	__ASSERT_NO_MSG(value <= INA228_UINT24_MAX);

	LOG_DBG("Setting emulated INA228 24-bit register %u: Value 0x%06X", reg_addr, value);

	data->reg[reg_addr] = (uint64_t)value;
}

void ina228_emul_set_reg_40(const struct emul *target, uint8_t reg_addr, uint64_t value)
{
	struct ina228_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr < NUM_REGS);
	__ASSERT_NO_MSG(value <= INA228_UINT40_MAX);

	LOG_DBG("Setting emulated INA228 40-bit register %u: Value 0x%010X", reg_addr, value);

	data->reg[reg_addr] = value;
}

void ina228_emul_get_reg_16(const struct emul *target, uint8_t reg_addr, uint16_t *value)
{
	struct ina228_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr < NUM_REGS);

	*value = (uint16_t)(data->reg[reg_addr] & UINT16_MAX);

	LOG_DBG("Inspecting emulated INA228 16-bit register %u: Value 0x%04X", reg_addr, *value);
}

void ina228_emul_reset(const struct emul *target)
{
	struct ina228_emul_data *data = target->data;

	LOG_DBG("Resetting INA228 emulator registers");

	memset(&data->reg, 0, sizeof(data->reg));
}

static int ina228_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	LOG_DBG("Initializing INA228 emulator");

	ina228_emul_reset(target);

	return 0;
}

static int ina228_emul_i2c_write(const struct emul *target, struct i2c_msg msgs[], int num_msgs,
				 int addr)
{
	/* These should have been checked before calling this function */
	__ASSERT_NO_MSG(num_msgs == 1);
	__ASSERT_NO_MSG((msgs[0].flags & I2C_MSG_READ) == 0);

	uint8_t reg_addr;
	uint16_t write_value;
	uint8_t expected_register_size;
	bool is_write_allowed;
	struct ina228_emul_data *data = (struct ina228_emul_data *)target->data;

	/* Write 16 bits */
	if (msgs[0].len != 3) {
		LOG_ERR("Write messages should contain 3 bytes, has %d bytes", msgs[0].len);
		return -EIO;
	}

	reg_addr = msgs[0].buf[0];
	write_value = sys_get_be16(&msgs[0].buf[1]);

	if (reg_addr >= NUM_REGS) {
		LOG_ERR("Invalid register address for write: %02X", reg_addr);
		return -EIO;
	}
	expected_register_size = register_sizes[reg_addr];
	is_write_allowed = write_allowed[reg_addr];

	if (!is_write_allowed) {
		LOG_ERR("Register %u is read-only", reg_addr);
		return -EIO;
	}

	if (expected_register_size != sizeof(uint16_t)) {
		LOG_ERR("Illegal to write to register %u, as the register has size %u bytes",
			reg_addr, expected_register_size);
		return -EIO;
	}

	data->reg[reg_addr] = (uint64_t)write_value;
	LOG_DBG("Write 16 bits to register %u: value 0x%04X via emulated I2C", reg_addr,
		write_value);

	return 0;
}

static int ina228_emul_i2c_read(const struct emul *target, struct i2c_msg msgs[], int num_msgs,
				int addr)
{
	/* These should have been checked before calling this function */
	__ASSERT_NO_MSG(num_msgs == 2);
	__ASSERT_NO_MSG((msgs[0].flags & I2C_MSG_READ) == 0);

	uint32_t read_len;
	uint8_t reg_addr;
	uint8_t expected_register_size;
	struct ina228_emul_data *data = (struct ina228_emul_data *)target->data;

	if (!(msgs[1].flags & I2C_MSG_READ)) {
		LOG_ERR("The second I2C message should be of type read");
		return -EIO;
	}
	if (msgs[0].len != 1) {
		LOG_ERR("First message for read should have 1 byte for register address, but has "
			"%d bytes",
			msgs[0].len);
		return -EIO;
	}

	read_len = msgs[1].len;
	reg_addr = msgs[0].buf[0];
	if ((reg_addr >= NUM_REGS) && (reg_addr != INA237_REG_MANUFACTURER_ID) &&
	    (reg_addr != INA228_REG_DEVICE_ID)) {
		LOG_ERR("Invalid register address for read: %u", reg_addr);
		return -EIO;
	}

	if (reg_addr == INA237_REG_MANUFACTURER_ID) {
		if (read_len != sizeof(uint16_t)) {
			LOG_ERR("Invalid read size for MANUFACTURER_ID: %02X", read_len);
			return -EIO;
		}
		sys_put_be16(0x5449, msgs[1].buf); /* Value from data sheet */
		LOG_DBG("Read 16 bits MANUFACTURER_ID via emulated I2C");
		return 0;
	}

	if (reg_addr == INA228_REG_DEVICE_ID) {
		if (read_len != sizeof(uint16_t)) {
			LOG_ERR("Invalid read size for DEVICE_ID: %u", read_len);
			return -EIO;
		}
		sys_put_be16(0x2281, msgs[1].buf); /* Value from data sheet */
		LOG_DBG("Read 16 bits DEVICE_ID via emulated I2C");
		return 0;
	}

	expected_register_size = register_sizes[reg_addr];
	if (read_len != expected_register_size) {
		LOG_ERR("Invalid read size for register %u: Register is %u bytes, but asked for %u "
			"bytes",
			reg_addr, expected_register_size, read_len);
		return -EIO;
	}

	if (read_len == 2) {
		sys_put_be16(data->reg[reg_addr], msgs[1].buf);
		LOG_DBG("Read 16 bits from register %u: 0x%04X via emulated I2C", reg_addr,
			data->reg[reg_addr]);
		return 0;
	}

	if (read_len == 3) {
		sys_put_be24(data->reg[reg_addr], msgs[1].buf);
		LOG_DBG("Read 24 bits from register %u: 0x%06X via emulated I2C", reg_addr,
			data->reg[reg_addr]);
		return 0;
	}

	if (read_len == 5) {
		sys_put_be40(data->reg[reg_addr], msgs[1].buf);
		LOG_DBG("Read 40 bits from register %u: 0x%010X via emulated I2C", reg_addr,
			data->reg[reg_addr]);
		return 0;
	}

	LOG_ERR("Invalid read size for register %u: %u bytes", reg_addr, read_len);
	return -EIO;
}

static int ina228_emul_transfer_i2c(const struct emul *target, struct i2c_msg msgs[], int num_msgs,
				    int addr)
{
	if (!msgs || num_msgs < 1 || num_msgs > 2) {
		LOG_ERR("Invalid number of I2C messages: %d", num_msgs);
		return -EIO;
	}

	if (msgs[0].flags & I2C_MSG_READ) {
		LOG_ERR("The first I2C message should be write");
		return -EIO;
	}

	if (num_msgs == 1) {
		return ina228_emul_i2c_write(target, msgs, num_msgs, addr);
	}

	return ina228_emul_i2c_read(target, msgs, num_msgs, addr);
}

static const struct i2c_emul_api ina228_emul_api_i2c = {
	.transfer = ina228_emul_transfer_i2c,
};

#define INA228_EMUL(n)                                                                             \
	static const struct ina228_emul_cfg ina228_emul_cfg_##n = {.addr = DT_INST_REG_ADDR(n)};   \
	static struct ina228_emul_data ina228_emul_data_##n = {0};                                 \
	EMUL_DT_INST_DEFINE(n, ina228_emul_init, &ina228_emul_data_##n, &ina228_emul_cfg_##n,      \
			    &ina228_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(INA228_EMUL)
