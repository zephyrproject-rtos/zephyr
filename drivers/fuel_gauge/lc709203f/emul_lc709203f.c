/*
 * Copyright (c) 2025 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for lc709203f fuel gauge
 */

#include <string.h>
#define DT_DRV_COMPAT onnn_lc709203f

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(EMUL_LC709203F);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include "lc709203f.h"

/* You can store as many registers as you need.
 * Note: The LC709203F typically uses 16-bit registers.
 */
struct lc709203f_emul_data {
	/* This emulator object (required for i2c_emul). */
	struct i2c_emul emul;
	/* The I2C emulation config (pointer to our dev_config). */
	const struct i2c_emul_api *api;
	/* A backing store for registers in the device. */
	uint16_t regs[0x1B]; /* or enough to hold all used registers */
};

struct lc709203f_emul_cfg {
	/** I2C address of emulator */
	uint16_t addr;
};

/* Polynomial to calculate CRC-8-ATM */
#define LC709203F_CRC_POLYNOMIAL 0x07

/* Reset handler (optional). You can reset internal state here if desired. */
static int lc709203f_emul_reset(const struct emul *target)
{
	struct lc709203f_emul_data *data = (struct lc709203f_emul_data *)target->data;

	memset(data->regs, 0, sizeof(data->regs));

	/* Set default values for registers that your real hardware starts with */
	data->regs[LC709203F_REG_BEFORE_RSOC] = 0x0000;       /* - */
	data->regs[LC709203F_REG_THERMISTOR_B] = 0x0D34;      /* B -constant */
	data->regs[LC709203F_REG_INITIAL_RSOC] = 0x0000;      /* - */
	data->regs[LC709203F_REG_CELL_TEMPERATURE] = 0x0BA6;  /* 25.0 °C 298.2 °K -> */
	data->regs[LC709203F_REG_CELL_VOLTAGE] = 3700;        /* 3.7 V in mV */
	data->regs[LC709203F_REG_CURRENT_DIRECTION] = 0x0000; /* Auto mode */
	data->regs[LC709203F_REG_APA] = 0x0000;               /* - */
	data->regs[LC709203F_REG_APT] = 0x001E;               /* initial value */
	data->regs[LC709203F_REG_RSOC] = 50;                  /* 50% battery level */
	data->regs[LC709203F_REG_CELL_ITE] = 500;             /* 50.0% battery level */
	data->regs[LC709203F_REG_IC_VERSION] = 0x1234;        /* Example chip ID */
	data->regs[LC709203F_REG_BAT_PROFILE] = 0x0000;       /* - */
	data->regs[LC709203F_REG_ALARM_LOW_RSOC] = 0x0008;    /* 8% */
	data->regs[LC709203F_REG_ALARM_LOW_VOLTAGE] = 0x0000; /* initial value */
	data->regs[LC709203F_REG_IC_POWER_MODE] = 0x0002;     /* - */
	data->regs[LC709203F_REG_STATUS_BIT] = 0x0000;        /* initial value */
	data->regs[LC709203F_REG_NUM_PARAMETER] = 0x0301;     /* - */

	return 0;
}

static int emul_lc709203f_reg_write(const struct emul *target, uint8_t *buf, size_t len)
{
	struct lc709203f_emul_data *data = target->data;
	const struct lc709203f_emul_cfg *lc709203f_emul_cfg = target->cfg;
	const uint8_t reg = buf[0];
	const uint16_t value = sys_get_le16(&buf[1]);
	const uint8_t crc = buf[3];
	uint8_t crc_buf[4];

	crc_buf[0] = lc709203f_emul_cfg->addr << 1;
	crc_buf[1] = reg;
	crc_buf[2] = buf[1];
	crc_buf[3] = buf[2];

	const uint8_t crc_calc = crc8(crc_buf, sizeof(crc_buf), LC709203F_CRC_POLYNOMIAL, 0, false);

	if (crc != crc_calc) {
		LOG_ERR("CRC mismatch on reg 0x%02x", reg);
		return -EIO;
	}

	switch (reg) {
	case LC709203F_REG_RSOC:
		data->regs[LC709203F_REG_RSOC] = value;
		data->regs[LC709203F_REG_CELL_ITE] = value * 10;
		break;
	case LC709203F_REG_BEFORE_RSOC:
	case LC709203F_REG_THERMISTOR_B:
	case LC709203F_REG_INITIAL_RSOC:
	case LC709203F_REG_CELL_TEMPERATURE:
	case LC709203F_REG_CURRENT_DIRECTION:
	case LC709203F_REG_APA:
	case LC709203F_REG_APT:
	case LC709203F_REG_BAT_PROFILE:
	case LC709203F_REG_ALARM_LOW_RSOC:
	case LC709203F_REG_ALARM_LOW_VOLTAGE:
	case LC709203F_REG_IC_POWER_MODE:
	case LC709203F_REG_STATUS_BIT:
		data->regs[reg] = value;
		break;
	default:
		LOG_ERR("Unknown or read only register 0x%x write", reg);
		return -EIO;
	}
	return 0;
}

static int emul_lc709203f_reg_read(const struct emul *target, int reg, uint8_t *buf, size_t len)
{
	struct lc709203f_emul_data *data = target->data;
	const struct lc709203f_emul_cfg *lc709203f_emul_cfg = target->cfg;
	uint16_t val = 0;

	switch (reg) {
	case LC709203F_REG_CELL_TEMPERATURE:
		if (data->regs[LC709203F_REG_STATUS_BIT] == 0x0000) {
			LOG_ERR("Temperature obtaining method is not set to Thermistor mode, "
				"instead its set to I2C mode");
			return -EIO;
		}
	case LC709203F_REG_THERMISTOR_B:
	case LC709203F_REG_CELL_VOLTAGE:
	case LC709203F_REG_CURRENT_DIRECTION:
	case LC709203F_REG_APA:
	case LC709203F_REG_APT:
	case LC709203F_REG_RSOC:
	case LC709203F_REG_CELL_ITE:
	case LC709203F_REG_IC_VERSION:
	case LC709203F_REG_BAT_PROFILE:
	case LC709203F_REG_ALARM_LOW_RSOC:
	case LC709203F_REG_ALARM_LOW_VOLTAGE:
	case LC709203F_REG_IC_POWER_MODE:
	case LC709203F_REG_STATUS_BIT:
	case LC709203F_REG_NUM_PARAMETER:
		val = data->regs[reg];
		break;
	default:
		LOG_ERR("Unknown or write only register 0x%x read", reg);
		return -EIO;
	}

	sys_put_le16(val, buf);

	uint8_t crc_buf[5];

	/* Build buffer for CRC calculation */
	crc_buf[0] = lc709203f_emul_cfg->addr << 1;
	crc_buf[1] = reg;
	crc_buf[2] = (lc709203f_emul_cfg->addr << 1) | 0x01;
	crc_buf[3] = buf[0]; /* LSB */
	crc_buf[4] = buf[1]; /* MSB */

	/* Calculate CRC and write it into the receive buffer */
	buf[2] = crc8(crc_buf, sizeof(crc_buf), LC709203F_CRC_POLYNOMIAL, 0, false);

	return 0;
}

static int lc709203f_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs,
				       int num_msgs, int addr)
{
	int reg;

	__ASSERT_NO_MSG(msgs && num_msgs);

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);
	switch (num_msgs) {
	case 1:
		if (msgs->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}

		if (msgs->len == 4) {
			return emul_lc709203f_reg_write(target, msgs->buf, msgs->len);
		}

		LOG_ERR("Unexpected msg length %d", msgs->len);
		return -EIO;

	case 2:
		if (msgs->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}
		if (msgs->len != 1) {
			LOG_ERR("Unexpected msg0 length %d", msgs->len);
			return -EIO;
		}
		reg = msgs->buf[0];

		/* Now process the 'read' part of the message */
		msgs++;
		if (msgs->flags & I2C_MSG_READ) {
			if (msgs->len == 3) {
				return emul_lc709203f_reg_read(target, reg, msgs->buf, msgs->len);
			}

			LOG_ERR("Unexpected msg length %d", msgs->len);
			return -EIO;
		}
		LOG_ERR("Second message must be an I2C write");
		return -EIO;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return 0;
}
/* The I2C emulator API required by Zephyr. */
static const struct i2c_emul_api lc709203f_emul_api_i2c = {
	.transfer = lc709203f_emul_transfer_i2c,
};

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>

/* Add test reset handlers in when using emulators with tests */
#define LC709203F_EMUL_RESET_RULE_BEFORE(inst) lc709203f_emul_reset(EMUL_DT_GET(DT_DRV_INST(inst)));

static void lc709203f_gauge_reset_rule_after(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);

	DT_INST_FOREACH_STATUS_OKAY(LC709203F_EMUL_RESET_RULE_BEFORE)
}
ZTEST_RULE(lc709203f_gauge_reset, NULL, lc709203f_gauge_reset_rule_after);
#endif /* CONFIG_ZTEST */

/**
 * Set up a new emulator (I2C)
 *
 * @param emul Emulation information
 * @param parent Device to emulate
 * @return 0 indicating success (always)
 */
static int lc709203f_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);
	lc709203f_emul_reset(target);
	return 0;
}

/*
 * Main instantiation macro.
 */
#define DEFINE_LC709203F_EMUL(n)                                                                   \
	static struct lc709203f_emul_data lc709203f_emul_data_##n;                                 \
	static const struct lc709203f_emul_cfg lc709203f_emul_cfg_##n = {                          \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, lc709203f_emul_init, &lc709203f_emul_data_##n,                      \
			    &lc709203f_emul_cfg_##n, &lc709203f_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(DEFINE_LC709203F_EMUL);
