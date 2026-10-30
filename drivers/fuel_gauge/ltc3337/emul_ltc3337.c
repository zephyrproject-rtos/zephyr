/*
 * Copyright (c) 2026 Federico Borgo
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the LTC3337 fuel gauge
 */

#define DT_DRV_COMPAT adi_ltc3337

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "emul_ltc3337.h"
#include "ltc3337.h"

LOG_MODULE_REGISTER(EMUL_LTC3337, CONFIG_FUEL_GAUGE_LOG_LEVEL);

struct ltc3337_emul_data {
	uint16_t regs[LTC3337_NUM_REGS];
};

struct ltc3337_emul_cfg {
	/* IPK[2:0] pin strap, reported in register C */
	uint8_t ipk;
};

static void ltc3337_emul_reset(const struct emul *target)
{
	const struct ltc3337_emul_cfg *cfg = target->cfg;
	struct ltc3337_emul_data *data = target->data;

	memset(data->regs, 0, sizeof(data->regs));
	data->regs[LTC3337_REG_A] = LTC3337_A_POR;
	data->regs[LTC3337_REG_C] = FIELD_PREP(LTC3337_C_IPK_MASK, cfg->ipk);
	data->regs[LTC3337_REG_H] = LTC3337_H_POR;
}

void ltc3337_emul_set_reg(const struct emul *target, uint8_t reg, uint16_t val)
{
	struct ltc3337_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg < LTC3337_NUM_REGS);
	data->regs[reg] = val;
}

uint16_t ltc3337_emul_get_reg(const struct emul *target, uint8_t reg)
{
	struct ltc3337_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg < LTC3337_NUM_REGS);

	return data->regs[reg];
}

static int ltc3337_emul_reg_write(const struct emul *target, uint8_t reg, uint16_t val)
{
	struct ltc3337_emul_data *data = target->data;

	switch (reg) {
	case LTC3337_REG_A:
		if ((val & LTC3337_A_CLEAR_INT) != 0U) {
			/* The overflow condition persists until the counter is rewritten */
			data->regs[LTC3337_REG_C] &= ~(LTC3337_C_ALARM_TRIP | LTC3337_C_COLD_ALARM |
						       LTC3337_C_HOT_ALARM);
		}
		/* Clear interrupt and ADC trigger are strobes, they are never stored */
		data->regs[LTC3337_REG_A] = val & ~(LTC3337_A_CLEAR_INT | LTC3337_A_ADC_TRIGGER);
		return 0;
	case LTC3337_REG_B:
		/* The lower byte of the counter is not writable and restarts from zero */
		data->regs[LTC3337_REG_B] = val & LTC3337_B_WRITABLE_MASK;
		data->regs[LTC3337_REG_C] &= ~LTC3337_C_OVERFLOW;
		return 0;
	case LTC3337_REG_H:
		data->regs[LTC3337_REG_H] = val;
		return 0;
	default:
		LOG_ERR("Write to read only or unknown register 0x%02x", reg);
		return -EIO;
	}
}

static int ltc3337_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				     int addr)
{
	struct ltc3337_emul_data *data = target->data;

	ARG_UNUSED(addr);
	__ASSERT_NO_MSG(msgs != NULL && num_msgs > 0);
	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	if ((msgs[0].flags & I2C_MSG_READ) != 0U || msgs[0].len < 1U) {
		LOG_ERR("The first message must write the sub-address");
		return -EIO;
	}

	uint8_t reg = msgs[0].buf[0];

	if (reg < LTC3337_REG_A || reg > LTC3337_REG_H) {
		LOG_ERR("Unknown register 0x%02x", reg);
		return -EIO;
	}

	if (num_msgs == 1) {
		/* Sub-address followed by the two data bytes */
		if (msgs[0].len != 3U) {
			LOG_ERR("A write must be the sub-address and two bytes");
			return -EIO;
		}
		return ltc3337_emul_reg_write(target, reg, sys_get_le16(&msgs[0].buf[1]));
	}

	if (num_msgs == 2 && (msgs[1].flags & I2C_MSG_READ) != 0U && msgs[1].len == 2U) {
		sys_put_le16(data->regs[reg], msgs[1].buf);
		return 0;
	}

	LOG_ERR("Unsupported transfer of %d messages", num_msgs);

	return -EIO;
}

static const struct i2c_emul_api ltc3337_emul_api_i2c = {
	.transfer = ltc3337_emul_transfer_i2c,
};

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>

#define LTC3337_EMUL_RESET(inst) ltc3337_emul_reset(EMUL_DT_GET(DT_DRV_INST(inst)));

static void ltc3337_emul_reset_rule_after(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);

	DT_INST_FOREACH_STATUS_OKAY(LTC3337_EMUL_RESET)
}
ZTEST_RULE(ltc3337_emul_reset, NULL, ltc3337_emul_reset_rule_after);
#endif /* CONFIG_ZTEST */

static int ltc3337_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);
	ltc3337_emul_reset(target);

	return 0;
}

/* The pin strap encoding is the index of ipeak-milliamp in its enum */
#define LTC3337_EMUL_IPK(inst) DT_ENUM_IDX(DT_DRV_INST(inst), ipeak_milliamp)

#define LTC3337_EMUL_DEFINE(inst)                                                                  \
	static struct ltc3337_emul_data ltc3337_emul_data_##inst;                                  \
	static const struct ltc3337_emul_cfg ltc3337_emul_cfg_##inst = {                           \
		.ipk = LTC3337_EMUL_IPK(inst),                                                     \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(inst, ltc3337_emul_init, &ltc3337_emul_data_##inst,                    \
			    &ltc3337_emul_cfg_##inst, &ltc3337_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(LTC3337_EMUL_DEFINE)
