/*
 * Copyright (c) 2026, Iomico
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp112

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "tmp112.h"
#include "tmp112_emul.h"

static int tmp112_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				int addr)
{
	struct tmp112_emul_data *data = target->data;
	uint16_t value;
	uint8_t reg;

	ARG_UNUSED(addr);

	if ((msgs == NULL) || (num_msgs < 1) || (num_msgs > 2) ||
	    ((msgs[0].flags & I2C_MSG_READ) != 0U) || (msgs[0].len < 1U)) {
		return -EIO;
	}

	reg = msgs[0].buf[0];
	if (reg >= ARRAY_SIZE(data->regs)) {
		return -EIO;
	}

	if (num_msgs == 2) {
		if ((msgs[0].len != 1U) || (msgs[1].len != sizeof(value)) ||
		    ((msgs[1].flags & I2C_MSG_READ) == 0U)) {
			return -EIO;
		}

		value = data->regs[reg];
		if (reg == TMP112_REG_CONFIG) {
			/* Conversions complete immediately; OS reads as ready. */
			value |= TMP112_ONE_SHOT;
		}
		sys_put_be16(value, msgs[1].buf);
		return 0;
	}

	if ((msgs[0].len != 3U) || (reg == TMP112_REG_TEMPERATURE)) {
		return -EIO;
	}

	value = sys_get_be16(&msgs[0].buf[1]);
	if (reg == TMP112_REG_CONFIG) {
		data->config_write_count++;
		if (data->fail_config_write) {
			return -EIO;
		}

		if ((value & (TMP112_CONFIG_SD | TMP112_ONE_SHOT)) ==
		    (TMP112_CONFIG_SD | TMP112_ONE_SHOT)) {
			data->one_shot_count++;
		}
		/* OS is a command/status bit, not part of the stored configuration. */
		value &= ~TMP112_ONE_SHOT;
	}
	data->regs[reg] = value;

	return 0;
}

static int tmp112_emul_init(const struct emul *target, const struct device *parent)
{
	struct tmp112_emul_data *data = target->data;

	ARG_UNUSED(parent);
	/* A valid 25 degree Celsius sample in the normal 12-bit format. */
	data->regs[TMP112_REG_TEMPERATURE] = 0x1900;

	return 0;
}

static const struct i2c_emul_api tmp112_emul_api = {
	.transfer = tmp112_emul_transfer,
};

#define TMP112_EMUL_DEFINE(inst)                                                                   \
	static struct tmp112_emul_data tmp112_emul_data_##inst;                                    \
	EMUL_DT_INST_DEFINE(inst, tmp112_emul_init, &tmp112_emul_data_##inst, NULL,                \
			    &tmp112_emul_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(TMP112_EMUL_DEFINE)
