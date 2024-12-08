/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adxl372.h"

#ifdef CONFIG_ADXL372_STREAM

/* (1.0 / 10 (sensor sensitivity)) * (2^31 / 2^11 (sensor shift) ) * SENSOR_G */
#define SENSOR_QSCALE_FACTOR UINT32_C(1027604)

#define ADXL372_COMPLEMENT         0xf000

static const uint32_t accel_period_ns[] = {
	[ADXL372_ODR_400HZ] = UINT32_C(1000000000) / 400,
	[ADXL372_ODR_800HZ] = UINT32_C(1000000000) / 800,
	[ADXL372_ODR_1600HZ] = UINT32_C(1000000000) / 1600,
	[ADXL372_ODR_3200HZ] = UINT32_C(1000000000) / 3200,
	[ADXL372_ODR_6400HZ] = UINT32_C(1000000000) / 6400,
};

static inline void adxl372_accel_convert_q31(q31_t *out, const uint8_t *buff)
{
	int16_t data_in = ((int16_t)*buff << 4) | (((int16_t)*(buff + 1) & 0xF0) >> 4);

	if (data_in & BIT(11)) {
		data_in |= ADXL372_COMPLEMENT;
	}

	*out = data_in * SENSOR_QSCALE_FACTOR;
}

static int adxl372_decode_stream(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				 uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adxl372_fifo_data *enc_data = (const struct adxl372_fifo_data *)buffer;
	const uint8_t *buffer_end =
		buffer + sizeof(struct adxl372_fifo_data) + enc_data->fifo_byte_count;
	int count = 0;
	uint8_t sample_num = 0;

	if ((uintptr_t)buffer_end <= *fit || chan_spec.chan_idx != 0) {
		return 0;
	}

	struct sensor_three_axis_data *data = (struct sensor_three_axis_data *)data_out;

	memset(data, 0, sizeof(struct sensor_three_axis_data));
	data->header.base_timestamp_ns = enc_data->timestamp;
	data->header.reading_count = 1;
	data->header.shift = 11; /* Sensor shift */

	buffer += sizeof(struct adxl372_fifo_data);

	uint8_t sample_set_size = enc_data->sample_set_size;
	uint64_t period_ns = accel_period_ns[enc_data->accel_odr];

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
		case SENSOR_CHAN_ACCEL_X:
			if (enc_data->has_x) {
				data->readings[count].timestamp_delta = sample_num * period_ns;
				adxl372_accel_convert_q31(&data->readings[count].x, buffer);
			}
			break;
		case SENSOR_CHAN_ACCEL_Y:
			if (enc_data->has_y) {
				uint8_t buff_offset = 0;

				/* If packet has X channel, then Y channel has offset. */
				if (enc_data->has_x) {
					buff_offset = 2;
				}
				data->readings[count].timestamp_delta = sample_num * period_ns;
				adxl372_accel_convert_q31(&data->readings[count].y,
							  (buffer + buff_offset));
			}
			break;
		case SENSOR_CHAN_ACCEL_Z:
			if (enc_data->has_z) {
				uint8_t buff_offset = 0;

				/* If packet has X channel and/or Y channel,
				 * then Z channel has offset.
				 */
				if (enc_data->has_x) {
					buff_offset = 2;
				}

				if (enc_data->has_y) {
					buff_offset += 2;
				}
				data->readings[count].timestamp_delta = sample_num * period_ns;
				adxl372_accel_convert_q31(&data->readings[count].z,
							  (buffer + buff_offset));
			}
			break;
		case SENSOR_CHAN_ACCEL_XYZ:
			data->readings[count].timestamp_delta = sample_num * period_ns;
			uint8_t buff_offset = 0;

			if (enc_data->has_x) {
				adxl372_accel_convert_q31(&data->readings[count].x, buffer);
				buff_offset = 2;
			}

			if (enc_data->has_y) {
				adxl372_accel_convert_q31(&data->readings[count].y,
							  (buffer + buff_offset));

				buff_offset += 2;
			}

			if (enc_data->has_z) {
				adxl372_accel_convert_q31(&data->readings[count].z,
							  (buffer + buff_offset));
			}
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

#endif /* CONFIG_ADXL372_STREAM */

static int adxl372_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
					   uint16_t *frame_count)
{
	int32_t ret = -ENOTSUP;

	if (chan_spec.chan_idx != 0) {
		return ret;
	}

#ifdef CONFIG_ADXL372_STREAM
	const struct adxl372_fifo_data *data = (const struct adxl372_fifo_data *)buffer;

	if (!data->is_fifo) {
#endif /* CONFIG_ADXL372_STREAM */
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
#ifdef CONFIG_ADXL372_STREAM
	} else {
		if (data->fifo_byte_count == 0) {
			*frame_count = 0;
			ret = 0;
		} else {
			switch (chan_spec.chan_type) {
			case SENSOR_CHAN_ACCEL_X:
				if (data->has_x) {
					*frame_count =
						data->fifo_byte_count / data->sample_set_size;
					ret = 0;
				}
				break;
			case SENSOR_CHAN_ACCEL_Y:
				if (data->has_y) {
					*frame_count =
						data->fifo_byte_count / data->sample_set_size;
					ret = 0;
				}
				break;
			case SENSOR_CHAN_ACCEL_Z:
				if (data->has_z) {
					*frame_count =
						data->fifo_byte_count / data->sample_set_size;
					ret = 0;
				}
				break;
			case SENSOR_CHAN_ACCEL_XYZ:
				if (data->has_x || data->has_y || data->has_z) {
					*frame_count =
						data->fifo_byte_count / data->sample_set_size;
					ret = 0;
				}
				break;

			default:
				break;
			}
		}
	}
#endif /* CONFIG_ADXL372_STREAM */

	return ret;
}

static int adxl372_decode_sample(const struct adxl372_xyz_accel_data *data,
				 struct sensor_chan_spec chan_spec, uint32_t *fit,
				 uint16_t max_count, void *data_out)
{
	struct sensor_value *out = (struct sensor_value *)data_out;

	if (*fit > 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
		adxl372_accel_convert(out, data->x);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl372_accel_convert(out, data->y);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl372_accel_convert(out, data->z);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl372_accel_convert(out++, data->x);
		adxl372_accel_convert(out++, data->y);
		adxl372_accel_convert(out, data->z);
		break;
	default:
		return -ENOTSUP;
	}

	*fit = 1;

	return 0;
}

static int adxl372_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adxl372_xyz_accel_data *data = (const struct adxl372_xyz_accel_data *)buffer;

#ifdef CONFIG_ADXL372_STREAM
	if (data->is_fifo) {
		return adxl372_decode_stream(buffer, chan_spec, fit, max_count, data_out);
	}
#endif /* CONFIG_ADXL372_STREAM */

	return adxl372_decode_sample(data, chan_spec, fit, max_count, data_out);
}

static bool adxl372_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct adxl372_fifo_data *data = (const struct adxl372_fifo_data *)buffer;

	if (!data->is_fifo) {
		return false;
	}

	switch (trigger) {
	case SENSOR_TRIG_DATA_READY:
		return FIELD_GET(ADXL372_INT1_MAP_DATA_RDY_MSK, data->int_status);
	case SENSOR_TRIG_FIFO_WATERMARK:
	case SENSOR_TRIG_FIFO_FULL:
		return FIELD_GET(ADXL372_INT1_MAP_FIFO_FULL_MSK, data->int_status);
	default:
		return false;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adxl372_decoder_get_frame_count,
	.decode = adxl372_decoder_decode,
	.has_trigger = adxl372_decoder_has_trigger,
};

int adxl372_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
