/*
 * Copyright (c) 2024 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mmc56x3.h"
#include <math.h>

static int mmc56x3_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
					   uint16_t *frame_count)
{
	const struct mmc56x3_encoded_data *edata = (const struct mmc56x3_encoded_data *)buffer;
	int32_t ret = -ENOTSUP;

	if (chan_spec.chan_idx != 0) {
		return ret;
	}

	/* This sensor lacks a FIFO; there will always only be one frame at a time. */
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		*frame_count = edata->has_temp ? 1 : 0;
		break;
	case SENSOR_CHAN_MAGN_X:
		*frame_count = edata->has_magn_x ? 1 : 0;
		break;
	case SENSOR_CHAN_MAGN_Y:
		*frame_count = edata->has_magn_y ? 1 : 0;
		break;
	case SENSOR_CHAN_MAGN_Z:
		*frame_count = edata->has_magn_z ? 1 : 0;
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		*frame_count =
			((edata->has_magn_x && edata->has_magn_y && edata->has_magn_z) ? 1 : 0);
		break;
	default:
		return ret;
	}

	if (*frame_count > 0) {
		ret = 0;
	}

	return ret;
}

static int mmc56x3_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					 size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
		*base_size = sizeof(struct sensor_q31_sample_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
	case SENSOR_CHAN_MAGN_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mmc56x3_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				  uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct mmc56x3_encoded_data *edata = (const struct mmc56x3_encoded_data *)buffer;
	const struct mmc56x3_data *data = &edata->data;

	if (*fit != 0) {
		return 0;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		if (edata->has_temp) {
			struct sensor_q31_data *out = data_out;

			out->header.base_timestamp_ns = edata->header.timestamp;
			out->header.reading_count = 1;
			out->readings[0].temperature = MMC56X3_TEMP_CONV_Q7_24_BASE +
						       data->temp * MMC56X3_TEMP_CONV_Q7_24_RES;
			out->shift = MMC56X3_TEMP_SHIFT;
		} else {
			return -ENODATA;
		}
		break;
	case SENSOR_CHAN_MAGN_X:
		if (edata->has_magn_x) {
			struct sensor_q31_data *out = data_out;

			out->header.base_timestamp_ns = edata->header.timestamp;
			out->header.reading_count = 1;
			out->readings[0].value = data->magn_x * MMC56X3_MAGN_CONV_Q5_26_20B;
			out->shift = MMC56X3_MAGN_SHIFT;
		} else {
			return -ENODATA;
		}
		break;
	case SENSOR_CHAN_MAGN_Y:
		if (edata->has_magn_y) {
			struct sensor_q31_data *out = data_out;

			out->header.base_timestamp_ns = edata->header.timestamp;
			out->header.reading_count = 1;
			out->readings[0].value = data->magn_y * MMC56X3_MAGN_CONV_Q5_26_20B;
			out->shift = MMC56X3_MAGN_SHIFT;
		} else {
			return -ENODATA;
		}
		break;
	case SENSOR_CHAN_MAGN_Z:
		if (edata->has_magn_z) {
			struct sensor_q31_data *out = data_out;

			out->header.base_timestamp_ns = edata->header.timestamp;
			out->header.reading_count = 1;
			out->readings[0].value = data->magn_z * MMC56X3_MAGN_CONV_Q5_26_20B;
			out->shift = MMC56X3_MAGN_SHIFT;
		} else {
			return -ENODATA;
		}
		break;
	case SENSOR_CHAN_MAGN_XYZ: {
		if (edata->has_magn_x && edata->has_magn_y && edata->has_magn_z) {
			struct sensor_three_axis_data *out_3 = data_out;

			out_3->header.base_timestamp_ns = edata->header.timestamp;
			out_3->header.reading_count = 1;
			out_3->shift = MMC56X3_MAGN_SHIFT;

			out_3->readings[0].v[0] = data->magn_x * MMC56X3_MAGN_CONV_Q5_26_20B;
			out_3->readings[0].v[1] = data->magn_y * MMC56X3_MAGN_CONV_Q5_26_20B;
			out_3->readings[0].v[2] = data->magn_z * MMC56X3_MAGN_CONV_Q5_26_20B;
		} else {
			return -ENODATA;
		}
		break;
	}
	default:
		return -EINVAL;
	}

	*fit = 1;
	return 1;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = mmc56x3_decoder_get_frame_count,
	.get_size_info = mmc56x3_decoder_get_size_info,
	.decode = mmc56x3_decoder_decode,
};

int mmc56x3_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
