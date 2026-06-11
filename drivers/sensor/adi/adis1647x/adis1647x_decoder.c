/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include "adis1647x.h"
/* 1000 converts from mg to g, 100 rescales accel_scale */
#define ADIS1647X_ACCEL_SCALE_DEN (1000LL * 100)
/* 100000 rescales gyro_scale, 180 for PI/180 rad->deg conversion */
#define ADIS1647X_GYRO_SCALE_DEN  (100000LL * 180)

void adis1647x_accel_convert(struct sensor_value *val, int16_t raw, uint8_t accel_scale_num)
{
	int64_t micro_m_s2 =
		((int64_t)raw * SENSOR_G * accel_scale_num) / ADIS1647X_ACCEL_SCALE_DEN;

	val->val1 = (int32_t)(micro_m_s2 / 1000000);
	val->val2 = (int32_t)(micro_m_s2 % 1000000);
}

void adis1647x_gyro_convert(struct sensor_value *val, int16_t raw, uint16_t gyro_scale_num)
{
	int64_t micro_rad_s =
		((int64_t)raw * SENSOR_PI * gyro_scale_num) / ADIS1647X_GYRO_SCALE_DEN;

	val->val1 = (int32_t)(micro_rad_s / 1000000);
	val->val2 = (int32_t)(micro_rad_s % 1000000);
}

static int adis1647x_decoder_get_frame_count(const uint8_t *buffer,
					     struct sensor_chan_spec chan_spec,
					     uint16_t *frame_count)
{
	int32_t ret = -ENOTSUP;

	if (chan_spec.chan_idx != 0) {
		return ret;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_DIE_TEMP:
		*frame_count = 1;
		ret = 0;
		break;

	default:
		break;
	}

	return ret;
}

static int adis1647x_decode_sample(const struct adis1647x_sample_data *data,
				   struct sensor_chan_spec chan_spec, uint32_t *fit,
				   uint16_t max_count, void *data_out)
{
	struct sensor_value *out = (struct sensor_value *)data_out;
	const struct adis1647x_burst_data *burst_data = &data->burst_data;

	if (*fit > 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
		adis1647x_accel_convert(out, (int16_t)sys_be16_to_cpu(burst_data->x_accel_out),
					data->accel_scale_num);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adis1647x_accel_convert(out, (int16_t)sys_be16_to_cpu(burst_data->y_accel_out),
					data->accel_scale_num);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adis1647x_accel_convert(out, (int16_t)sys_be16_to_cpu(burst_data->z_accel_out),
					data->accel_scale_num);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adis1647x_accel_convert(out++, (int16_t)sys_be16_to_cpu(burst_data->x_accel_out),
					data->accel_scale_num);
		adis1647x_accel_convert(out++, (int16_t)sys_be16_to_cpu(burst_data->y_accel_out),
					data->accel_scale_num);
		adis1647x_accel_convert(out, (int16_t)sys_be16_to_cpu(burst_data->z_accel_out),
					data->accel_scale_num);
		break;
	case SENSOR_CHAN_GYRO_X:
		adis1647x_gyro_convert(out, (int16_t)sys_be16_to_cpu(burst_data->x_gyro_out),
				       data->gyro_scale_num);
		break;
	case SENSOR_CHAN_GYRO_Y:
		adis1647x_gyro_convert(out, (int16_t)sys_be16_to_cpu(burst_data->y_gyro_out),
				       data->gyro_scale_num);
		break;
	case SENSOR_CHAN_GYRO_Z:
		adis1647x_gyro_convert(out, (int16_t)sys_be16_to_cpu(burst_data->z_gyro_out),
				       data->gyro_scale_num);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		adis1647x_gyro_convert(out++, (int16_t)sys_be16_to_cpu(burst_data->x_gyro_out),
				       data->gyro_scale_num);
		adis1647x_gyro_convert(out++, (int16_t)sys_be16_to_cpu(burst_data->y_gyro_out),
				       data->gyro_scale_num);
		adis1647x_gyro_convert(out, (int16_t)sys_be16_to_cpu(burst_data->z_gyro_out),
				       data->gyro_scale_num);
		break;
	case SENSOR_CHAN_DIE_TEMP: {
		int64_t micro_celsius =
			(int64_t)(int16_t)sys_be16_to_cpu(burst_data->temp_out) * 100000;

		out->val1 = (int32_t)(micro_celsius / 1000000);
		out->val2 = (int32_t)(micro_celsius % 1000000);
		break;
	}
	default:
		return -ENOTSUP;
	}

	*fit = 1;

	return 0;
}

static int adis1647x_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adis1647x_sample_data *sample_data =
		(const struct adis1647x_sample_data *)buffer;

	return adis1647x_decode_sample(sample_data, chan_spec, fit, max_count, data_out);
}

static bool adis1647x_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	return trigger == SENSOR_TRIG_DATA_READY;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adis1647x_decoder_get_frame_count,
	.decode = adis1647x_decoder_decode,
	.has_trigger = adis1647x_decoder_has_trigger,
};

int adis1647x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
