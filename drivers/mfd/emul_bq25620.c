/*
 * Copyright (c) 2026 Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for the TI BQ25620 battery charger.
 *
 * Models the register file with its power-on reset values, read-only status
 * registers, clear-on-read flag registers and the self-clearing register reset
 * bit.
 */

#define DT_DRV_COMPAT ti_bq25620

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "emul_bq25620.h"
#include "mfd_bq25620.h"

LOG_MODULE_REGISTER(EMUL_BQ25620, CONFIG_MFD_LOG_LEVEL);

struct emul_bq25620_data {
	uint8_t regs[BQ25620_REG_COUNT];
};

struct emul_bq25620_cfg {
	uint16_t addr;
};

/* Power-on reset values, registers not listed reset to 0 */
static const uint8_t emul_bq25620_por[BQ25620_REG_COUNT] = {
	[0x02] = 0x40, [0x03] = 0x03, [0x04] = 0x20, [0x05] = 0x0d, [0x06] = 0x00, [0x07] = 0x0a,
	[0x08] = 0x60, [0x09] = 0x0e, [0x0a] = 0x20, [0x0b] = 0x03, [0x0c] = 0xc0, [0x0d] = 0x0f,
	[0x0e] = 0x00, [0x0f] = 0x0b, [0x10] = 0x50, [0x12] = 0x30, [0x14] = 0x06, [0x15] = 0x5c,
	[0x16] = 0xa1, [0x17] = 0x4f, [0x18] = 0x04, [0x19] = 0xc0, [0x1a] = 0x3d, [0x1b] = 0x25,
	[0x1c] = 0x3f, [0x26] = 0x30, [0x38] = 0x02,
};

/* Self-clearing bits */
#define EMUL_BQ25620_TIMER_CTRL_FORCE_INDET BIT(5)
#define EMUL_BQ25620_CHG_CTRL_1_WD_RST      BIT(2)

static bool emul_bq25620_writable(uint8_t reg)
{
	return IN_RANGE(reg, BQ25620_REG_ICHG, BQ25620_REG_CHG_STAT_0 - 1) ||
	       IN_RANGE(reg, BQ25620_REG_CHG_MASK_0, BQ25620_REG_CHG_MASK_0 + 4);
}

static bool emul_bq25620_clear_on_read(uint8_t reg)
{
	return IN_RANGE(reg, BQ25620_REG_CHG_FLAG_0, BQ25620_REG_FAULT_FLAG_0);
}

static void emul_bq25620_reset(struct emul_bq25620_data *data)
{
	memcpy(data->regs, emul_bq25620_por, sizeof(data->regs));
}

static void emul_bq25620_write_reg(struct emul_bq25620_data *data, uint8_t reg, uint8_t val)
{
	if (!emul_bq25620_writable(reg)) {
		LOG_DBG("Ignoring write to read-only register 0x%02x", reg);
		return;
	}

	switch (reg) {
	case BQ25620_REG_CHG_CTRL_2:
		if ((val & BQ25620_CHG_CTRL_2_REG_RST) != 0U) {
			emul_bq25620_reset(data);
			return;
		}
		break;
	case BQ25620_REG_TIMER_CTRL:
		val &= ~EMUL_BQ25620_TIMER_CTRL_FORCE_INDET;
		break;
	case BQ25620_REG_CHG_CTRL_1:
		val &= ~EMUL_BQ25620_CHG_CTRL_1_WD_RST;
		break;
	default:
		break;
	}

	data->regs[reg] = val;
}

static int emul_bq25620_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				     int addr)
{
	const struct emul_bq25620_cfg *cfg = target->cfg;
	struct emul_bq25620_data *data = target->data;
	uint8_t reg;

	if (addr != cfg->addr) {
		LOG_ERR("Address mismatch, expected 0x%02x, got 0x%02x", cfg->addr, addr);
		return -EIO;
	}

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	if (((msgs[0].flags & I2C_MSG_READ) != 0U) || (msgs[0].len == 0U)) {
		LOG_ERR("First message must be a write with the register");
		return -EIO;
	}

	reg = msgs[0].buf[0];

	/* Write: the payload follows the register in the same or the next message */
	if ((num_msgs == 1) || ((num_msgs == 2) && ((msgs[1].flags & I2C_MSG_READ) == 0U))) {
		const uint8_t *payload = (num_msgs == 1) ? &msgs[0].buf[1] : msgs[1].buf;
		uint32_t len = (num_msgs == 1) ? (msgs[0].len - 1U) : msgs[1].len;

		if ((num_msgs == 2) && (msgs[0].len != 1U)) {
			LOG_ERR("Unsupported message sequence");
			return -EIO;
		}

		if (reg + len > BQ25620_REG_COUNT) {
			return -EIO;
		}

		for (uint32_t i = 0U; i < len; i++) {
			emul_bq25620_write_reg(data, reg + i, payload[i]);
		}

		return 0;
	}

	if ((num_msgs != 2) || (msgs[0].len != 1U)) {
		LOG_ERR("Unsupported message sequence");
		return -EIO;
	}

	if (reg + msgs[1].len > BQ25620_REG_COUNT) {
		return -EIO;
	}

	for (uint32_t i = 0U; i < msgs[1].len; i++) {
		msgs[1].buf[i] = data->regs[reg + i];

		if (emul_bq25620_clear_on_read(reg + i)) {
			data->regs[reg + i] = 0U;
		}
	}

	return 0;
}

void emul_bq25620_set_reg(const struct emul *target, uint8_t reg, uint8_t val)
{
	struct emul_bq25620_data *data = target->data;

	__ASSERT_NO_MSG(reg < BQ25620_REG_COUNT);

	data->regs[reg] = val;
}

uint8_t emul_bq25620_get_reg(const struct emul *target, uint8_t reg)
{
	struct emul_bq25620_data *data = target->data;

	__ASSERT_NO_MSG(reg < BQ25620_REG_COUNT);

	return data->regs[reg];
}

static const struct i2c_emul_api emul_bq25620_api_i2c = {
	.transfer = emul_bq25620_transfer_i2c,
};

static int emul_bq25620_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	emul_bq25620_reset(target->data);

	return 0;
}

#define EMUL_BQ25620_DEFINE(n)                                                                     \
	static struct emul_bq25620_data emul_bq25620_data_##n;                                     \
	static const struct emul_bq25620_cfg emul_bq25620_cfg_##n = {                              \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, emul_bq25620_init, &emul_bq25620_data_##n, &emul_bq25620_cfg_##n,   \
			    &emul_bq25620_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(EMUL_BQ25620_DEFINE)
