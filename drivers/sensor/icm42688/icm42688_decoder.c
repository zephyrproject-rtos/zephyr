/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm42688_decoder.h"
#include "icm42688.h"
#include <errno.h>

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM42688_DECODER, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT  invensense_icm42688
#define TEMP_SHIFT_VAL 8

static inline int8_t icm42688_decoder_get_accel_shift(enum icm42688_accel_fs accel_fs)
{
	static const int8_t accel_fs_to_shift[] = {
		4, /* ICM42688_ACCEL_FS_16G */
		3, /* ICM42688_ACCEL_FS_8G  */
		2, /* ICM42688_ACCEL_FS_4G  */
		1, /* ICM42688_ACCEL_FS_2G  */
	};

	return (accel_fs <= ICM42688_ACCEL_FS_2G ? accel_fs_to_shift[accel_fs] : -EINVAL);
}

static inline int8_t icm42688_decoder_get_gyro_shift(enum icm42688_gyro_fs gyro_fs)
{
	static const int8_t gyro_fs_to_shift[] = {
		11, /* ICM42688_GYRO_FS_2000   */
		10, /* ICM42688_GYRO_FS_1000   */
		9,  /* ICM42688_GYRO_FS_500    */
		8,  /* ICM42688_GYRO_FS_250    */
		7,  /* ICM42688_GYRO_FS_125    */
		6,  /* ICM42688_GYRO_FS_62_5   */
		5,  /* ICM42688_GYRO_FS_31_25  */
		4,  /* ICM42688_GYRO_FS_15_625 */
	};

	return (gyro_fs <= ICM42688_GYRO_FS_15_625 ? gyro_fs_to_shift[gyro_fs] : -EINVAL);
}

int icm42688_convert_raw_to_q31(struct icm42688_cfg *cfg, enum sensor_channel chan,
				icm42688_sample_t reading, q31_t *out)
{
	int32_t whole;
	uint32_t fraction;
	uint32_t w_mask, f_mask;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		icm42688_accel_ms(cfg, reading, &whole, &fraction);
		w_mask = GENMASK(31, 31 - icm42688_decoder_get_accel_shift(cfg->accel_fs));
		f_mask = ~w_mask;
		*out = FIELD_PREP(w_mask, whole) | FIELD_PREP(f_mask, fraction);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		icm42688_gyro_rads(cfg, reading, &whole, &fraction);
		w_mask = GENMASK(31, 31 - icm42688_decoder_get_gyro_shift(cfg->gyro_fs));
		f_mask = ~w_mask;
		*out = FIELD_PREP(w_mask, whole) | FIELD_PREP(f_mask, fraction);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm42688_temp_c(reading, &whole, &fraction);
		w_mask = GENMASK(31, 31 - TEMP_SHIFT_VAL);
		f_mask = ~w_mask;
		*out = FIELD_PREP(w_mask, whole) | FIELD_PREP(f_mask, fraction);
		break;
	default:
		return -ENOTSUP;
	}

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
	case SENSOR_CHAN_GYRO_XYZ:
		encode_bmask = BIT(icm42688_get_channel_position(chan)) |
			       BIT(icm42688_get_channel_position(chan + 1)) |
			       BIT(icm42688_get_channel_position(chan + 2));
		break;
	default:
		break;
	}

	return encode_bmask;
}

int icm42688_encode(const struct device *dev, const enum sensor_channel *const channels,
		    const size_t num_channels, uint8_t *buf)
{
	struct icm42688_sensor_data *data = dev->data;
	struct icm42688_encoded_data *edata = (struct icm42688_encoded_data *)buf;

	edata->channels = 0;

	for (int i = 0; i < num_channels; i++) {
		edata->channels |= icm42688_encode_channel(channels[i]);
	}

	edata->accelerometer_scale = data->cfg.accel_fs;
	edata->gyroscope_scale = data->cfg.gyro_fs;
	edata->timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());

	return 0;
}

static int icm42688_decoder_decode(const uint8_t *buffer, sensor_frame_iterator_t *fit,
				   sensor_channel_iterator_t *cit, enum sensor_channel *channels,
				   q31_t *values, uint8_t max_count)
{
	struct icm42688_encoded_data *edata = (struct icm42688_encoded_data *)buffer;
	uint8_t channels_read = edata->channels;
	struct icm42688_cfg cfg;
	int chan;
	int pos;
	int count = 0;
	int num_samples = __builtin_popcount(channels_read);

	cfg.accel_fs = edata->accelerometer_scale;
	cfg.gyro_fs = edata->gyroscope_scale;

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
		chan = __builtin_ctz(channels_read);

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

static int icm42688_decoder_get_frame_count(const uint8_t *buffer, uint16_t *frame_count)
{
	*frame_count = 1;
	return 0;
}

static int icm42688_decoder_get_timestamp(const uint8_t *buffer, uint64_t *timestamp_ns)
{
	const struct icm42688_encoded_data *edata = (const struct icm42688_encoded_data *)buffer;

	*timestamp_ns = edata->timestamp;
	return 0;
}

static int icm42688_decoder_get_shift(const uint8_t *buffer, enum sensor_channel channel_type,
				      int8_t *shift)
{
	const struct icm42688_encoded_data *edata = (const struct icm42688_encoded_data *)buffer;

	switch (channel_type) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		*shift = icm42688_decoder_get_accel_shift(edata->accelerometer_scale);
		return 0;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		*shift = icm42688_decoder_get_gyro_shift(edata->gyroscope_scale);
		return 0;
	case SENSOR_CHAN_DIE_TEMP:
		*shift = TEMP_SHIFT_VAL;
		return 0;
	default:
		return -EINVAL;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = icm42688_decoder_get_frame_count,
	.get_timestamp = icm42688_decoder_get_timestamp,
	.get_shift = icm42688_decoder_get_shift,
	.decode = icm42688_decoder_decode,
};

int icm42688_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
