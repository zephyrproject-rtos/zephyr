/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm42688_decoder.h"
#include "icm42688_reg.h"
#include "icm42688.h"
#include <errno.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM42688_DECODER, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT invensense_icm42688

static int icm42688_get_shift(enum sensor_channel channel, int accel_fs, int gyro_fs, int8_t *shift)
{
	switch (channel) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		switch (accel_fs) {
		case ICM42688_ACCEL_FS_2G:
			*shift = 5;
			return 0;
		case ICM42688_ACCEL_FS_4G:
			*shift = 6;
			return 0;
		case ICM42688_ACCEL_FS_8G:
			*shift = 7;
			return 0;
		case ICM42688_ACCEL_FS_16G:
			*shift = 8;
			return 0;
		default:
			return -EINVAL;
		}
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		switch (gyro_fs) {
		case ICM42688_GYRO_FS_15_625:
			*shift = -1;
			return 0;
		case ICM42688_GYRO_FS_31_25:
			*shift = 0;
			return 0;
		case ICM42688_GYRO_FS_62_5:
			*shift = 1;
			return 0;
		case ICM42688_GYRO_FS_125:
			*shift = 2;
			return 0;
		case ICM42688_GYRO_FS_250:
			*shift = 3;
			return 0;
		case ICM42688_GYRO_FS_500:
			*shift = 4;
			return 0;
		case ICM42688_GYRO_FS_1000:
			*shift = 5;
			return 0;
		case ICM42688_GYRO_FS_2000:
			*shift = 6;
			return 0;
		default:
			return -EINVAL;
		}
	case SENSOR_CHAN_DIE_TEMP:
		*shift = 9;
		return 0;
	default:
		return -EINVAL;
	}
}

int icm42688_convert_raw_to_q31(struct icm42688_cfg *cfg, enum sensor_channel chan, int32_t reading,
				q31_t *out)
{
	int32_t whole;
	int32_t fraction;
	int64_t intermediate;
	int8_t shift;
	int rc;

	rc = icm42688_get_shift(chan, cfg->accel_fs, cfg->gyro_fs, &shift);
	if (rc != 0) {
		return rc;
	}

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		icm42688_accel_ms(cfg, reading, &whole, &fraction);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		icm42688_gyro_rads(cfg, reading, &whole, &fraction);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm42688_temp_c(reading, &whole, &fraction);
		break;
	default:
		return -ENOTSUP;
	}
	intermediate = ((int64_t)whole * INT64_C(1000000) + fraction);
	if (shift < 0) {
		intermediate =
			intermediate * ((int64_t)INT32_MAX + 1) * (1 << -shift) / INT64_C(1000000);
	} else if (shift > 0) {
		intermediate =
			intermediate * ((int64_t)INT32_MAX + 1) / ((1 << shift) * INT64_C(1000000));
	}
	*out = CLAMP(intermediate, INT32_MIN, INT32_MAX);

	return 0;
}

static int icm42688_get_channel_position(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP:
		return 0;
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
		return 1;
	case SENSOR_CHAN_ACCEL_Y:
		return 2;
	case SENSOR_CHAN_ACCEL_Z:
		return 3;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
		return 4;
	case SENSOR_CHAN_GYRO_Y:
		return 5;
	case SENSOR_CHAN_GYRO_Z:
		return 6;
	default:
		return 0;
	}
}

static uint8_t icm42688_encode_channel(enum sensor_channel chan)
{
	uint8_t encode_bmask = 0;

	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		encode_bmask = BIT(icm42688_get_channel_position(chan));
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		encode_bmask = BIT(icm42688_get_channel_position(SENSOR_CHAN_ACCEL_X)) |
			       BIT(icm42688_get_channel_position(SENSOR_CHAN_ACCEL_Y)) |
			       BIT(icm42688_get_channel_position(SENSOR_CHAN_ACCEL_Z));
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		encode_bmask = BIT(icm42688_get_channel_position(SENSOR_CHAN_GYRO_X)) |
			       BIT(icm42688_get_channel_position(SENSOR_CHAN_GYRO_Y)) |
			       BIT(icm42688_get_channel_position(SENSOR_CHAN_GYRO_Z));
		break;
	default:
		break;
	}

	return encode_bmask;
}

int icm42688_encode(const struct device *dev, const enum sensor_channel *const channels,
		    const size_t num_channels, uint8_t *buf)
{
	struct icm42688_dev_data *data = dev->data;
	struct icm42688_encoded_data *edata = (struct icm42688_encoded_data *)buf;

	edata->channels = 0;

	for (int i = 0; i < num_channels; i++) {
		edata->channels |= icm42688_encode_channel(channels[i]);
	}

	edata->header.is_fifo = false;
	edata->header.accel_fs = data->cfg.accel_fs;
	edata->header.gyro_fs = data->cfg.gyro_fs;
	edata->header.timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());

	return 0;
}

static int icm42688_one_shot_decode(const uint8_t *buffer, enum sensor_channel channel,
				   size_t channel_idx, uint32_t *fit,
				   uint16_t max_count, void *data_out)
{
	const struct icm42688_encoded_data *edata = (const struct icm42688_encoded_data *)buffer;
	const struct icm42688_decoder_header *header = &edata->header;
	struct icm42688_cfg cfg = {
		.accel_fs = edata->header.accel_fs,
		.gyro_fs = edata->header.gyro_fs,
	};
	uint8_t channel_request;
	int rc;

	if (*fit != 0) {
		return 0;
	}
	if (max_count == 0 || channel_idx != 0) {
		return -EINVAL;
	}

	switch (channel) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ: {
		channel_request = icm42688_encode_channel(SENSOR_CHAN_ACCEL_XYZ);
		if ((channel_request & edata->channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		rc = icm42688_get_shift(SENSOR_CHAN_ACCEL_XYZ, header->accel_fs, header->gyro_fs,
					&out->shift);
		if (rc != 0) {
			return -EINVAL;
		}

		icm42688_convert_raw_to_q31(
			&cfg, SENSOR_CHAN_ACCEL_X,
			edata->readings[icm42688_get_channel_position(SENSOR_CHAN_ACCEL_X)],
			&out->readings[0].x);
		icm42688_convert_raw_to_q31(
			&cfg, SENSOR_CHAN_ACCEL_Y,
			edata->readings[icm42688_get_channel_position(SENSOR_CHAN_ACCEL_Y)],
			&out->readings[0].y);
		icm42688_convert_raw_to_q31(
			&cfg, SENSOR_CHAN_ACCEL_Z,
			edata->readings[icm42688_get_channel_position(SENSOR_CHAN_ACCEL_Z)],
			&out->readings[0].z);
		*fit = 1;
		return 1;
	}
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ: {
		channel_request = icm42688_encode_channel(SENSOR_CHAN_GYRO_XYZ);
		if ((channel_request & edata->channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		rc = icm42688_get_shift(SENSOR_CHAN_GYRO_XYZ, header->accel_fs, header->gyro_fs,
					&out->shift);
		if (rc != 0) {
			return -EINVAL;
		}

		out->readings[0].timestamp_delta = 0;
		icm42688_convert_raw_to_q31(
			&cfg, SENSOR_CHAN_GYRO_X,
			edata->readings[icm42688_get_channel_position(SENSOR_CHAN_GYRO_X)],
			&out->readings[0].x);
		icm42688_convert_raw_to_q31(
			&cfg, SENSOR_CHAN_GYRO_Y,
			edata->readings[icm42688_get_channel_position(SENSOR_CHAN_GYRO_Y)],
			&out->readings[0].y);
		icm42688_convert_raw_to_q31(
			&cfg, SENSOR_CHAN_GYRO_Z,
			edata->readings[icm42688_get_channel_position(SENSOR_CHAN_GYRO_Z)],
			&out->readings[0].z);
		*fit = 1;
		return 1;
	}
	case SENSOR_CHAN_DIE_TEMP: {
		channel_request = icm42688_encode_channel(SENSOR_CHAN_DIE_TEMP);
		if ((channel_request & edata->channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		rc = icm42688_get_shift(SENSOR_CHAN_DIE_TEMP, header->accel_fs, header->gyro_fs,
					&out->shift);
		if (rc != 0) {
			return -EINVAL;
		}
		out->readings[0].timestamp_delta = 0;
		icm42688_convert_raw_to_q31(
			&cfg, SENSOR_CHAN_DIE_TEMP,
			edata->readings[icm42688_get_channel_position(SENSOR_CHAN_DIE_TEMP)],
			&out->readings[0].temperature);
		*fit = 1;
		return 1;
	}
	default:
		return -EINVAL;
	}
}

static int icm42688_decoder_decode(const uint8_t *buffer, enum sensor_channel channel,
				   size_t channel_idx, uint32_t *fit,
				   uint16_t max_count, void *data_out)
{
	return icm42688_one_shot_decode(buffer, channel, channel_idx, fit, max_count, data_out);
}

static int icm42688_decoder_get_frame_count(const uint8_t *buffer, enum sensor_channel channel,
					    size_t channel_idx, uint16_t *frame_count)
{
	ARG_UNUSED(buffer);
	if (channel_idx != 0) {
		return -ENOTSUP;
	}
	switch (channel) {
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
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int icm42688_decoder_get_size_info(enum sensor_channel channel, size_t *base_size,
					  size_t *frame_size)
{
	switch (channel) {
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
	.get_frame_count = icm42688_decoder_get_frame_count,
	.get_size_info = icm42688_decoder_get_size_info,
	.decode = icm42688_decoder_decode,
};

int icm42688_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
