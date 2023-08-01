/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm42688

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/logging/log.h>

#include <icm42688_reg.h>

LOG_MODULE_DECLARE(ICM42688, CONFIG_SENSOR_LOG_LEVEL);

#define NUM_REGS (UINT8_MAX >> 1)

struct icm42688_emul_data {
	uint8_t reg[NUM_REGS];
};

struct icm42688_emul_cfg {
};

void icm42688_emul_set_reg(const struct emul *target, uint8_t reg_addr, const uint8_t *val,
			   size_t count)
{
	struct icm42688_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr + count < NUM_REGS);
	memcpy(data->reg + reg_addr, val, count);
}

void icm42688_emul_get_reg(const struct emul *target, uint8_t reg_addr, uint8_t *val, size_t count)
{
	struct icm42688_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr + count < NUM_REGS);
	memcpy(val, data->reg + reg_addr, count);
}

static void icm42688_emul_handle_write(const struct emul *target, uint8_t regn, uint8_t value)
{
	struct icm42688_emul_data *data = target->data;

	switch (regn) {
	case REG_DEVICE_CONFIG:
		if (FIELD_GET(BIT_SOFT_RESET, value) == 1) {
			/* Perform a soft reset */
			memset(data->reg, 0, NUM_REGS);
			/* Initialized the who-am-i register */
			data->reg[REG_WHO_AM_I] = WHO_AM_I_ICM42688;
			/* Set the bit for the reset being done */
			data->reg[REG_INT_STATUS] |= BIT_INT_STATUS_RESET_DONE;
		}
		break;
	}
}

static int icm42688_emul_io_spi(const struct emul *target, const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	struct icm42688_emul_data *data = target->data;
	const struct spi_buf *tx, *rx;
	uint8_t regn;
	bool is_read;

	ARG_UNUSED(config);
	__ASSERT_NO_MSG(tx_bufs != NULL);

	tx = tx_bufs->buffers;
	__ASSERT_NO_MSG(tx != NULL);
	__ASSERT_NO_MSG(tx->len > 0);

	regn = *(uint8_t *)tx->buf;
	is_read = FIELD_GET(REG_SPI_READ_BIT, regn);
	regn &= GENMASK(6, 0);
	if (is_read) {
		__ASSERT_NO_MSG(rx_bufs != NULL);
		__ASSERT_NO_MSG(rx_bufs->count > 1);

		rx = &rx_bufs->buffers[1];
		__ASSERT_NO_MSG(rx->buf != NULL);
		__ASSERT_NO_MSG(rx->len > 0);
		for (uint16_t i = 0; i < rx->len; ++i) {
			((uint8_t *)rx->buf)[i] = data->reg[regn + i];
		}
	} else {
		/* Writing to regn */
		uint8_t value;

		__ASSERT_NO_MSG(tx_bufs->count > 1);
		tx = &tx_bufs->buffers[1];

		__ASSERT_NO_MSG(tx->len > 0);
		value = ((uint8_t *)tx->buf)[0];
		icm42688_emul_handle_write(target, regn, value);
	}

	return 0;
}

static int icm42688_emul_init(const struct emul *target, const struct device *parent)
{
	struct icm42688_emul_data *data = target->data;

	/* Initialized the who-am-i register */
	data->reg[REG_WHO_AM_I] = WHO_AM_I_ICM42688;

	return 0;
}

static const struct spi_emul_api icm42688_emul_spi_api = {
	.io = icm42688_emul_io_spi,
};

#define Q31_SCALE ((int64_t)INT32_MAX + 1)

/**
 * @brief Get current full-scale range in g's based on register config, along with corresponding
 *        sensitivity and shift. See datasheet section 3.2, table 2.
 */
static void icm42688_emul_get_accel_settings(const struct emul *target, int *fs_g, int *sensitivity,
					     int8_t *shift)
{
	uint8_t reg;

	int sensitivity_out, fs_g_out;
	int8_t shift_out;

	icm42688_emul_get_reg(target, REG_ACCEL_CONFIG0, &reg, 1);

	switch ((reg & MASK_ACCEL_UI_FS_SEL) >> 5) {
	case BIT_ACCEL_UI_FS_16:
		fs_g_out = 16;
		sensitivity_out = 2048;
		/* shift is based on `fs_g * 9.8` since the final numbers will be in SI units of
		 * m/s^2, not g's
		 */
		shift_out = 8;
		break;
	case BIT_ACCEL_UI_FS_8:
		fs_g_out = 8;
		sensitivity_out = 4096;
		shift_out = 7;
		break;
	case BIT_ACCEL_UI_FS_4:
		fs_g_out = 4;
		sensitivity_out = 8192;
		shift_out = 6;
		break;
	case BIT_ACCEL_UI_FS_2:
		fs_g_out = 2;
		sensitivity_out = 16384;
		shift_out = 5;
		break;
	default:
		__ASSERT_UNREACHABLE;
	}

	if (fs_g) {
		*fs_g = fs_g_out;
	}
	if (sensitivity) {
		*sensitivity = sensitivity_out;
	}
	if (shift) {
		*shift = shift_out;
	}
}

/**
 * @brief Helper function for calculating accelerometer ranges. Considers the current full-scale
 *        register config (i.e. +/-2g, +/-4g, etc...)
 */
static void icm42688_emul_get_accel_ranges(const struct emul *target, q31_t *lower, q31_t *upper,
					   q31_t *epsilon, int8_t *shift)
{
	int fs_g;
	int sensitivity;

	icm42688_emul_get_accel_settings(target, &fs_g, &sensitivity, shift);

	/* Epsilon is equal to 1.5 bit-counts worth of error. */
	*epsilon = (3 * SENSOR_G * Q31_SCALE / sensitivity / 1000000LL / 2) >> *shift;
	*upper = (fs_g * SENSOR_G * Q31_SCALE / 1000000LL) >> *shift;
	*lower = -*upper;
}

/**
 * @brief Get current full-scale gyro range in milli-degrees per second based on register config,
 *        along with corresponding sensitivity and shift. See datasheet section 3.1, table 1.
 */
static void icm42688_emul_get_gyro_settings(const struct emul *target, int *fs_mdps,
					    int *sensitivity, int8_t *shift)
{
	uint8_t reg;

	int sensitivity_out, fs_mdps_out;
	int8_t shift_out;

	icm42688_emul_get_reg(target, REG_GYRO_CONFIG0, &reg, 1);

	switch ((reg & MASK_GYRO_UI_FS_SEL) >> 5) {
	case BIT_GYRO_UI_FS_2000:
		/* Milli-degrees per second */
		fs_mdps_out = 2000000;
		/* 10x LSBs/deg/s */
		sensitivity_out = 164;
		/* Shifts are based on rad/s: `(fs_mdps * pi / 180 / 1000)` */
		shift_out = 6; /* +/- 34.90659 */
		break;
	case BIT_GYRO_UI_FS_1000:
		fs_mdps_out = 1000000;
		sensitivity_out = 328;
		shift_out = 5; /* +/- 17.44444 */
		break;
	case BIT_GYRO_UI_FS_500:
		fs_mdps_out = 500000;
		sensitivity_out = 655;
		shift_out = 4; /* +/- 8.72222 */
		break;
	case BIT_GYRO_UI_FS_250:
		fs_mdps_out = 250000;
		sensitivity_out = 1310;
		shift_out = 3; /* +/- 4.36111 */
		break;
	case BIT_GYRO_UI_FS_125:
		fs_mdps_out = 125000;
		sensitivity_out = 2620;
		shift_out = 2; /* +/- 2.18055 */
		break;
	case BIT_GYRO_UI_FS_62_5:
		fs_mdps_out = 62500;
		sensitivity_out = 5243;
		shift_out = 1; /* +/- 1.09027 */
		break;
	case BIT_GYRO_UI_FS_31_25:
		fs_mdps_out = 31250;
		sensitivity_out = 10486;
		shift_out = 0; /* +/- 0.54513 */
		break;
	case BIT_GYRO_UI_FS_15_625:
		fs_mdps_out = 15625;
		sensitivity_out = 20972;
		shift_out = -1; /* +/- 0.27256 */
		break;
	default:
		__ASSERT_UNREACHABLE;
	}

	if (fs_mdps) {
		*fs_mdps = fs_mdps_out;
	}
	if (sensitivity) {
		*sensitivity = sensitivity_out;
	}
	if (shift) {
		*shift = shift_out;
	}
}

/**
 * @brief Helper function for calculating gyroscope ranges. Considers the current full-scale
 *        register config
 */
static void icm42688_emul_get_gyro_ranges(const struct emul *target, q31_t *lower, q31_t *upper,
					  q31_t *epsilon, int8_t *shift)
{
	/* millidegrees/second */
	int fs_mdps;
	/* 10x LSBs per degrees/second*/
	int sensitivity;

	icm42688_emul_get_gyro_settings(target, &fs_mdps, &sensitivity, shift);

	/* Reduce the actual range of gyroscope values. Some full-scale ranges actually exceed the
	 * size of an int16 by a small margin. For example, FS_SEL=0 has a +/-2000 deg/s range with
	 * 16.4 bits/deg/s sensitivity (Section 3.1, Table 1). This works out to register values of
	 * +/-2000 * 16.4 = +/-32800. This will cause the expected value to get clipped when
	 * setting the register and throw off the actual reading. Therefore, scale down the range
	 * to 99% to avoid the top and bottom edges.
	 */

	fs_mdps *= 0.99;

	/* Epsilon is equal to 1.5 bit-counts worth of error. */
	*epsilon = (3 * SENSOR_PI * Q31_SCALE * 10LL / 1000000LL / 180LL / sensitivity / 2LL) >>
		   *shift;
	*upper = (((fs_mdps * SENSOR_PI / 1000000LL) * Q31_SCALE) / 1000LL / 180LL) >> *shift;
	*lower = -*upper;
}

static int icm42688_emul_backend_get_sample_range(const struct emul *target, enum sensor_channel ch,
						  q31_t *lower, q31_t *upper, q31_t *epsilon,
						  int8_t *shift)
{
	if (!lower || !upper || !epsilon || !shift) {
		return -EINVAL;
	}

	switch (ch) {
	case SENSOR_CHAN_DIE_TEMP:
		/* degrees C = ([16-bit signed temp_data register] / 132.48) + 25 */
		*shift = 9;
		*lower = (int64_t)(-222.342995169 * Q31_SCALE) >> *shift;
		*upper = (int64_t)(272.33544686 * Q31_SCALE) >> *shift;
		*epsilon = (int64_t)(0.0076 * Q31_SCALE) >> *shift;
		break;
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		icm42688_emul_get_accel_ranges(target, lower, upper, epsilon, shift);
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		icm42688_emul_get_gyro_ranges(target, lower, upper, epsilon, shift);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int icm42688_emul_backend_set_channel(const struct emul *target, enum sensor_channel ch,
					     q31_t value, int8_t shift)
{
	if (!target || !target->data) {
		return -EINVAL;
	}

	struct icm42688_emul_data *data = target->data;

	int sensitivity;
	uint8_t reg_addr;
	int32_t reg_val;
	int64_t value_unshifted =
		shift < 0 ? ((int64_t)value >> -shift) : ((int64_t)value << shift);

	switch (ch) {
	case SENSOR_CHAN_DIE_TEMP:
		reg_addr = REG_TEMP_DATA1;
		reg_val = ((value_unshifted - (25 * Q31_SCALE)) * 13248) / (100 * Q31_SCALE);
		break;
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		switch (ch) {
		case SENSOR_CHAN_ACCEL_X:
			reg_addr = REG_ACCEL_DATA_X1;
			break;
		case SENSOR_CHAN_ACCEL_Y:
			reg_addr = REG_ACCEL_DATA_Y1;
			break;
		case SENSOR_CHAN_ACCEL_Z:
			reg_addr = REG_ACCEL_DATA_Z1;
			break;
		default:
			__ASSERT_UNREACHABLE;
		}
		icm42688_emul_get_accel_settings(target, NULL, &sensitivity, NULL);
		reg_val = ((value_unshifted * sensitivity / Q31_SCALE) * 1000000LL) / SENSOR_G;
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		switch (ch) {
		case SENSOR_CHAN_GYRO_X:
			reg_addr = REG_GYRO_DATA_X1;
			break;
		case SENSOR_CHAN_GYRO_Y:
			reg_addr = REG_GYRO_DATA_Y1;
			break;
		case SENSOR_CHAN_GYRO_Z:
			reg_addr = REG_GYRO_DATA_Z1;
			break;
		default:
			__ASSERT_UNREACHABLE;
		}
		icm42688_emul_get_gyro_settings(target, NULL, &sensitivity, NULL);
		reg_val =
			CLAMP((((value_unshifted * sensitivity * 180LL) / Q31_SCALE) * 1000000LL) /
				      SENSOR_PI / 10LL,
			      INT16_MIN, INT16_MAX);
		break;
	default:
		return -ENOTSUP;
	}

	data->reg[reg_addr] = (reg_val >> 8) & 0xFF;
	data->reg[reg_addr + 1] = reg_val & 0xFF;

	/* Set data ready flag */
	data->reg[REG_INT_STATUS] |= BIT_INT_STATUS_DATA_RDY;

	return 0;
}

static const struct emul_sensor_backend_api icm42688_emul_sensor_backend_api = {
	.set_channel = icm42688_emul_backend_set_channel,
	.get_sample_range = icm42688_emul_backend_get_sample_range,
};

#define ICM42688_EMUL_DEFINE(n, api)                                                               \
	EMUL_DT_INST_DEFINE(n, icm42688_emul_init, &icm42688_emul_data_##n,                        \
			    &icm42688_emul_cfg_##n, &api, &icm42688_emul_sensor_backend_api)

#define ICM42688_EMUL_SPI(n)                                                                       \
	static struct icm42688_emul_data icm42688_emul_data_##n;                                   \
	static const struct icm42688_emul_cfg icm42688_emul_cfg_##n;                               \
	ICM42688_EMUL_DEFINE(n, icm42688_emul_spi_api)

DT_INST_FOREACH_STATUS_OKAY(ICM42688_EMUL_SPI)
