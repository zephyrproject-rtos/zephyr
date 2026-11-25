/* ST Microelectronics LIS2DW12 3-axis accelerometer driver
 *
 * Copyright (c) 2026 Sloc GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2dw12

#include <zephyr/drivers/sensor.h>

#include "lis2dw12.h"
#include "lis2dw12_async.h"

#define LIS2DW12_ACCEL_Q31_SHIFT 8
#define LIS2DW12_TEMP_Q31_SHIFT  9

static q31_t lis2dw12_accel_to_q31(int16_t raw, uint16_t gain)
{
	int64_t micro_ms2;

	/* gain is expressed in ug/LSB; convert the sample to micro-m/s^2 first. */
	micro_ms2 = (int64_t)raw * gain * SENSOR_G / 1000000LL;

	return (q31_t)(micro_ms2 * (int64_t)BIT64(31 - LIS2DW12_ACCEL_Q31_SHIFT) / 1000000LL);
}

static q31_t lis2dw12_temp_to_q31(int16_t raw)
{
	int64_t micro_celsius;

	micro_celsius = ((raw >> LIS2DW12_SHIFT_TEMP) * LIS2DW12_TEMP_SCALE_FACTOR) + 25000000LL;

	return (q31_t)(micro_celsius * (int64_t)BIT64(31 - LIS2DW12_TEMP_Q31_SHIFT) / 1000000LL);
}

static int lis2dw12_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	const struct lis2dw12_encoded_data *data = (const struct lis2dw12_encoded_data *)buffer;

	if (chan_spec.chan_idx != 0U) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		*frame_count = data->has_accel ? 1U : 0U;
		return 0;
	case SENSOR_CHAN_DIE_TEMP:
		*frame_count = data->has_temp ? 1U : 0U;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int lis2dw12_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					  size_t *frame_size)
{
	if (chan_spec.chan_idx != 0U) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	case SENSOR_CHAN_DIE_TEMP:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int lis2dw12_decode_accel(const struct lis2dw12_encoded_data *data,
				 struct sensor_chan_spec chan_spec, void *data_out)
{
	struct sensor_three_axis_data *out = data_out;

	if (!data->has_accel) {
		return -ENODATA;
	}

	out->header.base_timestamp_ns = data->timestamp_ns;
	out->header.reading_count = 1;
	out->shift = LIS2DW12_ACCEL_Q31_SHIFT;
	out->readings[0].timestamp_delta = 0;
	out->readings[0].x = 0;
	out->readings[0].y = 0;
	out->readings[0].z = 0;

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
		out->readings[0].x = lis2dw12_accel_to_q31(data->accel[0], data->gain);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		out->readings[0].y = lis2dw12_accel_to_q31(data->accel[1], data->gain);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		out->readings[0].z = lis2dw12_accel_to_q31(data->accel[2], data->gain);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		out->readings[0].x = lis2dw12_accel_to_q31(data->accel[0], data->gain);
		out->readings[0].y = lis2dw12_accel_to_q31(data->accel[1], data->gain);
		out->readings[0].z = lis2dw12_accel_to_q31(data->accel[2], data->gain);
		break;
	default:
		return -ENOTSUP;
	}

	return 1;
}

static int lis2dw12_decode_temp(const struct lis2dw12_encoded_data *data, void *data_out)
{
	struct sensor_q31_data *out = data_out;

	if (!data->has_temp) {
		return -ENODATA;
	}

	out->header.base_timestamp_ns = data->timestamp_ns;
	out->header.reading_count = 1;
	out->shift = LIS2DW12_TEMP_Q31_SHIFT;
	out->readings[0].timestamp_delta = 0;
	out->readings[0].temperature = lis2dw12_temp_to_q31(data->temp);

	return 1;
}

static int lis2dw12_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct lis2dw12_encoded_data *data = (const struct lis2dw12_encoded_data *)buffer;
	int ret;

	if ((max_count == 0U) || (*fit != 0U)) {
		return 0;
	}

	if (chan_spec.chan_idx != 0U) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		ret = lis2dw12_decode_accel(data, chan_spec, data_out);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		ret = lis2dw12_decode_temp(data, data_out);
		break;
	default:
		return -ENOTSUP;
	}

	if (ret > 0) {
		*fit = 1;
	}

	return ret;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = lis2dw12_decoder_get_frame_count,
	.get_size_info = lis2dw12_decoder_get_size_info,
	.decode = lis2dw12_decoder_decode,
};

int lis2dw12_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);

	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
