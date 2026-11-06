/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ictx53xx.h"

static int ictx53xx_decoder_get_frame_count(const uint8_t *buffer,
					     struct sensor_chan_spec chan_spec,
					     uint16_t *frame_count)
{
	const struct ictx53xx_encoded_data *data = (const struct ictx53xx_encoded_data *)buffer;
	const struct ictx53xx_decoder_header *header = &data->header;

	ARG_UNUSED(buffer);

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	uint8_t channel_request = ictx53xx_encode_channel(chan_spec.chan_type);

	if ((header->channels & channel_request) != channel_request) {
		return -ENODATA;
	}

	/* This sensor lacks a FIFO; there will always only be one frame at a time. */
	*frame_count = 1;
	return 0;
}

static int ictx53xx_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					   size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_DIE_TEMP:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	case SENSOR_CHAN_MAGN_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int ictx53xx_convert_raw_to_q31(uint8_t sensitivity, int16_t reading, q31_t *q31_out)
{
	int32_t microgauss = ictx53xx_convert_mag(sensitivity, reading);

	/* Convert to Q31 format: Q31 = (value * 2^(31 - shift)) / 1000000
	 * For magnetic field, we use shift=16, so the full scale is ±2^16 = ±65536 gauss
	 */
	*q31_out = (q31_t)(((int64_t)microgauss << (31 - ICTX53XX_SHIFT)) / 1000000);
	return 0;
}

static int ictx53xx_convert_raw_temp_to_q31(int16_t reading, q31_t *q31_out)
{
	int32_t microcelsius = ictx53xx_convert_temp(reading);

	/* Convert to Q31 format: Q31 = (value * 2^(31 - shift)) / 1000000
	 * For temperature, we use shift=16, so the full scale is ±2^16 = ±65536°C
	 * This gives us good resolution for typical temperature ranges (-40°C to +125°C)
	 */
	*q31_out = (q31_t)(((int64_t)microcelsius << (31 - ICTX53XX_SHIFT)) / 1000000);
	return 0;
}

static int ictx53xx_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct ictx53xx_encoded_data *edata = (const struct ictx53xx_encoded_data *)buffer;

	if (*fit != 0) {
		return 0;
	}
	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_X: {
		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = ICTX53XX_SHIFT;
		ictx53xx_convert_raw_to_q31(edata->header.sensitivity, edata->mag.x,
				&out->readings[0].value);
		break;
	}
	case SENSOR_CHAN_MAGN_Y: {
		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = ICTX53XX_SHIFT;
		ictx53xx_convert_raw_to_q31(edata->header.sensitivity, edata->mag.y,
				&out->readings[0].value);
		break;
	}
	case SENSOR_CHAN_MAGN_Z: {
		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = ICTX53XX_SHIFT;
		ictx53xx_convert_raw_to_q31(edata->header.sensitivity, edata->mag.z,
				&out->readings[0].value);
		break;
	}
	case SENSOR_CHAN_DIE_TEMP: {
		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = ICTX53XX_SHIFT;
		ictx53xx_convert_raw_temp_to_q31(edata->temp, &out->readings[0].temperature);
		break;
	}
	case SENSOR_CHAN_MAGN_XYZ: {
		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = ICTX53XX_SHIFT;
		ictx53xx_convert_raw_to_q31(edata->header.sensitivity, edata->mag.x,
				&out->readings[0].x);
		ictx53xx_convert_raw_to_q31(edata->header.sensitivity, edata->mag.y,
				&out->readings[0].y);
		ictx53xx_convert_raw_to_q31(edata->header.sensitivity, edata->mag.z,
				&out->readings[0].z);
		break;
	}

	default:
		return -EINVAL;
	}
	*fit = 1;

	return 1;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = ictx53xx_decoder_get_frame_count,
	.get_size_info = ictx53xx_decoder_get_size_info,
	.decode = ictx53xx_decoder_decode,
};

int ictx53xx_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
