/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ltc4286

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "ltc4286.h"
#include "ltc4286_emul.h"

LOG_MODULE_REGISTER(ltc4286_emul, CONFIG_SENSOR_LOG_LEVEL);

#define LTC4286_EMUL_REG_COUNT 256

struct ltc4286_emul_data {
	uint8_t regs[LTC4286_EMUL_REG_COUNT];
	uint8_t current_reg;
	bool reg_addr_set;
};

int ltc4286_emul_set_register(void *emul_data, uint8_t reg_addr, uint16_t value)
{
	struct ltc4286_emul_data *data = emul_data;

	if (reg_addr >= LTC4286_EMUL_REG_COUNT) {
		return -EINVAL;
	}

	/* Store as little-endian */
	data->regs[reg_addr] = FIELD_PREP(GENMASK(7, 0), value);
	if (reg_addr + 1 < LTC4286_EMUL_REG_COUNT) {
		data->regs[reg_addr + 1] =
			FIELD_PREP(GENMASK(7, 0), FIELD_GET(GENMASK(15, 8), value));
	}

	return 0;
}

int ltc4286_emul_get_register(void *emul_data, uint8_t reg_addr, uint16_t *value)
{
	struct ltc4286_emul_data *data = emul_data;

	if (reg_addr >= LTC4286_EMUL_REG_COUNT || !value) {
		return -EINVAL;
	}

	/* Read as little-endian */
	*value = data->regs[reg_addr];
	if (reg_addr + 1 < LTC4286_EMUL_REG_COUNT) {
		*value |= FIELD_PREP(GENMASK(15, 8), data->regs[reg_addr + 1]);
	}

	return 0;
}

void ltc4286_emul_reset(const struct emul *target)
{
	struct ltc4286_emul_data *data = target->data;

	memset(data->regs, 0, sizeof(data->regs));
	data->current_reg = 0;
	data->reg_addr_set = false;
}

static int ltc4286_emul_write_i2c(struct ltc4286_emul_data *data, struct i2c_msg *msgs)
{
	if ((msgs[0].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
		if (msgs[0].len < 1) {
			return -EINVAL;
		}

		/* Check for extended alert mask (0xFE prefix) */
		if (msgs[0].buf[0] == LTC4286_CMD_ALERT_MASK && msgs[0].len >= 2) {
			/* Extended command with 2-byte command code */
			data->current_reg = msgs[0].buf[1];
		} else {
			data->current_reg = msgs[0].buf[0];
		}
		data->reg_addr_set = true;

		/* If this is a write operation with data */
		if (msgs[0].len > 1) {
			uint8_t start_idx = (msgs[0].buf[0] == LTC4286_CMD_ALERT_MASK) ? 2 : 1;

			for (int i = start_idx; i < msgs[0].len; i++) {
				if (data->current_reg + (i - start_idx) < LTC4286_EMUL_REG_COUNT) {
					data->regs[data->current_reg + (i - start_idx)] =
						msgs[0].buf[i];
				}
			}
		}
	}

	return 0;
}

static int ltc4286_emul_read_i2c(struct ltc4286_emul_data *data, struct i2c_msg *msgs)
{
	if ((msgs[1].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
		if (!data->reg_addr_set) {
			return -EINVAL;
		}

		for (int i = 0; i < msgs[1].len; i++) {
			if (data->current_reg + i < LTC4286_EMUL_REG_COUNT) {
				msgs[1].buf[i] = data->regs[data->current_reg + i];
			} else {
				msgs[1].buf[i] = 0;
			}
		}
	}

	return 0;
}

static int ltc4286_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				     int addr)
{
	struct ltc4286_emul_data *data = target->data;
	int ret;

	if (num_msgs < 1) {
		return -EINVAL;
	}

	/* First message should be a write with the register address */
	ret = ltc4286_emul_write_i2c(data, msgs);
	if (ret) {
		return ret;
	}

	/* If there's a second message, it should be a read */
	if (num_msgs > 1) {
		ret = ltc4286_emul_read_i2c(data, msgs);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static const struct i2c_emul_api ltc4286_emul_api = {
	.transfer = ltc4286_emul_transfer_i2c,
};

static int ltc4286_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	ltc4286_emul_reset(target);

	return 0;
}

#define LTC4286_EMUL_DEFINE(n)                                                                     \
	static struct ltc4286_emul_data ltc4286_emul_data_##n;                                     \
	EMUL_DT_INST_DEFINE(n, ltc4286_emul_init, &ltc4286_emul_data_##n, NULL, &ltc4286_emul_api, \
			    NULL)

DT_INST_FOREACH_STATUS_OKAY(LTC4286_EMUL_DEFINE)
