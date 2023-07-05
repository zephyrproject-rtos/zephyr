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

static enum sensor_channel icm42688_get_channel_from_position(int pos)
{
	switch (pos) {
	case 0:
		return SENSOR_CHAN_DIE_TEMP;
	case 1:
		return SENSOR_CHAN_ACCEL_X;
	case 2:
		return SENSOR_CHAN_ACCEL_Y;
	case 3:
		return SENSOR_CHAN_ACCEL_Z;
	case 4:
		return SENSOR_CHAN_GYRO_X;
	case 5:
		return SENSOR_CHAN_GYRO_Y;
	case 6:
		return SENSOR_CHAN_GYRO_Z;
	default:
		return SENSOR_CHAN_MAX;
	}
}

int icm42688_convert_raw_to_q31(struct icm42688_cfg *cfg, enum sensor_channel chan,
				int32_t reading, q31_t *out)
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
		intermediate = intermediate * INT32_MAX * (1 << -shift) /  INT64_C(1000000);
	} else if (shift > 0) {
		intermediate = intermediate * INT32_MAX / (((1 << shift) - 1) * INT64_C(1000000));
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

static int icm42688_fifo_decode(const uint8_t *buffer, sensor_frame_iterator_t *fit,
				sensor_channel_iterator_t *cit, enum sensor_channel *channels,
				q31_t *values, uint8_t max_count)
{
	const struct icm42688_fifo_data *edata = (const struct icm42688_fifo_data *)buffer;

	if (edata->fifo_count <= *fit) {
		return 0;
	}

	/* Jump to the frame start */
	buffer += sizeof(struct icm42688_fifo_data) + *fit;

	const bool is_20b = FIELD_GET(FIFO_HEADER_20, buffer[0]);
	const bool has_accel = FIELD_GET(FIFO_HEADER_ACCEL, buffer[0]);
	const bool has_gyro = FIELD_GET(FIFO_HEADER_GYRO, buffer[0]);
	const int channel_count = 1 + (has_accel ? 3 : 0) + (has_gyro ? 3 : 0);
	const uint8_t *frame_end = buffer + /* header */ 1 + /* accel */ (has_accel ? 6 : 0) +
				   /* gyro */ (has_gyro ? 6 : 0) +
				   /* timestamp */ (has_accel && has_gyro ? 2 : 0) +
				   /* temperature */ (is_20b ? 2 : 1);
	int header_channels[7];
	int count = 0;

	if (*cit >= channel_count) {
		return -1;
	}

	if (has_accel) {
		header_channels[count++] = SENSOR_CHAN_ACCEL_X;
		header_channels[count++] = SENSOR_CHAN_ACCEL_Y;
		header_channels[count++] = SENSOR_CHAN_ACCEL_Z;
	}
	if (has_gyro) {
		header_channels[count++] = SENSOR_CHAN_GYRO_X;
		header_channels[count++] = SENSOR_CHAN_GYRO_Y;
		header_channels[count++] = SENSOR_CHAN_GYRO_Z;
	}
	header_channels[count] = SENSOR_CHAN_DIE_TEMP;
	count = 0;

	while (count < max_count && *cit < channel_count) {
		/*
		 * [0]@0 - accel_x
		 * [1]@2 - accel_y
		 * [2]@4 - accel_z
		 * [3]@6 - gyro_x
		 * [4]@8 - gyro_y
		 * [5]@a - gyro_z
		 * [6]@c - temp
		 */
		int offset = 1 + *cit * 2;

		channels[count] = header_channels[*cit];
		int32_t value;
		int32_t value_max = INT16_MAX;

		if (channels[count] == SENSOR_CHAN_DIE_TEMP) {
			value = buffer[offset];
			if (is_20b) {
				value |= (buffer[offset + 1] << 8);
			} else {
				value_max = INT8_MAX;
			}
		} else {
			value = (buffer[offset] << 8) | buffer[offset + 1];
			if (is_20b) {
				int mask = channels[count] < SENSOR_CHAN_ACCEL_XYZ ? GENMASK(7, 4)
										   : GENMASK(3, 0);
				/* Get the offset after the end of the frame data */
				int ext_offset =
					(channels[count] - (channels[count] < SENSOR_CHAN_ACCEL_XYZ
								    ? SENSOR_CHAN_ACCEL_X
								    : SENSOR_CHAN_GYRO_X));

				value = (value << 4) | FIELD_GET(mask, frame_end[ext_offset]);
				value_max = (1 << 19) - 1;
			}
		}
		values[count] = (q31_t)((int64_t)value * INT32_MAX / value_max);
		count++;
		*cit += 1;
	}

	if (*cit == channel_count) {
		/* Reached the end of the frame */
		*fit += (uintptr_t)frame_end - (uintptr_t)buffer + (is_20b ? 3 : 0);
		*cit = 0;
	}
	return count;
}

static int icm42688_one_shot_decode(const uint8_t *buffer, sensor_frame_iterator_t *fit,
				    sensor_channel_iterator_t *cit, enum sensor_channel *channels,
				    q31_t *values, uint8_t max_count)
{
	const struct icm42688_encoded_data *edata = (const struct icm42688_encoded_data *)buffer;
	uint8_t channels_read = edata->channels;
	struct icm42688_cfg cfg = {
		.accel_fs = edata->header.accel_fs,
		.gyro_fs = edata->header.gyro_fs,
	};
	int chan;
	int pos;
	int count = 0;
	int num_samples = __builtin_popcount(channels_read);

	channels_read = edata->channels;

	if (*fit != 0) {
		return 0;
	}

	/* Skip channels already decoded */
	for (int i = 0; i < *cit && channels_read; i++) {
		chan = __builtin_ctz(channels_read);
		channels_read &= ~BIT(chan);
	}

	/* Decode remaining channels */
	while (channels_read && *cit < num_samples && count < max_count) {
		chan = icm42688_get_channel_from_position(__builtin_ctz(channels_read));

		channels[count] = chan;
		pos = icm42688_get_channel_position(chan);

		icm42688_convert_raw_to_q31(&cfg, chan, edata->readings[pos], &values[count]);

		count++;
		channels_read &= ~BIT(chan);
		*cit += 1;
	}

	if (*cit >= __builtin_popcount(edata->channels)) {
		*fit += 1;
		*cit = 0;
	}

	return count;
}

static int icm42688_decoder_decode(const uint8_t *buffer, sensor_frame_iterator_t *fit,
				   sensor_channel_iterator_t *cit, enum sensor_channel *channels,
				   q31_t *values, uint8_t max_count)
{
	const struct icm42688_decoder_header *header =
		(const struct icm42688_decoder_header *)buffer;

	if (header->is_fifo) {
		return icm42688_fifo_decode(buffer, fit, cit, channels, values, max_count);
	}
	return icm42688_one_shot_decode(buffer, fit, cit, channels, values, max_count);
}

static int icm42688_decoder_get_frame_count(const uint8_t *buffer, uint16_t *frame_count)
{
	const struct icm42688_fifo_data *data = (const struct icm42688_fifo_data *)buffer;
	const struct icm42688_decoder_header *header = &data->header;

	if (!header->is_fifo) {
		*frame_count = 1;
		return 0;
	}

	/* Skip the header */
	buffer += sizeof(struct icm42688_fifo_data);

	uint16_t count = 0;
	const uint8_t *end = buffer + data->fifo_count;

	while (buffer < end) {
		bool is_20b = FIELD_GET(FIFO_HEADER_20, buffer[0]);
		int size = is_20b ? 3 : 2;

		if (FIELD_GET(FIFO_HEADER_ACCEL, buffer[0])) {
			size += 6;
		}
		if (FIELD_GET(FIFO_HEADER_GYRO, buffer[0])) {
			size += 6;
		}
		if (FIELD_GET(FIFO_HEADER_TIMESTAMP_FSYNC, buffer[0])) {
			size += 2;
		}
		if (is_20b) {
			size += 3;
		}

		buffer += size;
		++count;
	}

	*frame_count = count;
	return 0;
}

static int icm42688_decoder_get_timestamp(const uint8_t *buffer, uint64_t *timestamp_ns)
{
	const struct icm42688_decoder_header *header =
		(const struct icm42688_decoder_header *)buffer;

	*timestamp_ns = header->timestamp;
	return 0;
}

static bool icm24688_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct icm42688_fifo_data *edata = (const struct icm42688_fifo_data *)buffer;

	if (!edata->header.is_fifo) {
		return false;
	}

	switch (trigger) {
	case SENSOR_TRIG_DATA_READY:
		return FIELD_GET(BIT_INT_STATUS_DATA_RDY, edata->int_status);
	case SENSOR_TRIG_FIFO_THRESHOLD:
		return FIELD_GET(BIT_INT_STATUS_FIFO_THS, edata->int_status);
	case SENSOR_TRIG_FIFO_FULL:
		return FIELD_GET(BIT_INT_STATUS_FIFO_FULL, edata->int_status);
	default:
		return false;
	}
}

static int icm42688_decoder_get_shift(const uint8_t *buffer, enum sensor_channel channel_type,
				      int8_t *shift)
{
	const struct icm42688_decoder_header *header =
		(const struct icm42688_decoder_header *)buffer;

	return icm42688_get_shift(channel_type, header->accel_fs, header->gyro_fs, shift);
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = icm42688_decoder_get_frame_count,
	.get_timestamp = icm42688_decoder_get_timestamp,
	.get_shift = icm42688_decoder_get_shift,
	.has_trigger = icm24688_decoder_has_trigger,
	.decode = icm42688_decoder_decode,
};

int icm42688_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
