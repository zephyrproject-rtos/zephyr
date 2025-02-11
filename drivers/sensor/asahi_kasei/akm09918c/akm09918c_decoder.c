/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include "akm09918c.h"

#define DT_DRV_COMPAT asahi_kasei_akm09918c

static int akm09918c_decoder_get_frame_count(const uint8_t *buffer,
					     struct sensor_chan_spec chan_spec,
					     uint16_t *frame_count)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(chan_spec);

	/* This sensor lacks a FIFO; there will always only be one frame at a time. */
	*frame_count = 1;
	return 0;
}

static int akm09918c_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					   size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

/** Fixed shift value to use. All channels (MAGN_X, _Y, and _Z) have the same fixed range of
 *  +/- 49.12 Gauss.
 */
#define AKM09918C_SHIFT (6)

static int akm09918c_convert_raw_to_q31(int16_t reading, q31_t *out)
{
	int64_t intermediate = ((int64_t)reading * AKM09918C_MICRO_GAUSS_PER_BIT) *
			       ((int64_t)INT32_MAX + 1) /
			       ((1 << AKM09918C_SHIFT) * INT64_C(1000000));

	*out = CLAMP(intermediate, INT32_MIN, INT32_MAX);
	return 0;
}

static int akm09918c_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct akm09918c_encoded_data *edata = (const struct akm09918c_encoded_data *)buffer;

	if (*fit != 0) {
		return 0;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ: {
		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = AKM09918C_SHIFT;

		akm09918c_convert_raw_to_q31(edata->readings[0], &out->readings[0].x);
		akm09918c_convert_raw_to_q31(edata->readings[1], &out->readings[0].y);
		akm09918c_convert_raw_to_q31(edata->readings[2], &out->readings[0].z);
		*fit = 1;

		return 1;
	}
	default:
		return -EINVAL;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = akm09918c_decoder_get_frame_count,
	.get_size_info = akm09918c_decoder_get_size_info,
	.decode = akm09918c_decoder_decode,
};

int akm09918c_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
