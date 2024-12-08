/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adxl345.h"

#ifdef CONFIG_ADXL345_STREAM

#define SENSOR_SCALING_FACTOR (SENSOR_G / (16 * 1000 / 100))

static const uint32_t accel_period_ns[] = {
	[ADXL345_ODR_12HZ] = UINT32_C(1000000000) / 12,
	[ADXL345_ODR_25HZ] = UINT32_C(1000000000) / 25,
	[ADXL345_ODR_50HZ] = UINT32_C(1000000000) / 50,
	[ADXL345_ODR_100HZ] = UINT32_C(1000000000) / 100,
	[ADXL345_ODR_200HZ] = UINT32_C(1000000000) / 200,
	[ADXL345_ODR_400HZ] = UINT32_C(1000000000) / 400,
};

static const uint32_t range_to_shift[] = {
	[ADXL345_RANGE_2G] = 5,
	[ADXL345_RANGE_4G] = 6,
	[ADXL345_RANGE_8G] = 7,
	[ADXL345_RANGE_16G] = 8,
};

/* (1 / sensitivity) * (pow(2,31) / pow(2,shift)) * (unit_scaler) */
static const uint32_t qscale_factor_no_full_res[] = {
	/* (1.0 / ADXL362_ACCEL_2G_LSB_PER_G) * (2^31 / 2^5) * SENSOR_G / 1000000 */
	[ADXL345_RANGE_2G] = UINT32_C(2569011),
	/* (1.0 / ADXL362_ACCEL_4G_LSB_PER_G) * (2^31 / 2^6) * SENSOR_G / 1000000  */
	[ADXL345_RANGE_4G] = UINT32_C(642253),
	/* (1.0 / ADXL362_ACCEL_8G_LSB_PER_G) * (2^31 / 2^7) ) * SENSOR_G / 1000000 */
	[ADXL345_RANGE_8G] = UINT32_C(160563),
	/* (1.0 / ADXL362_ACCEL_8G_LSB_PER_G) * (2^31 / 2^8) ) * SENSOR_G / 1000000 */
	[ADXL345_RANGE_16G] = UINT32_C(40141),
};

/* (1 / sensitivity) * (pow(2,31) / pow(2,shift)) * (unit_scaler) */
static const uint32_t qscale_factor_full_res[] = {
	/* (1.0 / ADXL362_ACCEL_2G_LSB_PER_G) * (2^31 / 2^5) * SENSOR_G / 1000000 */
	[ADXL345_RANGE_2G] = UINT32_C(2569011),
	/* (1.0 / ADXL362_ACCEL_4G_LSB_PER_G) * (2^31 / 2^6) * SENSOR_G / 1000000  */
	[ADXL345_RANGE_4G] = UINT32_C(1284506),
	/* (1.0 / ADXL362_ACCEL_8G_LSB_PER_G) * (2^31 / 2^7) ) * SENSOR_G / 1000000 */
	[ADXL345_RANGE_8G] = UINT32_C(642253),
	/* (1.0 / ADXL362_ACCEL_8G_LSB_PER_G) * (2^31 / 2^8) ) * SENSOR_G / 1000000 */
	[ADXL345_RANGE_16G] = UINT32_C(321126),
};

static inline void adxl345_accel_convert_q31(q31_t *out, int16_t sample, int32_t range,
					uint8_t is_full_res)
{
	if (is_full_res) {
		switch (range) {
		case ADXL345_RANGE_2G:
			if (sample & BIT(9)) {
				sample |= ADXL345_COMPLEMENT_MASK(10);
			}
			break;
		case ADXL345_RANGE_4G:
			if (sample & BIT(10)) {
				sample |= ADXL345_COMPLEMENT_MASK(11);
			}
			break;
		case ADXL345_RANGE_8G:
			if (sample & BIT(11)) {
				sample |= ADXL345_COMPLEMENT_MASK(12);
			}
			break;
		case ADXL345_RANGE_16G:
			if (sample & BIT(12)) {
				sample |= ADXL345_COMPLEMENT_MASK(13);
			}
			break;
		}
	} else {
		if (sample & BIT(9)) {
			sample |= ADXL345_COMPLEMENT;
		}
	}

	*out = sample * qscale_factor_no_full_res[range];
}

static int adxl345_decode_stream(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				 uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adxl345_fifo_data *enc_data = (const struct adxl345_fifo_data *)buffer;
	const uint8_t *buffer_end =
		buffer + sizeof(struct adxl345_fifo_data) + enc_data->fifo_byte_count;
	int count = 0;
	uint8_t sample_num = 0;

	if ((uintptr_t)buffer_end <= *fit || chan_spec.chan_idx != 0) {
		return 0;
	}

	struct sensor_three_axis_data *data = (struct sensor_three_axis_data *)data_out;

	memset(data, 0, sizeof(struct sensor_three_axis_data));
	data->header.base_timestamp_ns = enc_data->timestamp;
	data->header.reading_count = 1;
	data->shift = range_to_shift[enc_data->selected_range];

	buffer += sizeof(struct adxl345_fifo_data);

	uint8_t sample_set_size = enc_data->sample_set_size;
	uint64_t period_ns = accel_period_ns[enc_data->accel_odr];
	uint8_t is_full_res = enc_data->is_full_res;

	/* Calculate which sample is decoded. */
	if ((uint8_t *)*fit >= buffer) {
		sample_num = ((uint8_t *)*fit - buffer) / sample_set_size;
	}

	while (count < max_count && buffer < buffer_end) {
		const uint8_t *sample_end = buffer;

		sample_end += sample_set_size;

		if ((uintptr_t)buffer < *fit) {
			/* This frame was already decoded, move on to the next frame */
			buffer = sample_end;
			continue;
		}

		switch (chan_spec.chan_type) {
		case SENSOR_CHAN_ACCEL_XYZ:
			data->readings[count].timestamp_delta = sample_num * period_ns;
			uint8_t buff_offset = 0;

			adxl345_accel_convert_q31(&data->readings[count].x, *(int16_t *)buffer,
					enc_data->selected_range, is_full_res);
			buff_offset = 2;
			adxl345_accel_convert_q31(&data->readings[count].y,
						*(int16_t *)(buffer + buff_offset),
							enc_data->selected_range, is_full_res);
			buff_offset += 2;
			adxl345_accel_convert_q31(&data->readings[count].z,
						*(int16_t *)(buffer + buff_offset),
							enc_data->selected_range, is_full_res);
			break;
		default:
			return -ENOTSUP;
		}
		buffer = sample_end;
		*fit = (uintptr_t)sample_end;
		count++;
	}
	return count;
}

#endif /* CONFIG_ADXL345_STREAM */

static int adxl345_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
					   uint16_t *frame_count)
{
	int32_t ret = -ENOTSUP;

	if (chan_spec.chan_idx != 0) {
		return ret;
	}

#ifdef CONFIG_ADXL345_STREAM
	const struct adxl345_fifo_data *data = (const struct adxl345_fifo_data *)buffer;

	if (!data->is_fifo) {
#endif /* CONFIG_ADXL345_STREAM */
		switch (chan_spec.chan_type) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			*frame_count = 1;
			ret = 0;
			break;

		default:
			break;
		}
#ifdef CONFIG_ADXL345_STREAM
	} else {
		if (data->fifo_byte_count == 0) {
			*frame_count = 0;
			ret = 0;
		} else {
			switch (chan_spec.chan_type) {
			case SENSOR_CHAN_ACCEL_XYZ:
				*frame_count =
					data->fifo_byte_count / data->sample_set_size;
				ret = 0;
				break;

			default:
				break;
			}
		}
	}
#endif /* CONFIG_ADXL345_STREAM */

	return ret;
}

static int adxl345_decode_sample(const struct adxl345_sample *data,
				 struct sensor_chan_spec chan_spec, uint32_t *fit,
				 uint16_t max_count, void *data_out)
{
	struct sensor_value *out = (struct sensor_value *)data_out;

	if (*fit > 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl345_accel_convert(out++, data->x);
		adxl345_accel_convert(out++, data->y);
		adxl345_accel_convert(out, data->z);
		break;
	default:
		return -ENOTSUP;
	}

	*fit = 1;

	return 0;
}

static int adxl345_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adxl345_sample *data = (const struct adxl345_sample *)buffer;

#ifdef CONFIG_ADXL345_STREAM
	if (data->is_fifo) {
		return adxl345_decode_stream(buffer, chan_spec, fit, max_count, data_out);
	}
#endif /* CONFIG_ADXL345_STREAM */

	return adxl345_decode_sample(data, chan_spec, fit, max_count, data_out);
}

static bool adxl345_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct adxl345_fifo_data *data = (const struct adxl345_fifo_data *)buffer;

	if (!data->is_fifo) {
		return false;
	}

	switch (trigger) {
	case SENSOR_TRIG_FIFO_WATERMARK:
		return FIELD_GET(ADXL345_INT_MAP_WATERMARK_MSK, data->int_status);
	default:
		return false;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adxl345_decoder_get_frame_count,
	.decode = adxl345_decoder_decode,
	.has_trigger = adxl345_decoder_has_trigger,
};

int adxl345_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
