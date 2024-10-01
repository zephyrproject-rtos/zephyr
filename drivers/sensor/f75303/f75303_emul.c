/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fintek_f75303

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor/f75303.h>
#include "f75303.h"

LOG_MODULE_DECLARE(F75303, CONFIG_SENSOR_LOG_LEVEL);

#define NUM_REGS 128

struct f75303_emul_data {
	uint8_t reg[NUM_REGS];
};

struct f75303_emul_cfg {
};

static void f75303_emul_set_reg(const struct emul *target, uint8_t reg, uint8_t val)
{
	struct f75303_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg < NUM_REGS);
	data->reg[reg] = val;
}

static uint8_t f75303_emul_get_reg(const struct emul *target, uint8_t reg)
{
	struct f75303_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg < NUM_REGS);
	return data->reg[reg];
}

static void f75303_emul_reset(const struct emul *target)
{
	struct f75303_emul_data *data = target->data;

	memset(data->reg, 0, NUM_REGS);
}

static int f75303_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs,
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
				val = f75303_emul_get_reg(target, reg);
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
			f75303_emul_set_reg(target, reg, msgs->buf[0]);
		}
		break;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return 0;
}

static int f75303_emul_init(const struct emul *target, const struct device *parent)
{
	f75303_emul_reset(target);
	return 0;
}

static int f75303_emul_set_channel(const struct emul *target, struct sensor_chan_spec ch,
				   const q31_t *value, int8_t shift)
{
	struct f75303_emul_data *data = target->data;
	int64_t scaled_value;
	int32_t millicelsius;
	int32_t reg_value;
	uint8_t reg_h, reg_l;

	switch ((int32_t)ch.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		reg_h = F75303_LOCAL_TEMP_H;
		reg_l = F75303_LOCAL_TEMP_L;
		break;
	case SENSOR_CHAN_F75303_REMOTE1:
		reg_h = F75303_REMOTE1_TEMP_H;
		reg_l = F75303_REMOTE1_TEMP_L;
		break;
	case SENSOR_CHAN_F75303_REMOTE2:
		reg_h = F75303_REMOTE2_TEMP_H;
		reg_l = F75303_REMOTE2_TEMP_L;
		break;
	default:
		return -ENOTSUP;
	}

	scaled_value = (int64_t)*value << shift;
	millicelsius = scaled_value * 1000 / ((int64_t)INT32_MAX + 1);
	reg_value = CLAMP(millicelsius / 125, 0, 0x7ff);

	data->reg[reg_h] = reg_value >> 3;
	data->reg[reg_l] = (reg_value & 0x7) << 5;

	return 0;
}

static int f75303_emul_get_sample_range(const struct emul *target, struct sensor_chan_spec ch,
					q31_t *lower, q31_t *upper, q31_t *epsilon, int8_t *shift)
{
	if (ch.chan_type != SENSOR_CHAN_AMBIENT_TEMP &&
	    ch.chan_type != (enum sensor_channel)SENSOR_CHAN_F75303_REMOTE1 &&
	    ch.chan_type != (enum sensor_channel)SENSOR_CHAN_F75303_REMOTE2) {
		return -ENOTSUP;
	}

	*shift = 8;
	*lower = 0;
	*upper = (int64_t)(255.875 * ((int64_t)INT32_MAX + 1)) >> *shift;
	*epsilon = (int64_t)(0.125 * ((int64_t)INT32_MAX + 1)) >> *shift;

	return 0;
}

static const struct i2c_emul_api f75303_emul_api_i2c = {
	.transfer = f75303_emul_transfer_i2c,
};

static const struct emul_sensor_driver_api f75303_emul_api_sensor = {
	.set_channel = f75303_emul_set_channel,
	.get_sample_range = f75303_emul_get_sample_range,
};


#define F75303_EMUL(n)								\
	const struct f75303_emul_cfg f75303_emul_cfg_##n;			\
	struct f75303_emul_data f75303_emul_data_##n;				\
	EMUL_DT_INST_DEFINE(n, f75303_emul_init, &f75303_emul_data_##n,		\
			    &f75303_emul_cfg_##n, &f75303_emul_api_i2c,		\
			    &f75303_emul_api_sensor);

DT_INST_FOREACH_STATUS_OKAY(F75303_EMUL)
