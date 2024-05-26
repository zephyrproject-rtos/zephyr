/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT asahi_kasei_akm09918c

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "akm09918c.h"
#include "akm09918c_emul.h"
#include "akm09918c_reg.h"

LOG_MODULE_DECLARE(AKM09918C, CONFIG_SENSOR_LOG_LEVEL);

#define NUM_REGS AKM09918C_REG_TS2

struct akm09918c_emul_data {
	uint8_t reg[NUM_REGS];
};

struct akm09918c_emul_cfg {
};

void akm09918c_emul_set_reg(const struct emul *target, uint8_t reg_addr, const uint8_t *val,
			    size_t count)
{
	struct akm09918c_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr + count < NUM_REGS);
	memcpy(data->reg + reg_addr, val, count);
}

void akm09918c_emul_get_reg(const struct emul *target, uint8_t reg_addr, uint8_t *val, size_t count)
{
	struct akm09918c_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr + count < NUM_REGS);
	memcpy(val, data->reg + reg_addr, count);
}

void akm09918c_emul_reset(const struct emul *target)
{
	struct akm09918c_emul_data *data = target->data;

	memset(data->reg, 0, NUM_REGS);
	data->reg[AKM09918C_REG_WIA1] = AKM09918C_WIA1;
	data->reg[AKM09918C_REG_WIA2] = AKM09918C_WIA2;
}

static int akm09918c_emul_handle_write(const struct emul *target, uint8_t regn, uint8_t value)
{
	struct akm09918c_emul_data *data = target->data;

	switch (regn) {
	case AKM09918C_REG_CNTL2:
		data->reg[AKM09918C_REG_CNTL2] = value;
		break;
	case AKM09918C_REG_CNTL3:
		if (FIELD_GET(AKM09918C_CNTL3_SRST, value) == 1) {
			akm09918c_emul_reset(target);
		}
		break;
	}
	return 0;
}

static int akm09918c_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs,
				       int num_msgs, int addr)
{
	struct akm09918c_emul_data *data = target->data;

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	if (num_msgs < 1) {
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}
	if (FIELD_GET(I2C_MSG_READ, msgs->flags)) {
		LOG_ERR("Unexpected read");
		return -EIO;
	}
	if (msgs->len < 1) {
		LOG_ERR("Unexpected msg0 length %d", msgs->len);
		return -EIO;
	}

	uint8_t regn = msgs->buf[0];
	bool is_read = FIELD_GET(I2C_MSG_READ, msgs->flags) == 1;
	bool is_stop = FIELD_GET(I2C_MSG_STOP, msgs->flags) == 1;

	if (!is_stop && !is_read) {
		/* First message was a write with the register number, check next message */
		msgs++;
		is_read = FIELD_GET(I2C_MSG_READ, msgs->flags) == 1;
		is_stop = FIELD_GET(I2C_MSG_STOP, msgs->flags) == 1;
	}
	if (is_read) {
		/* Read data */
		uint8_t mode = data->reg[AKM09918C_REG_CNTL2];

		for (int i = 0; i < msgs->len; ++i) {
			msgs->buf[i] = data->reg[regn + i];
			if (regn + i == AKM09918C_REG_TMPS &&
			    mode == AKM09918C_CNTL2_SINGLE_MEASURE) {
				/* Reading the TMPS register clears the DRDY bit */
				data->reg[AKM09918C_REG_ST1] = 0;
			}
		}
	} else {
		/* Write data */
		int rc = akm09918c_emul_handle_write(target, regn, msgs->buf[1]);

		if (rc != 0) {
			return rc;
		}
	}

	return 0;
}

static int akm09918c_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);
	akm09918c_emul_reset(target);

	return 0;
}

static int akm09918c_emul_backend_set_channel(const struct emul *target, struct sensor_chan_spec ch,
					      const q31_t *value, int8_t shift)
{
	if (!target || !target->data) {
		return -EINVAL;
	}

	struct akm09918c_emul_data *data = target->data;
	uint8_t reg;

	switch (ch.chan_type) {
	case SENSOR_CHAN_MAGN_X:
		reg = AKM09918C_REG_HXL;
		break;
	case SENSOR_CHAN_MAGN_Y:
		reg = AKM09918C_REG_HYL;
		break;
	case SENSOR_CHAN_MAGN_Z:
		reg = AKM09918C_REG_HZL;
		break;
	/* This function only supports setting single channels, so skip MAGN_XYZ */
	default:
		return -ENOTSUP;
	}

	/* Set the ST1 register to show we have data */
	data->reg[AKM09918C_REG_ST1] |= AKM09918C_ST1_DRDY;

	/* Convert fixed-point Gauss values into microgauss and then into its bit representation */
	int32_t microgauss =
		(shift < 0 ? ((int64_t)*value >> -shift) : ((int64_t)*value << shift)) * 1000000 /
		((int64_t)INT32_MAX + 1);

	int16_t reg_val =
		CLAMP(microgauss, AKM09918C_MAGN_MIN_MICRO_GAUSS, AKM09918C_MAGN_MAX_MICRO_GAUSS) /
		AKM09918C_MICRO_GAUSS_PER_BIT;

	/* Insert reading into registers */
	data->reg[reg] = reg_val & 0xFF;
	data->reg[reg + 1] = (reg_val >> 8) & 0xFF;

	return 0;
}

static int akm09918c_emul_backend_get_sample_range(const struct emul *target,
						   struct sensor_chan_spec ch, q31_t *lower,
						   q31_t *upper, q31_t *epsilon, int8_t *shift)
{
	ARG_UNUSED(target);

	if (!lower || !upper || !epsilon || !shift) {
		return -EINVAL;
	}

	switch (ch.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
		/* +/- 49.12 Gs is the measurement range. 0.0015 Gs is the granularity */
		*shift = 6;
		*upper = (int64_t)(49.12 * ((int64_t)INT32_MAX + 1)) >> *shift;
		*lower = -*upper;
		*epsilon = (int64_t)(0.0015 * ((int64_t)INT32_MAX + 1)) >> *shift;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct i2c_emul_api akm09918c_emul_api_i2c = {
	.transfer = akm09918c_emul_transfer_i2c,
};

static const struct emul_sensor_driver_api akm09918c_emul_sensor_driver_api = {
	.set_channel = akm09918c_emul_backend_set_channel,
	.get_sample_range = akm09918c_emul_backend_get_sample_range,
};

#define AKM09918C_EMUL(n)                                                                          \
	const struct akm09918c_emul_cfg akm09918c_emul_cfg_##n;                                    \
	struct akm09918c_emul_data akm09918c_emul_data_##n;                                        \
	EMUL_DT_INST_DEFINE(n, akm09918c_emul_init, &akm09918c_emul_data_##n,                      \
			    &akm09918c_emul_cfg_##n, &akm09918c_emul_api_i2c,                      \
			    &akm09918c_emul_sensor_driver_api)

DT_INST_FOREACH_STATUS_OKAY(AKM09918C_EMUL)
