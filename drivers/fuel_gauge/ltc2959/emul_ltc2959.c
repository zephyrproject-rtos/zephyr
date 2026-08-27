/**
 * Copyright (c) 2025 Nathan Winslow
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for ltc2959 fuel gauge
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/byteorder.h>

#define DT_DRV_COMPAT adi_ltc2959

LOG_MODULE_REGISTER(EMUL_LTC2959);

#include "ltc2959.h"

struct ltc2959_emul_data {
	uint8_t regs[LTC2959_REG_GPIO_THRESH_LOW_LSB + 1]; /* enough for all regs */
};

struct ltc2959_emul_cfg {
	/* I2C Address of emulator */
	uint16_t addr;
};

static int ltc2959_emul_reset(const struct emul *target)
{
	struct ltc2959_emul_data *data = (struct ltc2959_emul_data *)target->data;

	memset(data->regs, 0, sizeof(data->regs));

	/* Values according to pgs 10-11 of the LTC2959 datasheet */
	data->regs[LTC2959_REG_STATUS] = 0x01;
	data->regs[LTC2959_REG_ADC_CONTROL] = 0x18;
	data->regs[LTC2959_REG_CC_CONTROL] = 0x50;
	data->regs[LTC2959_REG_ACC_CHARGE_3] = 0x80;
	data->regs[LTC2959_REG_CHG_THRESH_HIGH_3] = 0xFF;
	data->regs[LTC2959_REG_CHG_THRESH_HIGH_2] = 0xFF;
	data->regs[LTC2959_REG_CHG_THRESH_HIGH_1] = 0xFF;
	data->regs[LTC2959_REG_CHG_THRESH_HIGH_0] = 0xFF;
	data->regs[LTC2959_REG_VOLT_THRESH_HIGH_MSB] = 0xFF;
	data->regs[LTC2959_REG_VOLT_THRESH_HIGH_LSB] = 0xFF;
	data->regs[LTC2959_REG_CURR_THRESH_HIGH_MSB] = 0x7F;
	data->regs[LTC2959_REG_CURR_THRESH_HIGH_LSB] = 0xFF;
	data->regs[LTC2959_REG_CURR_THRESH_LOW_MSB] = 0x80;
	data->regs[LTC2959_REG_MAX_CURRENT_MSB] = 0x80;
	data->regs[LTC2959_REG_MIN_CURRENT_MSB] = 0x7F;
	data->regs[LTC2959_REG_MIN_CURRENT_LSB] = 0xFF;
	data->regs[LTC2959_REG_TEMP_THRESH_HIGH_MSB] = 0xFF;
	data->regs[LTC2959_REG_TEMP_THRESH_HIGH_LSB] = 0xFF;
	data->regs[LTC2959_REG_GPIO_THRESH_HIGH_MSB] = 0x7F;
	data->regs[LTC2959_REG_GPIO_THRESH_HIGH_LSB] = 0xFF;
	data->regs[LTC2959_REG_GPIO_THRESH_LOW_MSB] = 0x80;

	return 0;
}

static int emul_ltc2959_reg_write(const struct emul *target, int reg, int val)
{
	struct ltc2959_emul_data *data = target->data;

	switch (reg) {
	case LTC2959_REG_ADC_CONTROL:
	case LTC2959_REG_CC_CONTROL:
	case LTC2959_REG_ACC_CHARGE_3:
	case LTC2959_REG_ACC_CHARGE_2:
	case LTC2959_REG_ACC_CHARGE_1:
	case LTC2959_REG_ACC_CHARGE_0:
	case LTC2959_REG_CHG_THRESH_LOW_3:
	case LTC2959_REG_CHG_THRESH_LOW_2:
	case LTC2959_REG_CHG_THRESH_LOW_1:
	case LTC2959_REG_CHG_THRESH_LOW_0:
	case LTC2959_REG_CHG_THRESH_HIGH_3:
	case LTC2959_REG_CHG_THRESH_HIGH_2:
	case LTC2959_REG_CHG_THRESH_HIGH_1:
	case LTC2959_REG_CHG_THRESH_HIGH_0:
	case LTC2959_REG_VOLT_THRESH_HIGH_MSB:
	case LTC2959_REG_VOLT_THRESH_HIGH_LSB:
	case LTC2959_REG_VOLT_THRESH_LOW_MSB:
	case LTC2959_REG_VOLT_THRESH_LOW_LSB:
	case LTC2959_REG_MAX_VOLTAGE_MSB:
	case LTC2959_REG_MAX_VOLTAGE_LSB:
	case LTC2959_REG_MIN_VOLTAGE_MSB:
	case LTC2959_REG_MIN_VOLTAGE_LSB:
	case LTC2959_REG_CURR_THRESH_HIGH_MSB:
	case LTC2959_REG_CURR_THRESH_HIGH_LSB:
	case LTC2959_REG_CURR_THRESH_LOW_MSB:
	case LTC2959_REG_CURR_THRESH_LOW_LSB:
	case LTC2959_REG_MAX_CURRENT_MSB:
	case LTC2959_REG_MAX_CURRENT_LSB:
	case LTC2959_REG_MIN_CURRENT_MSB:
	case LTC2959_REG_MIN_CURRENT_LSB:
	case LTC2959_REG_TEMP_THRESH_HIGH_MSB:
	case LTC2959_REG_TEMP_THRESH_HIGH_LSB:
	case LTC2959_REG_TEMP_THRESH_LOW_MSB:
	case LTC2959_REG_TEMP_THRESH_LOW_LSB:
	case LTC2959_REG_GPIO_THRESH_HIGH_MSB:
	case LTC2959_REG_GPIO_THRESH_HIGH_LSB:
	case LTC2959_REG_GPIO_THRESH_LOW_MSB:
	case LTC2959_REG_GPIO_THRESH_LOW_LSB:
		data->regs[reg] = val;
		break;

	case LTC2959_REG_STATUS:
	case LTC2959_REG_VOLTAGE_MSB:
	case LTC2959_REG_VOLTAGE_LSB:
	case LTC2959_REG_CURRENT_MSB:
	case LTC2959_REG_CURRENT_LSB:
	case LTC2959_REG_TEMP_MSB:
	case LTC2959_REG_TEMP_LSB:
	case LTC2959_REG_GPIO_VOLTAGE_MSB:
	case LTC2959_REG_GPIO_VOLTAGE_LSB:
	default:
		LOG_ERR("Unknown or Read Only Register: 0x%x", reg);
		return -EIO;
	}
	return 0;
}

static int emul_ltc2959_reg_read(const struct emul *target, int reg, int *val)
{
	if (reg < LTC2959_REG_STATUS || reg > LTC2959_REG_GPIO_THRESH_LOW_LSB) {
		LOG_ERR("Unknown Register: 0x%x", reg);
		return -EIO;
	}

	struct ltc2959_emul_data *data = target->data;
	*val = data->regs[reg];

	return 0;
}

static int ltc2959_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				     int addr)
{
	__ASSERT_NO_MSG(msgs && num_msgs);
	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	switch (num_msgs) {
	case 1: {
		/* Single write: [reg, data0, data1, ...] */
		struct i2c_msg *m = &msgs[0];

		if (m->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected single-message read");
			return -EIO;
		}
		if (m->len < 2) {
			LOG_ERR("Single-message write must be reg+data (len=%d)", m->len);
			return -EIO;
		}
		uint8_t reg = m->buf[0];

		for (size_t i = 1; i < m->len; i++, reg++) {
			int ret = emul_ltc2959_reg_write(target, reg, m->buf[i]);

			if (ret < 0) {
				return ret;
			}
		}
		return 0;
	}

	case 2: {
		/* Two-message: [reg], then [read N] OR [write N] */
		struct i2c_msg *m0 = &msgs[0];
		struct i2c_msg *m1 = &msgs[1];

		if ((m0->flags & I2C_MSG_READ) || m0->len != 1) {
			LOG_ERR("Invalid first msg (flags=0x%x len=%d)", m0->flags, m0->len);
			return -EIO;
		}

		uint8_t reg = m0->buf[0];

		if (m1->flags & I2C_MSG_READ) {
			/* Burst READ: stream N bytes starting at reg */
			for (size_t i = 0; i < m1->len; i++) {
				int val;
				int ret = emul_ltc2959_reg_read(target, reg + i, &val);

				if (ret < 0) {
					return ret;
				}

				m1->buf[i] = (uint8_t)val;
			}
			return 0;
		}
		/* Burst WRITE: stream N bytes into reg..reg+N-1 */
		if (!m1->len) {
			LOG_ERR("Empty write");
			return -EIO;
		}
		for (size_t i = 0; i < m1->len; i++) {
			int ret = emul_ltc2959_reg_write(target, reg + i, m1->buf[i]);

			if (ret < 0) {
				return ret;
			}
		}
		return 0;
	}

	default:
		LOG_ERR("Unsupported number of I2C messages: %d", num_msgs);
		return -EIO;
	}
}

/* The I2C emulator API required by Zephyr. */
static const struct i2c_emul_api ltc2959_emul_api_i2c = {
	.transfer = ltc2959_emul_transfer_i2c,
};

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>

/* Add test reset handlers in when using emulators with tests */
#define LTC2959_EMUL_RESET_RULE_BEFORE(inst) ltc2959_emul_reset(EMUL_DT_GET(DT_DRV_INST(inst)));

static void ltc2959_gauge_reset_rule_after(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);

	DT_INST_FOREACH_STATUS_OKAY(LTC2959_EMUL_RESET_RULE_BEFORE)
}
ZTEST_RULE(ltc2959_gauge_reset, NULL, ltc2959_gauge_reset_rule_after);
#endif /* CONFIG_ZTEST */

static int ltc2959_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);
	ltc2959_emul_reset(target);
	return 0;
}

/*
 * Main instantiation macro.
 */
#define DEFINE_LTC2959_EMUL(n)                                                                     \
	static struct ltc2959_emul_data ltc2959_emul_data_##n;                                     \
	static const struct ltc2959_emul_cfg ltc2959_emul_cfg_##n = {                              \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, ltc2959_emul_init, &ltc2959_emul_data_##n, &ltc2959_emul_cfg_##n,   \
			    &ltc2959_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(DEFINE_LTC2959_EMUL);
