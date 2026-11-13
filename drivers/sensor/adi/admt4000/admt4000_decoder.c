/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_admt4000

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/admt4000.h>
#include <zephyr/sys/util.h>

static int admt4000_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	const struct admt4000_encoded_data *edata = (const struct admt4000_encoded_data *)buffer;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	/* This sensor lacks a FIFO; there will always only be one frame at a time. */
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ADMT4000_ANGLE:
		*frame_count = edata->has_angle ? 1 : 0;
		break;
	case SENSOR_CHAN_ROTATION:
		*frame_count = edata->has_turns ? 1 : 0;
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		*frame_count = edata->has_temp ? 1 : 0;
		break;
	case SENSOR_CHAN_ADMT4000_COS:
		*frame_count = edata->has_cos ? 1 : 0;
		break;
	case SENSOR_CHAN_ADMT4000_SIN:
		*frame_count = edata->has_sin ? 1 : 0;
		break;
	case SENSOR_CHAN_ADMT4000_RADIUS:
		*frame_count = edata->has_radius ? 1 : 0;
		break;
	default:
		return -ENOTSUP;
	}

	return *frame_count > 0 ? 0 : -ENOTSUP;
}

static int admt4000_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					  size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ADMT4000_ANGLE:
	case SENSOR_CHAN_ROTATION:
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_ADMT4000_COS:
	case SENSOR_CHAN_ADMT4000_SIN:
	case SENSOR_CHAN_ADMT4000_RADIUS:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int admt4000_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct admt4000_encoded_data *edata = (const struct admt4000_encoded_data *)buffer;
	const struct admt4000_sample *data = &edata->data;

	if (*fit != 0) {
		return 0;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ADMT4000_ANGLE:
		if (edata->has_angle) {
			struct sensor_q31_data *out = data_out;
			uint16_t raw = FIELD_GET(ADMT4000_ANGLE_MASK, data->angle);

			out->header.base_timestamp_ns = edata->header.timestamp;
			out->header.reading_count = 1;
			out->shift = ADMT4000_ANGLE_Q31_SHIFT;
			out->readings[0].timestamp_delta = 0;
			out->readings[0].angle = (q31_t)((int64_t)raw * ADMT4000_ANGLE_CONV_Q9_22);
		} else {
			return -ENODATA;
		}
		break;

	case SENSOR_CHAN_ROTATION:
		if (edata->has_turns) {
			struct sensor_q31_data *out = data_out;

			out->header.base_timestamp_ns = edata->header.timestamp;
			out->header.reading_count = 1;
			out->shift = ADMT4000_TURNS_Q31_SHIFT;
			out->readings[0].timestamp_delta = 0;
			out->readings[0].value =
				(q31_t)((int64_t)data->quarter_turns * ADMT4000_TURNS_CONV_Q6_25);
		} else {
			return -ENODATA;
		}
		break;

	case SENSOR_CHAN_AMBIENT_TEMP:
		if (edata->has_temp) {
			struct sensor_q31_data *out = data_out;

			out->header.base_timestamp_ns = edata->header.timestamp;
			out->header.reading_count = 1;
			out->shift = ADMT4000_TEMP_Q31_SHIFT;
			out->readings[0].timestamp_delta = 0;
			/*
			 * converted_temp is in units of 10^-5 degrees C.
			 * Q7.24 format: q31 = converted_temp * 2^24 / 10^5
			 */
			out->readings[0].temperature =
				(q31_t)((int64_t)data->converted_temp * (1 << 24) / 100000);
		} else {
			return -ENODATA;
		}
		break;

	case SENSOR_CHAN_ADMT4000_COS:
		if (edata->has_cos) {
			struct sensor_q31_data *out = data_out;

			out->header.base_timestamp_ns = edata->header.timestamp;
			out->header.reading_count = 1;
			out->shift = ADMT4000_CORDIC_Q31_SHIFT;
			out->readings[0].timestamp_delta = 0;
			out->readings[0].value =
				(q31_t)((int64_t)data->cos_val * ADMT4000_CORDIC_CONV_Q0_31);
		} else {
			return -ENODATA;
		}
		break;

	case SENSOR_CHAN_ADMT4000_SIN:
		if (edata->has_sin) {
			struct sensor_q31_data *out = data_out;

			out->header.base_timestamp_ns = edata->header.timestamp;
			out->header.reading_count = 1;
			out->shift = ADMT4000_CORDIC_Q31_SHIFT;
			out->readings[0].timestamp_delta = 0;
			out->readings[0].value =
				(q31_t)((int64_t)data->sin_val * ADMT4000_CORDIC_CONV_Q0_31);
		} else {
			return -ENODATA;
		}
		break;

	case SENSOR_CHAN_ADMT4000_RADIUS:
		if (edata->has_radius) {
			struct sensor_q31_data *out = data_out;

			out->header.base_timestamp_ns = edata->header.timestamp;
			out->header.reading_count = 1;
			out->shift = ADMT4000_RADIUS_Q31_SHIFT;
			out->readings[0].timestamp_delta = 0;
			/*
			 * converted_radius is in units of 10^-7 mV/V.
			 * Q25.6 format: q31 = converted_radius * 2^25 / 10^7
			 */
			out->readings[0].value =
				(q31_t)((int64_t)data->converted_radius * (1 << 25) / 10000000);
		} else {
			return -ENODATA;
		}
		break;

	default:
		return -EINVAL;
	}

	*fit = 1;
	return 1;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = admt4000_decoder_get_frame_count,
	.get_size_info = admt4000_decoder_get_size_info,
	.decode = admt4000_decoder_decode,
};

int admt4000_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
