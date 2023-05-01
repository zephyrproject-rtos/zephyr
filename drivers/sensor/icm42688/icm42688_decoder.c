/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm42688_decoder.h"
#include "icm42688.h"
#include <errno.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM42688_DECODER, CONFIG_SENSOR_LOG_LEVEL);

int icm42688_get_shift(enum sensor_channel chan)
{
	/* TODO: May need fine tuning */
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		return 16;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		return 16;
	case SENSOR_CHAN_DIE_TEMP:
		return 16;
	default:
		return -ENOTSUP;
	}
}

int icm42688_convert_raw_to_q31(struct icm42688_cfg *cfg, enum sensor_channel chan,
				icm42688_sample_t reading, q31_t *out)
{
	int32_t whole;
	uint32_t fraction;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		icm42688_accel_ms(cfg, reading, &whole, &fraction);
		*out = (whole << icm42688_get_shift(chan)) | fraction;
		break;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		icm42688_gyro_rads(cfg, reading, &whole, &fraction);
		*out = (whole << icm42688_get_shift(chan)) | fraction;
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm42688_temp_c(reading, &whole, &fraction);
		*out = (whole << icm42688_get_shift(chan)) | fraction;
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
	struct icm42688_dev_data *dev_data = &data->dev_data;
	struct icm42688_encoded_data *edata = (struct icm42688_encoded_data *)buf;

	edata->channels = 0;

	for (int i = 0; i < num_channels; i++) {
		edata->channels |= icm42688_encode_channel(channels[i]);
	}

	edata->accelerometer_scale = dev_data->cfg.accel_fs;
	edata->gyroscope_scale = dev_data->cfg.gyro_fs;

	return 0;
}

static int calc_num_samples(uint8_t channels_read)
{
	int chan;
	int num_samples = 0;

	while (channels_read) {
		chan = __builtin_clz(channels_read);
		num_samples += SENSOR_CHANNEL_3_AXIS(chan) ? 3 : 1;
		channels_read &= ~BIT(chan);
	}

	return num_samples;
}

static int decode(const uint8_t *buffer, sensor_frame_iterator_t *fit,
		  sensor_channel_iterator_t *cit, enum sensor_channel *channels, q31_t *values,
		  uint8_t max_count)
{
	struct icm42688_encoded_data *edata = (struct icm42688_encoded_data *)buffer;
	uint8_t channels_read = edata->channels;
	struct icm42688_cfg cfg;
	int chan;
	int pos;
	int count = 0;
	int num_samples = calc_num_samples(channels_read);

	cfg.accel_fs = edata->accelerometer_scale;
	cfg.gyro_fs = edata->gyroscope_scale;

	channels_read = edata->channels;

	/* Skip channels already decoded */
	for(int i = 0; i < *cit && channels_read; i++) {
		chan = __builtin_clz(channels_read);
		channels_read &= ~BIT(chan);
	}

	/* Decode remaining channels */
	while (channels_read && *cit < num_samples && count < max_count) {
		chan = __builtin_clz(channels_read);

		channels[count] = chan;
		pos = icm42688_get_channel_position(chan);

		icm42688_convert_raw_to_q31(&cfg, chan, edata->readings[pos], &values[count]);

		count += SENSOR_CHANNEL_3_AXIS(chan) ? 3 : 1;
		channels_read &= ~BIT(chan);
		*cit += 1;
	}

	if (*cit >= num_samples) {
		*fit += 1;
		*cit = 0;
	}

	return 0;
}

struct sensor_decoder_api icm42688_decoder = {
	.get_frame_count = NULL,
	.get_timestamp = NULL,
	.get_shift = NULL,
	.decode = decode,
};

int icm42688_get_decoder(const struct device *dev, struct sensor_decoder_api *decoder)
{
	*decoder = icm42688_decoder;

	return 0;
}
