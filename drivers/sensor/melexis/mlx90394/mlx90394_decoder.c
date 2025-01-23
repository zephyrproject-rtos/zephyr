/*
 * Copyright (c) 2024 Florian Weber <Florian.Weber@live.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mlx90394.h"
#include <zephyr/drivers/sensor.h>

#define DT_DRV_COMPAT melexis_mlx90394

static int mlx90394_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec channel,
					    uint16_t *frame_count)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(channel);

	/* This sensor lacks a FIFO; there will always only be one frame at a time. */
	*frame_count = 1;
	return 0;
}

static int mlx90394_decoder_get_size_info(struct sensor_chan_spec channel, size_t *base_size,
					  size_t *frame_size)
{
	switch (channel.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ: {
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
	} break;
	case SENSOR_CHAN_AMBIENT_TEMP: {
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
	} break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int mlx90394_convert_raw_magn_to_q31(int16_t reading, q31_t *out,
					    const enum mlx90394_reg_config_val config_val)
{
	int64_t intermediate;

	if (config_val == MLX90394_CTRL2_CONFIG_HIGH_SENSITIVITY_LOW_NOISE) {
		intermediate = ((int64_t)reading * MLX90394_HIGH_SENSITIVITY_MICRO_GAUSS_PER_BIT) *
			       ((int64_t)INT32_MAX + 1) /
			       ((1 << MLX90394_SHIFT_MAGN_HIGH_SENSITIVITY) * INT64_C(1000000));
	} else {
		intermediate = ((int64_t)reading * MLX90394_HIGH_RANGE_MICRO_GAUSS_PER_BIT) *
			       ((int64_t)INT32_MAX + 1) /
			       ((1 << MLX90394_SHIFT_MAGN_HIGH_RANGE) * INT64_C(1000000));
	}

	*out = CLAMP(intermediate, INT32_MIN, INT32_MAX);
	return 0;
}
static int mlx90394_convert_raw_temp_to_q31(int16_t reading, q31_t *out)
{

	int64_t intermediate = ((int64_t)reading * MLX90394_MICRO_CELSIUS_PER_BIT) *
			       ((int64_t)INT32_MAX + 1) /
			       ((1 << MLX90394_SHIFT_TEMP) * INT64_C(1000000));

	*out = CLAMP(intermediate, INT32_MIN, INT32_MAX);
	return 0;
}

static int mlx90394_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec channel,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct mlx90394_encoded_data *edata = (const struct mlx90394_encoded_data *)buffer;

	if (*fit != 0) {
		return 0;
	}

	switch (channel.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ: {
		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		if (edata->header.config_val == MLX90394_CTRL2_CONFIG_HIGH_SENSITIVITY_LOW_NOISE) {
			out->shift = MLX90394_SHIFT_MAGN_HIGH_SENSITIVITY;
		} else {
			out->shift = MLX90394_SHIFT_MAGN_HIGH_RANGE;
		}

		mlx90394_convert_raw_magn_to_q31(edata->readings[0], &out->readings[0].x,
						 edata->header.config_val);
		mlx90394_convert_raw_magn_to_q31(edata->readings[1], &out->readings[0].y,
						 edata->header.config_val);
		mlx90394_convert_raw_magn_to_q31(edata->readings[2], &out->readings[0].z,
						 edata->header.config_val);
		*fit = 1;
	} break;
	case SENSOR_CHAN_AMBIENT_TEMP: {
		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = MLX90394_SHIFT_TEMP;
		mlx90394_convert_raw_temp_to_q31(edata->readings[3], &out->readings[0].temperature);
		*fit = 1;
	} break;
	default:
		return -ENOTSUP;
	}
	return 1;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = mlx90394_decoder_get_frame_count,
	.get_size_info = mlx90394_decoder_get_size_info,
	.decode = mlx90394_decoder_decode,
};

int mlx90394_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
