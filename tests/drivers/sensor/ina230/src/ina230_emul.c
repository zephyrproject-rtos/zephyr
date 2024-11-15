/*
 * Copyright (c) 2023 North River Systems Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the TI INA230 I2C power monitor
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <ina230.h>
#include <ina230_emul.h>

LOG_MODULE_REGISTER(INA230_EMUL, CONFIG_SENSOR_LOG_LEVEL);

/* Register ID, size, and value table */
struct ina230_reg {
	uint8_t id;
	uint8_t bytes;
	uint32_t value;
};

/* Emulator configuration passed into driver instance */
struct ina230_emul_cfg {
	uint16_t addr;
};

#define MAX_REGS 0xff

struct ina230_emul_data {
	struct ina230_reg *regs;
};

static struct ina230_reg *get_register(struct ina230_emul_data *data, int reg)
{
	struct ina230_reg *c_reg = data->regs;

	while (c_reg->bytes) {
		if (c_reg->id == reg) {
			return c_reg;
		}
		c_reg++;
	}

	return NULL;
}

int ina230_mock_set_register(void *data_ptr, int reg, uint32_t value)
{
	struct ina230_reg *reg_ptr = get_register(data_ptr, reg);

	if (reg_ptr == NULL) {
		return -EINVAL;
	}

	reg_ptr->value = value;

	if (reg == INA230_REG_CONFIG) {
		/* bit 14 is always set in hardware */
		value |= 1 << 14;
	}

	return 0;
}

int ina230_mock_get_register(void *data_ptr, int reg, uint32_t *value_ptr)
{
	struct ina230_reg *reg_ptr = get_register(data_ptr, reg);

	if (reg_ptr == NULL || value_ptr == NULL) {
		return -EINVAL;
	}

	*value_ptr = reg_ptr->value;
	return 0;
}

static int ina230_emul_transfer_i2c(const struct emul *target, struct i2c_msg msgs[], int num_msgs,
				    int addr)
{
	struct ina230_emul_data *data = (struct ina230_emul_data *)target->data;

	/* The INA230 uses big-endian read 16, read 24, and write 16 transactions */
	if (!msgs || num_msgs < 1 || num_msgs > 2) {
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	if (msgs[0].flags & I2C_MSG_READ) {
		LOG_ERR("Expected write");
		return -EIO;
	}

	if (num_msgs == 1) {
		/* Write16 Transaction */
		if (msgs[0].len != 3) {
			LOG_ERR("Expected 3 bytes");
			return -EIO;
		}

		/* Write 2 bytes */
		uint8_t reg = msgs[0].buf[0];
		uint16_t val = sys_get_be16(&msgs[0].buf[1]);

		struct ina230_reg *reg_ptr = get_register(data, reg);

		if (!reg_ptr) {
			LOG_ERR("Invalid register: %02x", reg);
			return -EIO;
		}
		reg_ptr->value = val;
		LOG_DBG("Write reg %02x: %04x", reg, val);
	} else {
		/* Read 2 or 3 bytes */
		if ((msgs[1].flags & I2C_MSG_READ) == I2C_MSG_WRITE) {
			LOG_ERR("Expected read");
			return -EIO;
		}
		uint8_t reg = msgs[0].buf[0];

		struct ina230_reg *reg_ptr = get_register(data, reg);

		if (!reg_ptr) {
			LOG_ERR("Invalid register: %02x", reg);
			return -EIO;
		}

		if (msgs[1].len == 2) {
			sys_put_be16(reg_ptr->value, msgs[1].buf);
			LOG_DBG("Read16 reg %02x: %04x", reg, reg_ptr->value);
		} else if (msgs[1].len == 3) {
			sys_put_be24(reg_ptr->value, msgs[1].buf);
			LOG_DBG("Read24 reg %02x: %06x", reg, reg_ptr->value);
		} else {
			LOG_ERR("Invalid read length: %d", msgs[1].len);
			return -EIO;
		}
	}

	return 0;
}

static int ina230_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

static const struct i2c_emul_api ina230_emul_api_i2c = {
	.transfer = ina230_emul_transfer_i2c,
};

/* clang-format off */

#define CREATE_INA230_REGS                                                                         \
	{INA230_REG_CONFIG, 2, 0x4127},                                                            \
	{INA230_REG_SHUNT_VOLT, 2, 0},                                                             \
	{INA230_REG_BUS_VOLT, 2, 0},                                                               \
	{INA230_REG_POWER, 2, 0},                                                                  \
	{INA230_REG_CURRENT, 2, 0},                                                                \
	{INA230_REG_CALIB, 2, 0},                                                                  \
	{INA230_REG_MASK, 2, 0},                                                                   \
	{INA230_REG_ALERT, 2, 0}

#define CREATE_INA236_REGS                                                                         \
	CREATE_INA230_REGS,                                                                        \
	{INA236_REG_MANUFACTURER_ID, 2, 0x449},                                                    \
	{INA236_REG_DEVICE_ID, 2, 0xa080}

/* clang-format on */

#define INA230_EMUL(n, v)                                                                          \
	static const struct ina230_emul_cfg ina23##v##_emul_cfg_##n = {                            \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	static struct ina230_reg ina23##v##_regs_##n[] = {CREATE_INA23##v##_REGS, {}};             \
	static struct ina230_emul_data ina23##v##_emul_data_##n = {                                \
		.regs = (struct ina230_reg *)ina23##v##_regs_##n,                                  \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, ina230_emul_init, &ina23##v##_emul_data_##n,                        \
			    &ina23##v##_emul_cfg_##n, &ina230_emul_api_i2c, NULL)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ina230
DT_INST_FOREACH_STATUS_OKAY_VARGS(INA230_EMUL, 0)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ina236
DT_INST_FOREACH_STATUS_OKAY_VARGS(INA230_EMUL, 6)
