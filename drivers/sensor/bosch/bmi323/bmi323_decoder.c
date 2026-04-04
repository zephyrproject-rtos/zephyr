/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/drivers/sensor.h>

#include "bmi323.h"

static int32_t bmi323_centi_to_q31(int32_t centi_deg, int shift)
{
	return (int32_t)(((int64_t)centi_deg * (1LL << (31 - shift))) / 100LL);
}

/* Q31 conversion for accel:
 * q31 = raw * range_g * SENSOR_G / 1000000 * 2^21 / 32768
 *     = raw * range_g * SENSOR_G / 1000000 * 64
 *     = raw * range_g * SENSOR_G / 15625
 */
static int32_t bmi323_accel_to_q31(int16_t raw, uint32_t range_g)
{
	return (int32_t)(((int64_t)raw * range_g * SENSOR_G) / 15625LL);
}

/* Q31 conversion for gyro (degrees to radians):
 * q31 = raw * range_dps * π / 180 * 2^21 / 32768
 *     = raw * range_dps * SENSOR_PI / 180 / 1000000 * 64
 *     = raw * range_dps * SENSOR_PI / 2812500
 */
static int32_t bmi323_gyro_to_q31(int16_t raw, uint32_t range_dps)
{
	return (int32_t)(((int64_t)raw * range_dps * SENSOR_PI) / 2812500LL);
}

/* Convert raw to centi-degrees C,
 * then to Q31 with shift=10:
 * q31 = centi_deg * 2^21 / 100
 */
static int32_t bmi323_temp_to_q31(int16_t raw_temp)
{
	int32_t centi_deg =
		(int32_t)(((int64_t)raw_temp *
		IMU_BOSCH_DIE_TEMP_MICRO_DEG_CELSIUS_LSB) +
		IMU_BOSCH_DIE_TEMP_OFFSET_MICRO_DEG_CELSIUS) /
		10000LL;

	return bmi323_centi_to_q31(centi_deg, BMI323_TEMP_SHIFT);
}

static int bmi323_decoder_get_frame_count(const uint8_t *buffer,
						struct sensor_chan_spec chan,
						uint16_t *frame_count)
{
	const struct bmi323_encoded_data *edata =
		(const struct bmi323_encoded_data *)buffer;

	if (chan.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (chan.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		*frame_count = edata->has_accel ? 1U : 0U;
		return 0;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		*frame_count = edata->has_gyro ? 1U : 0U;
		return 0;
	case SENSOR_CHAN_DIE_TEMP:
		*frame_count = edata->has_temp ? 1U : 0U;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int bmi323_decoder_get_size_info(struct sensor_chan_spec chan,
					size_t *base_size,
					size_t *frame_size)
{
	switch (chan.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_DIE_TEMP:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int bmi323_decoder_decode(const uint8_t *buffer,
				 struct sensor_chan_spec chan,
				 uint32_t *fit,
				 uint16_t max_count,
				 void *data_out)
{
	const struct bmi323_encoded_data *edata =
		(const struct bmi323_encoded_data *)buffer;

	if (*fit != 0U) {
		return 0;
	}

	if (max_count == 0U || chan.chan_idx != 0U) {
		return -EINVAL;
	}

	switch (chan.chan_type) {
	case SENSOR_CHAN_ACCEL_XYZ: {
		struct sensor_three_axis_data *out = data_out;

		if (!edata->has_accel) {
			return -ENODATA;
		}

		out->header.base_timestamp_ns    = edata->header.timestamp;
		out->header.reading_count        = 1U;
		out->shift                       = BMI323_ACCEL_SHIFT;
		out->readings[0].timestamp_delta = 0U;
		out->readings[0].x = bmi323_accel_to_q31(edata->reading.accel_x, edata->acce_range);
		out->readings[0].y = bmi323_accel_to_q31(edata->reading.accel_y, edata->acce_range);
		out->readings[0].z = bmi323_accel_to_q31(edata->reading.accel_z, edata->acce_range);
		break;
	}

	case SENSOR_CHAN_GYRO_XYZ: {
		struct sensor_three_axis_data *out = data_out;

		if (!edata->has_gyro) {
			return -ENODATA;
		}

		out->header.base_timestamp_ns    = edata->header.timestamp;
		out->header.reading_count        = 1U;
		out->shift                       = BMI323_GYRO_SHIFT;
		out->readings[0].timestamp_delta = 0U;
		out->readings[0].x = bmi323_gyro_to_q31(edata->reading.gyro_x, edata->gyro_range);
		out->readings[0].y = bmi323_gyro_to_q31(edata->reading.gyro_y, edata->gyro_range);
		out->readings[0].z = bmi323_gyro_to_q31(edata->reading.gyro_z, edata->gyro_range);
		break;
	}

	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z: {
		struct sensor_q31_data *out = data_out;
		int16_t raw;

		if (!edata->has_accel) {
			return -ENODATA;
		}

		if (chan.chan_type == SENSOR_CHAN_ACCEL_X) {
			raw = edata->reading.accel_x;
		} else if (chan.chan_type == SENSOR_CHAN_ACCEL_Y) {
			raw = edata->reading.accel_y;
		} else {
			raw = edata->reading.accel_z;
		}

		out->header.base_timestamp_ns       = edata->header.timestamp;
		out->header.reading_count           = 1U;
		out->readings[0].timestamp_delta    = 0U;
		out->shift                          = BMI323_ACCEL_SHIFT;
		out->readings[0].value = bmi323_accel_to_q31(raw, edata->acce_range);
		break;
	}

	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z: {
		struct sensor_q31_data *out = data_out;
		int16_t raw;

		if (!edata->has_gyro) {
			return -ENODATA;
		}

		if (chan.chan_type == SENSOR_CHAN_GYRO_X) {
			raw = edata->reading.gyro_x;
		} else if (chan.chan_type == SENSOR_CHAN_GYRO_Y) {
			raw = edata->reading.gyro_y;
		} else {
			raw = edata->reading.gyro_z;
		}

		out->header.base_timestamp_ns    = edata->header.timestamp;
		out->header.reading_count        = 1U;
		out->shift                       = BMI323_GYRO_SHIFT;
		out->readings[0].timestamp_delta = 0U;
		out->readings[0].value = bmi323_gyro_to_q31(raw, edata->gyro_range);
		break;
	}

	case SENSOR_CHAN_DIE_TEMP: {
		struct sensor_q31_data *out = data_out;

		if (!edata->has_temp) {
			return -ENODATA;
		}

		out->header.base_timestamp_ns    = edata->header.timestamp;
		out->header.reading_count        = 1U;
		out->shift                       = BMI323_TEMP_SHIFT;
		out->readings[0].timestamp_delta = 0U;
		out->readings[0].value =
			bmi323_temp_to_q31(edata->reading.temperature);
		break;
	}

	default:
		return -ENOTSUP;
	}

	*fit = 1U;
	return 1;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = bmi323_decoder_get_frame_count,
	.get_size_info = bmi323_decoder_get_size_info,
	.decode = bmi323_decoder_decode,
};

int bmi323_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
