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

/*
 * q31 shifts: chosen so 2^shift comfortably covers this driver's worst-case
 * physical value across all supported models (accel_scale_num/gyro_scale_num
 * are runtime, not compile-time, so a single fixed shift is used instead of a
 * per-model table). Accel: ADIS16470 at 1.25 mg/LSB, full int16 range, is
 * ~402 m/s^2 (2^9 = 512). Gyro: gyro_scale_num=10000 (2000 dps class), full
 * int16 range, is ~57 rad/s (2^7 = 128). Temp: realistic operating range is
 * well under +-256 degC (2^8), though the 0.1 degC/LSB conversion can exceed
 * that at extreme raw values that don't occur in practice.
 */
#define ADIS1647X_ACCEL_SHIFT 9
#define ADIS1647X_GYRO_SHIFT  7
#define ADIS1647X_TEMP_SHIFT  8

static inline void adis1647x_accel_convert_q31(q31_t *out, int16_t raw, uint8_t accel_scale_num)
{
	int64_t micro_m_s2 =
		((int64_t)raw * SENSOR_G * accel_scale_num) / ADIS1647X_ACCEL_SCALE_DEN;

	*out = (q31_t)((micro_m_s2 * ((int64_t)1 << 31))
			/ (1000000LL * ((int64_t)1 << ADIS1647X_ACCEL_SHIFT)));
}

static inline void adis1647x_gyro_convert_q31(q31_t *out, int16_t raw, uint16_t gyro_scale_num)
{
	int64_t micro_rad_s =
		((int64_t)raw * SENSOR_PI * gyro_scale_num) / ADIS1647X_GYRO_SCALE_DEN;

	*out = (q31_t)((micro_rad_s * ((int64_t)1 << 31))
			/ (1000000LL * ((int64_t)1 << ADIS1647X_GYRO_SHIFT)));
}

static inline void adis1647x_temp_convert_q31(q31_t *out, int16_t raw)
{
	int64_t micro_c = (int64_t)raw * 100000; /* 0.1 degC/LSB, no bias */

	*out = (q31_t)((micro_c * ((int64_t)1 << 31))
			/ (1000000LL * ((int64_t)1 << ADIS1647X_TEMP_SHIFT)));
}

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
	const struct adis1647x_burst_data *burst_data = &data->burst_data;
	uint64_t now_ns = k_ticks_to_ns_floor64(k_uptime_ticks());

	if (*fit > 0) {
		return -ENOTSUP;
	}

	if (chan_spec.chan_type == SENSOR_CHAN_DIE_TEMP) {
		struct sensor_q31_data *out = (struct sensor_q31_data *)data_out;

		memset(out, 0, sizeof(*out));
		out->header.base_timestamp_ns = now_ns;
		out->header.reading_count = 1;
		out->shift = ADIS1647X_TEMP_SHIFT;
		adis1647x_temp_convert_q31(&out->readings[0].temperature,
					   (int16_t)sys_be16_to_cpu(burst_data->temp_out));
		*fit = 1;
		return 1;
	}

	struct sensor_three_axis_data *out = (struct sensor_three_axis_data *)data_out;

	memset(out, 0, sizeof(*out));
	out->header.base_timestamp_ns = now_ns;
	out->header.reading_count = 1;

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
		out->shift = ADIS1647X_ACCEL_SHIFT;
		adis1647x_accel_convert_q31(&out->readings[0].x,
					    (int16_t)sys_be16_to_cpu(burst_data->x_accel_out),
					    data->accel_scale_num);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		out->shift = ADIS1647X_ACCEL_SHIFT;
		adis1647x_accel_convert_q31(&out->readings[0].y,
					    (int16_t)sys_be16_to_cpu(burst_data->y_accel_out),
					    data->accel_scale_num);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		out->shift = ADIS1647X_ACCEL_SHIFT;
		adis1647x_accel_convert_q31(&out->readings[0].z,
					    (int16_t)sys_be16_to_cpu(burst_data->z_accel_out),
					    data->accel_scale_num);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		out->shift = ADIS1647X_ACCEL_SHIFT;
		adis1647x_accel_convert_q31(&out->readings[0].x,
					    (int16_t)sys_be16_to_cpu(burst_data->x_accel_out),
					    data->accel_scale_num);
		adis1647x_accel_convert_q31(&out->readings[0].y,
					    (int16_t)sys_be16_to_cpu(burst_data->y_accel_out),
					    data->accel_scale_num);
		adis1647x_accel_convert_q31(&out->readings[0].z,
					    (int16_t)sys_be16_to_cpu(burst_data->z_accel_out),
					    data->accel_scale_num);
		break;
	case SENSOR_CHAN_GYRO_X:
		out->shift = ADIS1647X_GYRO_SHIFT;
		adis1647x_gyro_convert_q31(&out->readings[0].x,
					   (int16_t)sys_be16_to_cpu(burst_data->x_gyro_out),
					   data->gyro_scale_num);
		break;
	case SENSOR_CHAN_GYRO_Y:
		out->shift = ADIS1647X_GYRO_SHIFT;
		adis1647x_gyro_convert_q31(&out->readings[0].y,
					   (int16_t)sys_be16_to_cpu(burst_data->y_gyro_out),
					   data->gyro_scale_num);
		break;
	case SENSOR_CHAN_GYRO_Z:
		out->shift = ADIS1647X_GYRO_SHIFT;
		adis1647x_gyro_convert_q31(&out->readings[0].z,
					   (int16_t)sys_be16_to_cpu(burst_data->z_gyro_out),
					   data->gyro_scale_num);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		out->shift = ADIS1647X_GYRO_SHIFT;
		adis1647x_gyro_convert_q31(&out->readings[0].x,
					   (int16_t)sys_be16_to_cpu(burst_data->x_gyro_out),
					   data->gyro_scale_num);
		adis1647x_gyro_convert_q31(&out->readings[0].y,
					   (int16_t)sys_be16_to_cpu(burst_data->y_gyro_out),
					   data->gyro_scale_num);
		adis1647x_gyro_convert_q31(&out->readings[0].z,
					   (int16_t)sys_be16_to_cpu(burst_data->z_gyro_out),
					   data->gyro_scale_num);
		break;
	default:
		return -ENOTSUP;
	}

	*fit = 1;

	return 1;
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

static int adis1647x_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					   size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
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

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adis1647x_decoder_get_frame_count,
	.get_size_info = adis1647x_decoder_get_size_info,
	.decode = adis1647x_decoder_decode,
	.has_trigger = adis1647x_decoder_has_trigger,
};

int adis1647x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
