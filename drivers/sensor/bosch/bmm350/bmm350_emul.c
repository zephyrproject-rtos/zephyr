/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bmm350

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "bmm350.h"
#include "bmm350_emul.h"

LOG_MODULE_DECLARE(BMM350, CONFIG_SENSOR_LOG_LEVEL);

/* Size of the emulated register file (highest register is CMD at 0x7E). */
#define BMM350_EMUL_NUM_REGS 0x80

/*
 * BMM350 I2C reads return two dummy bytes ahead of the addressed register data.
 */
#define BMM350_EMUL_READ_DUMMY_BYTES 2

struct bmm350_emul_data {
	uint8_t reg[BMM350_EMUL_NUM_REGS];
};

struct bmm350_emul_cfg {
};

static void bmm350_emul_reset(const struct emul *target)
{
	struct bmm350_emul_data *data = target->data;

	memset(data->reg, 0, sizeof(data->reg));
	data->reg[BMM350_REG_CHIP_ID] = BMM350_CHIP_ID;
}

static void bmm350_emul_set_reg24(struct bmm350_emul_data *data, uint8_t reg, int32_t val)
{
	data->reg[reg] = (uint8_t)(val & 0xFF);
	data->reg[reg + 1] = (uint8_t)((val >> 8) & 0xFF);
	data->reg[reg + 2] = (uint8_t)((val >> 16) & 0xFF);
}

void bmm350_emul_set_mag_raw(const struct emul *target, int32_t x, int32_t y, int32_t z)
{
	struct bmm350_emul_data *data = target->data;

	bmm350_emul_set_reg24(data, BMM350_REG_MAG_X_XLSB, x);
	bmm350_emul_set_reg24(data, BMM350_REG_MAG_Y_XLSB, y);
	bmm350_emul_set_reg24(data, BMM350_REG_MAG_Z_XLSB, z);
}

void bmm350_emul_set_temp_raw(const struct emul *target, int32_t temp)
{
	struct bmm350_emul_data *data = target->data;

	bmm350_emul_set_reg24(data, BMM350_REG_TEMP_XLSB, temp);
}

static void bmm350_emul_handle_write(const struct emul *target, uint8_t reg, uint8_t val)
{
	struct bmm350_emul_data *data = target->data;

	switch (reg) {
	case BMM350_REG_CMD:
		if (val == BMM350_CMD_SOFTRESET) {
			bmm350_emul_reset(target);
		}
		break;
	case BMM350_REG_OTP_CMD_REG:
		data->reg[reg] = val;
		/* Mark the OTP command complete; OTP words read back as zero. */
		data->reg[BMM350_REG_OTP_STATUS_REG] = BMM350_OTP_STATUS_CMD_DONE;
		data->reg[BMM350_REG_OTP_DATA_MSB_REG] = 0;
		data->reg[BMM350_REG_OTP_DATA_LSB_REG] = 0;
		break;
	case BMM350_REG_TMR_SELFTEST_USER: {
		/* Model a self-test excitation response on the selected axis. */
		const int32_t st_raw = 30000;

		data->reg[reg] = val;
		switch (val) {
		case BMM350_SELF_TEST_POS_X:
			bmm350_emul_set_reg24(data, BMM350_REG_MAG_X_XLSB, st_raw);
			break;
		case BMM350_SELF_TEST_NEG_X:
			bmm350_emul_set_reg24(data, BMM350_REG_MAG_X_XLSB, -st_raw);
			break;
		case BMM350_SELF_TEST_POS_Y:
			bmm350_emul_set_reg24(data, BMM350_REG_MAG_Y_XLSB, st_raw);
			break;
		case BMM350_SELF_TEST_NEG_Y:
			bmm350_emul_set_reg24(data, BMM350_REG_MAG_Y_XLSB, -st_raw);
			break;
		default:
			break;
		}
		break;
	}
	case BMM350_REG_PMU_CMD: {
		/* Reflect the issued command back through PMU_CMD_STATUS_0. */
		uint8_t pmu_value = val & 0x07;
		uint8_t status;

		if (val == BMM350_PMU_CMD_BR_FAST) {
			pmu_value = BMM350_PMU_CMD_STATUS_0_BR_FAST;
		}
		status = pmu_value << BMM350_PMU_CMD_VALUE_POS;

		if (val == BMM350_PMU_CMD_NM) {
			status |= BMM350_PWR_MODE_IS_NORMAL_MSK;
		}

		data->reg[reg] = val;
		data->reg[BMM350_REG_PMU_CMD_STATUS_0] = status;
		break;
	}
	default:
		data->reg[reg] = val;
		break;
	}
}

static int bmm350_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				    int addr)
{
	struct bmm350_emul_data *data = target->data;

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	if (num_msgs < 1) {
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}
	if ((msgs[0].flags & I2C_MSG_READ) || msgs[0].len < 1) {
		LOG_ERR("Unexpected first message");
		return -EIO;
	}

	uint8_t reg = msgs[0].buf[0];

	if (num_msgs == 1) {
		/* Write transaction: [reg, data...]. */
		for (uint32_t i = 1; i < msgs[0].len; i++) {
			bmm350_emul_handle_write(target, reg + (i - 1), msgs[0].buf[i]);
		}
		return 0;
	}

	/* Read transaction: register write followed by a read. */
	struct i2c_msg *rd = &msgs[1];

	if (!(rd->flags & I2C_MSG_READ)) {
		LOG_ERR("Expected a read message");
		return -EIO;
	}

	for (uint32_t i = 0; i < rd->len; i++) {
		if (i < BMM350_EMUL_READ_DUMMY_BYTES) {
			rd->buf[i] = 0;
			continue;
		}

		uint8_t a = reg + (i - BMM350_EMUL_READ_DUMMY_BYTES);

		rd->buf[i] = (a < BMM350_EMUL_NUM_REGS) ? data->reg[a] : 0;
	}

	return 0;
}

static int bmm350_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	bmm350_emul_reset(target);

	return 0;
}

static const struct i2c_emul_api bmm350_emul_api_i2c = {
	.transfer = bmm350_emul_transfer_i2c,
};

#define BMM350_EMUL_DEFINE(n)                                                                      \
	static const struct bmm350_emul_cfg bmm350_emul_cfg_##n;                                   \
	static struct bmm350_emul_data bmm350_emul_data_##n;                                       \
	EMUL_DT_INST_DEFINE(n, bmm350_emul_init, &bmm350_emul_data_##n, &bmm350_emul_cfg_##n,      \
			    &bmm350_emul_api_i2c, NULL)

/*
 * Only the I2C transport is emulated. I3C bus instances also report as being on
 * the I2C bus (I3C is backward compatible), so they are excluded explicitly.
 */
#define BMM350_EMUL(n) COND_CODE_1(DT_INST_ON_BUS(n, i3c), (), (BMM350_EMUL_DEFINE(n)))

DT_INST_FOREACH_STATUS_OKAY(BMM350_EMUL)
