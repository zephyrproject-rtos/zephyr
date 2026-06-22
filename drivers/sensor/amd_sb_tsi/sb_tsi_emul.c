/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT amd_sb_tsi

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include "sb_tsi.h"

LOG_MODULE_DECLARE(AMD_SB_TSI, CONFIG_SENSOR_LOG_LEVEL);

#define NUM_REGS 128

struct sb_tsi_emul_data {
	uint8_t reg[NUM_REGS];
};

static void sb_tsi_emul_set_reg(const struct emul *target, uint8_t reg, uint8_t val)
{
	struct sb_tsi_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg < NUM_REGS);
	data->reg[reg] = val;
}

static uint8_t sb_tsi_emul_get_reg(const struct emul *target, uint8_t reg)
{
	struct sb_tsi_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg < NUM_REGS);
	return data->reg[reg];
}

static void sb_tsi_emul_reset(const struct emul *target)
{
	struct sb_tsi_emul_data *data = target->data;

	memset(data->reg, 0, NUM_REGS);
}

static int sb_tsi_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs,
				    int num_msgs, int addr)
{
	/* Largely copied from emul_bmi160.c */
	unsigned int val;
	int reg;

	__ASSERT_NO_MSG(msgs && num_msgs);

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);
	switch (num_msgs) {
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
			switch (msgs->len) {
			case 1:
				val = sb_tsi_emul_get_reg(target, reg);
				msgs->buf[0] = val;
				break;
			default:
				LOG_ERR("Unexpected msg1 length %d", msgs->len);
				return -EIO;
			}
		} else {
			if (msgs->len != 1) {
				LOG_ERR("Unexpected msg1 length %d", msgs->len);
			}
			sb_tsi_emul_set_reg(target, reg, msgs->buf[0]);
		}
		break;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return 0;
}

static int sb_tsi_emul_init(const struct emul *target, const struct device *parent)
{
	sb_tsi_emul_reset(target);
	return 0;
}

static int sb_tsi_emul_set_channel(const struct emul *target, struct sensor_chan_spec ch,
				   const q31_t *value, int8_t shift)
{
	struct sb_tsi_emul_data *data = target->data;
	int64_t scaled_value;
	int32_t millicelsius;
	int32_t reg_value;

	if (ch.chan_type != SENSOR_CHAN_AMBIENT_TEMP && ch.chan_idx != 0) {
		return -ENOTSUP;
	}

	scaled_value = (int64_t)*value << shift;
	millicelsius = scaled_value * 1000 / ((int64_t)INT32_MAX + 1);
	reg_value = CLAMP(millicelsius / 125, 0, 0x7ff);

	data->reg[SB_TSI_TEMP_INT] = reg_value >> 3;
	data->reg[SB_TSI_TEMP_DEC] = (reg_value & 0x7) << 5;

	return 0;
}

static int sb_tsi_emul_get_sample_range(const struct emul *target, struct sensor_chan_spec ch,
					q31_t *lower, q31_t *upper, q31_t *epsilon, int8_t *shift)
{
	if (ch.chan_type != SENSOR_CHAN_AMBIENT_TEMP || ch.chan_idx != 0) {
		return -ENOTSUP;
	}

	*shift = 8;
	*lower = 0;
	*upper = (int64_t)(255.875 * ((int64_t)INT32_MAX + 1)) >> *shift;
	*epsilon = (int64_t)(0.125 * ((int64_t)INT32_MAX + 1)) >> *shift;

	return 0;
}

static const struct i2c_emul_api sb_tsi_emul_api_i2c = {
	.transfer = sb_tsi_emul_transfer_i2c,
};

static const struct emul_sensor_driver_api sb_tsi_emul_api_sensor = {
	.set_channel = sb_tsi_emul_set_channel,
	.get_sample_range = sb_tsi_emul_get_sample_range,
};

#define SB_TSI_EMUL(n)								\
	struct sb_tsi_emul_data sb_tsi_emul_data_##n;				\
	EMUL_DT_INST_DEFINE(n, sb_tsi_emul_init, &sb_tsi_emul_data_##n,	NULL,	\
			    &sb_tsi_emul_api_i2c, &sb_tsi_emul_api_sensor)

DT_INST_FOREACH_STATUS_OKAY(SB_TSI_EMUL)
