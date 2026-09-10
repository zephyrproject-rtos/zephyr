/*
 * Copyright (c) 2026, Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adt7420

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "adt7420_emul.h"

LOG_MODULE_DECLARE(ADT7420, CONFIG_SENSOR_LOG_LEVEL);

#define ADT7420_NUM_REGS ADT7420_REG_RESET + 1

struct adt7420_emul_data {
	uint8_t regs[ADT7420_NUM_REGS];
	bool resolution_16_bit;
};

struct adt7420_emul_cfg {
};

void adt7420_emul_set_reg(const struct emul *target, uint8_t reg,
			  const uint8_t *val)
{
	struct adt7420_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg <= ADT7420_NUM_REGS);
	memcpy(&data->regs[reg], val, 1);
}

void adt7420_emul_get_reg(const struct emul *target, uint8_t reg, uint8_t *val)
{
	struct adt7420_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg <= ADT7420_NUM_REGS);
	memcpy(val, &data->regs[reg], 1);
}

void adt7420_emul_reset(const struct emul *target)
{
	struct adt7420_emul_data *data = target->data;

	memset(data->regs, 0, ADT7420_NUM_REGS);
	data->resolution_16_bit = false;
}

static int adt7420_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				     int addr)
{
	struct adt7420_emul_data *data = target->data;
	uint8_t regn = msgs->buf[0];

	/* Dump into log */
	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	/* Validate message count and structure */
	if (num_msgs < 1) {
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	if (msgs->len < 1) {
		LOG_ERR("Unexpected msg0 length: %d", msgs->len);
		return -EIO;
	}

	/* identify transaction type for MSG0 */
	bool is_read = FIELD_GET(I2C_MSG_READ, msgs->flags) == 1;
	bool is_stop = FIELD_GET(I2C_MSG_STOP, msgs->flags) == 1;

	if (!is_stop && !is_read) {
		msgs++;
		is_read = FIELD_GET(I2C_MSG_READ, msgs->flags) == 1;
		is_stop = FIELD_GET(I2C_MSG_STOP, msgs->flags) == 1;
	}

	/* Perform transaction into mock registers */
	if (is_read) {
		for (int i = 0; i < msgs->len; i++) {
			msgs->buf[i] = data->regs[regn + i];
		}
	} else { /* is_write */
		for (int i = 1; i < msgs->len; i++) {
			data->regs[regn + (i - 1)] = msgs->buf[i];
		}
	}

	return 0;
}

static int adt7420_emul_init(const struct emul *target, const struct device *parent)
{
	struct adt7420_emul_data *data = target->data;

	ARG_UNUSED(parent);
	adt7420_emul_reset(target);

	/* Seed ID register so the driver's ID check passes on init */
	data->regs[ADT7420_REG_ID] = ADT7420_DEFAULT_ID;

	return 0;
}

static int adt7420_emul_set_channel(const struct emul *target, struct sensor_chan_spec ch,
				    const q31_t *value, int8_t shift)
{
	struct adt7420_emul_data *data = target->data;

	if (ch.chan_type != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/* Set READY status bit to low to indicate data ready */
	data->regs[ADT7420_REG_STATUS] &= ~ADT7420_STATUS_RDY;

	/* Convert q31 to millicelsius: physical = (q31 << shift) / BIT(31) */
	int64_t scaled_value = (int64_t)*value << shift;
	int64_t millicelsius = scaled_value * 1000 / BIT(31);

	uint16_t code;

	if (data->resolution_16_bit) {
		/* 16-bit: 1 LSB = 1/128 °C = 7.8125 mC */
		int16_t reg_value = (int16_t)(millicelsius * 128 / 1000);

		code = (uint16_t)reg_value;
	} else {
		/* 13-bit: 1 LSB = 1/16 °C = 62.5 mC, data in bits [15:3] */
		int16_t reg_value = (int16_t)(millicelsius * 16 / 1000);

		code = (uint16_t)(reg_value << 3);
	}

	data->regs[ADT7420_REG_TEMP_MSB] = FIELD_GET(GENMASK(15, 8), code);
	data->regs[ADT7420_REG_TEMP_LSB] = FIELD_GET(GENMASK(7, 0), code);

	return 0;
}

static int adt7420_emul_get_sample_range(const struct emul *target, struct sensor_chan_spec ch,
					 q31_t *lower, q31_t *upper, q31_t *epsilon, int8_t *shift)
{
	struct adt7420_emul_data *data = target->data;
	const struct device *dev = target->dev;
	const struct adt7420_dev_config *cfg = dev->config;

	if (!lower || !upper || !epsilon || !shift) {
		return -EINVAL;
	}

	if (ch.chan_type != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/* max supported temp int is 150C. 2^8 (256) covers this range. */
	*shift = 8;
	*lower = ((int64_t)cfg->min_temp * BIT(31)) >> *shift;
	*upper = ((int64_t)cfg->max_temp * BIT(31)) >> *shift;

	if (data->resolution_16_bit) {
		/* 16-bit: 1/128C per LSB */
		*epsilon = (BIT(31) / 128) >> *shift;
	} else {
		/* 13-bit: 1/16C per LSB */
		*epsilon = (BIT(31) / 16) >> *shift;
	}

	return 0;
}

static const struct i2c_emul_api adt7420_emul_api_i2c = {
	.transfer = adt7420_emul_transfer_i2c,
};

static const struct emul_sensor_driver_api adt7420_emul_backend_api = {
	.set_channel = adt7420_emul_set_channel,
	.get_sample_range = adt7420_emul_get_sample_range,
};

#define ADT7420_EMUL_DEFINE(inst)                                                                  \
	const struct adt7420_emul_cfg adt7420_emul_cfg_##inst;                                     \
	struct adt7420_emul_data adt7420_emul_data_##inst;                                         \
	EMUL_DT_INST_DEFINE(inst, adt7420_emul_init, &adt7420_emul_data_##inst,                    \
			    &adt7420_emul_cfg_##inst, &adt7420_emul_api_i2c,                       \
			    &adt7420_emul_backend_api);

DT_INST_FOREACH_STATUS_OKAY(ADT7420_EMUL_DEFINE)
