/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm20689

#include <errno.h>
#include <zephyr/dt-bindings/sensor/icm20689.h>
#include <zephyr/sys/util.h>
#include "icm20689_decoder.h"

#define ICM20689_Q31_ACCEL_SHIFT_2G     5
#define ICM20689_Q31_ACCEL_SHIFT_4G     6
#define ICM20689_Q31_ACCEL_SHIFT_8G     7
#define ICM20689_Q31_ACCEL_SHIFT_16G    8
#define ICM20689_Q31_GYRO_SHIFT_250DPS  3
#define ICM20689_Q31_GYRO_SHIFT_500DPS  4
#define ICM20689_Q31_GYRO_SHIFT_1000DPS 5
#define ICM20689_Q31_GYRO_SHIFT_2000DPS 6
#define ICM20689_Q31_TEMP_SHIFT         9

#define ICM20689_RAW_COUNTS_PER_FS      32768LL
#define ICM20689_TEMP_SENSITIVITY_SCALE 10LL
#define ICM20689_TEMP_SENSITIVITY_X10   3268LL
#define ICM20689_TEMP_OFFSET_C          25LL

static int icm20689_channel_position(enum sensor_channel channel)
{
	switch (channel) {
	case SENSOR_CHAN_DIE_TEMP:
		return ICM20689_ENCODED_CHAN_DIE_TEMP;
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_XYZ:
		return ICM20689_ENCODED_CHAN_ACCEL_X;
	case SENSOR_CHAN_ACCEL_Y:
		return ICM20689_ENCODED_CHAN_ACCEL_Y;
	case SENSOR_CHAN_ACCEL_Z:
		return ICM20689_ENCODED_CHAN_ACCEL_Z;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_XYZ:
		return ICM20689_ENCODED_CHAN_GYRO_X;
	case SENSOR_CHAN_GYRO_Y:
		return ICM20689_ENCODED_CHAN_GYRO_Y;
	case SENSOR_CHAN_GYRO_Z:
		return ICM20689_ENCODED_CHAN_GYRO_Z;
	default:
		return -ENOTSUP;
	}
}

static uint8_t icm20689_channel_mask(enum sensor_channel channel)
{
	if (channel == SENSOR_CHAN_ACCEL_XYZ) {
		return BIT(ICM20689_ENCODED_CHAN_ACCEL_X) | BIT(ICM20689_ENCODED_CHAN_ACCEL_Y) |
		       BIT(ICM20689_ENCODED_CHAN_ACCEL_Z);
	}
	if (channel == SENSOR_CHAN_GYRO_XYZ) {
		return BIT(ICM20689_ENCODED_CHAN_GYRO_X) | BIT(ICM20689_ENCODED_CHAN_GYRO_Y) |
		       BIT(ICM20689_ENCODED_CHAN_GYRO_Z);
	}

	int position = icm20689_channel_position(channel);

	return position < 0 ? 0 : BIT(position);
}

static int icm20689_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec channel,
					    uint16_t *frame_count)
{
	const struct icm20689_encoded_data *data = (const void *)buffer;
	uint8_t mask;

	if (channel.chan_idx != 0) {
		return -ENOTSUP;
	}
	mask = icm20689_channel_mask(channel.chan_type);
	if (mask == 0) {
		return -ENOTSUP;
	}
	if ((data->channels & mask) != mask) {
		*frame_count = 0;
		return -ENODATA;
	}

	*frame_count = 1;
	return 0;
}

static int icm20689_decoder_get_size_info(struct sensor_chan_spec channel, size_t *base_size,
					  size_t *frame_size)
{
	if (channel.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (channel.chan_type) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
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
	default:
		return -ENOTSUP;
	}
}

static int8_t icm20689_get_shift(enum sensor_channel channel,
				 const struct icm20689_decoder_header *header)
{
	switch (channel) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		switch (header->accel_fs) {
		case ICM20689_DT_ACCEL_FS_2G:
			return ICM20689_Q31_ACCEL_SHIFT_2G;
		case ICM20689_DT_ACCEL_FS_4G:
			return ICM20689_Q31_ACCEL_SHIFT_4G;
		case ICM20689_DT_ACCEL_FS_8G:
			return ICM20689_Q31_ACCEL_SHIFT_8G;
		case ICM20689_DT_ACCEL_FS_16G:
		default:
			return ICM20689_Q31_ACCEL_SHIFT_16G;
		}
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		switch (header->gyro_fs) {
		case ICM20689_DT_GYRO_FS_250DPS:
			return ICM20689_Q31_GYRO_SHIFT_250DPS;
		case ICM20689_DT_GYRO_FS_500DPS:
			return ICM20689_Q31_GYRO_SHIFT_500DPS;
		case ICM20689_DT_GYRO_FS_1000DPS:
			return ICM20689_Q31_GYRO_SHIFT_1000DPS;
		case ICM20689_DT_GYRO_FS_2000DPS:
		default:
			return ICM20689_Q31_GYRO_SHIFT_2000DPS;
		}
	case SENSOR_CHAN_DIE_TEMP:
		return ICM20689_Q31_TEMP_SHIFT;
	default:
		return 0;
	}
}

static q31_t icm20689_convert(enum sensor_channel channel, int16_t raw,
			      const struct icm20689_decoder_header *header, int8_t shift)
{
	int64_t micro;

	if (channel >= SENSOR_CHAN_ACCEL_X && channel <= SENSOR_CHAN_ACCEL_XYZ) {
		micro = ((int64_t)raw * header->accel_fs * SENSOR_G) / ICM20689_RAW_COUNTS_PER_FS;
	} else if (channel >= SENSOR_CHAN_GYRO_X && channel <= SENSOR_CHAN_GYRO_XYZ) {
		micro = ((int64_t)raw * header->gyro_fs * SENSOR_PI) /
			(ICM20689_RAW_COUNTS_PER_FS * 180LL);
	} else {
		micro = ((int64_t)raw * ICM20689_TEMP_SENSITIVITY_SCALE * 1000000LL) /
				ICM20689_TEMP_SENSITIVITY_X10 +
			ICM20689_TEMP_OFFSET_C * 1000000LL;
	}

	return CLAMP((micro * ((int64_t)INT32_MAX + 1)) / (1000000LL * (1LL << shift)), INT32_MIN,
		     INT32_MAX);
}

static int icm20689_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec channel,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct icm20689_encoded_data *data = (const void *)buffer;
	int position;

	if (channel.chan_idx != 0 || max_count == 0) {
		return -EINVAL;
	}
	if (*fit != 0) {
		return 0;
	}

	uint8_t mask = icm20689_channel_mask(channel.chan_type);

	if (mask == 0) {
		return -ENOTSUP;
	}
	if ((data->channels & mask) != mask) {
		return -ENODATA;
	}

	int8_t shift = icm20689_get_shift(channel.chan_type, &data->header);

	if (channel.chan_type == SENSOR_CHAN_ACCEL_XYZ ||
	    channel.chan_type == SENSOR_CHAN_GYRO_XYZ) {
		struct sensor_three_axis_data *out = data_out;
		enum sensor_channel first = channel.chan_type == SENSOR_CHAN_ACCEL_XYZ
						    ? SENSOR_CHAN_ACCEL_X
						    : SENSOR_CHAN_GYRO_X;

		out->header.base_timestamp_ns = data->header.timestamp;
		out->header.reading_count = 1;
		out->shift = shift;
		q31_t *values[] = {&out->readings[0].x, &out->readings[0].y, &out->readings[0].z};

		for (int axis = 0; axis < 3; axis++) {
			const struct icm20689_axis_alignment *alignment =
				&data->header.axis_align[axis];
			int raw_position =
				(first == SENSOR_CHAN_ACCEL_X ? 1 : 4) + alignment->index;

			*values[axis] =
				alignment->sign * icm20689_convert(channel.chan_type,
								   data->readings[raw_position],
								   &data->header, shift);
		}
	} else {
		struct sensor_q31_data *out = data_out;

		position = icm20689_channel_position(channel.chan_type);
		if (channel.chan_type != SENSOR_CHAN_DIE_TEMP) {
			int axis = channel.chan_type >= SENSOR_CHAN_GYRO_X
					   ? channel.chan_type - SENSOR_CHAN_GYRO_X
					   : channel.chan_type - SENSOR_CHAN_ACCEL_X;
			position = (channel.chan_type >= SENSOR_CHAN_GYRO_X
					    ? ICM20689_ENCODED_CHAN_GYRO_X
					    : ICM20689_ENCODED_CHAN_ACCEL_X) +
				   data->header.axis_align[axis].index;
			out->readings[0].value =
				data->header.axis_align[axis].sign *
				icm20689_convert(channel.chan_type, data->readings[position],
						 &data->header, shift);
		} else {
			out->readings[0].value = icm20689_convert(
				channel.chan_type, data->readings[position], &data->header, shift);
		}

		out->header.base_timestamp_ns = data->header.timestamp;
		out->header.reading_count = 1;
		out->shift = shift;
	}

	*fit = 1;
	return 1;
}

static bool icm20689_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(trigger);
	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = icm20689_decoder_get_frame_count,
	.get_size_info = icm20689_decoder_get_size_info,
	.decode = icm20689_decoder_decode,
	.has_trigger = icm20689_decoder_has_trigger,
};

int icm20689_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
