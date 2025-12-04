/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adxl362.h"
#include <zephyr/sys/byteorder.h>

#ifdef CONFIG_ADXL362_STREAM

/* (2^31 / 2^8(shift) */
#define ADXL362_TEMP_QSCALE   8388608
#define ADXL362_TEMP_LSB_PER_C 15

#define ADXL362_COMPLEMENT         0xf000

static const uint32_t accel_period_ns[] = {
	[ADXL362_ODR_12_5_HZ] = UINT32_C(10000000000) / 125,
	[ADXL362_ODR_25_HZ] = UINT32_C(1000000000) / 25,
	[ADXL362_ODR_50_HZ] = UINT32_C(1000000000) / 50,
	[ADXL362_ODR_100_HZ] = UINT32_C(1000000000) / 100,
	[ADXL362_ODR_200_HZ] = UINT32_C(1000000000) / 200,
	[ADXL362_ODR_400_HZ] = UINT32_C(1000000000) / 400,
};

static const uint32_t range_to_shift[] = {
	[ADXL362_RANGE_2G] = 5,
	[ADXL362_RANGE_4G] = 6,
	[ADXL362_RANGE_8G] = 7,
};

/* (1 / sensitivity) * (pow(2,31) / pow(2,shift)) * (unit_scaler) */
static const uint32_t qscale_factor[] = {
	/* (1.0 / ADXL362_ACCEL_2G_LSB_PER_G) * (2^31 / 2^5) * SENSOR_G / 1000000 */
	[ADXL362_RANGE_2G] = UINT32_C(658338),
	/* (1.0 / ADXL362_ACCEL_4G_LSB_PER_G) * (2^31 / 2^6) * SENSOR_G / 1000000  */
	[ADXL362_RANGE_4G] = UINT32_C(658338),
	/* (1.0 / ADXL362_ACCEL_8G_LSB_PER_G) * (2^31 / 2^7) ) * SENSOR_G / 1000000 */
	[ADXL362_RANGE_8G] = UINT32_C(700360),
};

static inline void adxl362_temp_convert_q31(q31_t *out, int16_t data_in)
{
/* See sensitivity and bias specifications in table 1 of datasheet */
	data_in &= 0xFFF;

	if (data_in & BIT(11)) {
		data_in |= ADXL362_COMPLEMENT;
	}

	*out = ((data_in - ADXL362_TEMP_BIAS_LSB) / ADXL362_TEMP_LSB_PER_C
			+ ADXL362_TEMP_BIAS_TEST_CONDITION) * ADXL362_TEMP_QSCALE;
}

static inline void adxl362_accel_convert_q31(q31_t *out, int16_t data_in, int32_t range)
{
	data_in &= 0xFFF;

	if (data_in & BIT(11)) {
		data_in |= ADXL362_COMPLEMENT;
	}

	*out = data_in * qscale_factor[range];
}

static int adxl362_decode_stream(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adxl362_fifo_data *enc_data = (const struct adxl362_fifo_data *)buffer;
	const uint8_t *buffer_end =
			buffer + sizeof(struct adxl362_fifo_data) + enc_data->fifo_byte_count;
	int count = 0;
	uint8_t sample_num = 0;
	int16_t data_in;

	if ((uintptr_t)buffer_end <= *fit || chan_spec.chan_idx != 0) {
		return 0;
	}

	buffer += sizeof(struct adxl362_fifo_data);

	uint8_t sample_set_size = 6;

	if (enc_data->has_tmp) {
		sample_set_size = 8;
	}

	uint64_t period_ns = accel_period_ns[enc_data->accel_odr];

	/* Calculate which sample is decoded. */
	if (*fit >= (uintptr_t)buffer) {
		sample_num = (*fit - (uintptr_t)buffer) / sample_set_size;
	}

	while (count < max_count && buffer < buffer_end) {
		const uint8_t *sample_end = buffer;

		sample_end += sample_set_size;

		if ((uintptr_t)buffer < *fit) {
			/* This frame was already decoded, move on to the next frame */
			buffer = sample_end;
			continue;
		}

		if (chan_spec.chan_type == SENSOR_CHAN_DIE_TEMP) {
			if (enc_data->has_tmp) {
				struct sensor_q31_data *data = (struct sensor_q31_data *)data_out;

				memset(data, 0, sizeof(struct sensor_three_axis_data));
				data->header.base_timestamp_ns = enc_data->timestamp;
				data->header.reading_count = 1;
				data->shift = 8;

				data->readings[count].timestamp_delta =
						period_ns * sample_num;

				data_in = sys_le16_to_cpu(*((int16_t *)(buffer + 6)));

				/* Check if this sample contains temperature value. */
				if (ADXL362_FIFO_HDR_CHECK_TEMP(data_in)) {
					adxl362_temp_convert_q31(&data->readings[count].temperature,
										data_in);
				}
			}
		} else {
			struct sensor_three_axis_data *data =
					(struct sensor_three_axis_data *)data_out;

			memset(data, 0, sizeof(struct sensor_three_axis_data));
			data->header.base_timestamp_ns = enc_data->timestamp;
			data->header.reading_count = 1;
			data->shift = range_to_shift[enc_data->selected_range];

			switch (chan_spec.chan_type) {
			case SENSOR_CHAN_ACCEL_X:
				data->readings[count].timestamp_delta = sample_num
					* period_ns;

				/* Convert received data into signeg integer. */
				data_in = sys_le16_to_cpu(*((int16_t *)buffer));

				/* Check if this sample contains X value. */
				if (ADXL362_FIFO_HDR_CHECK_ACCEL_X(data_in)) {
					adxl362_accel_convert_q31(&data->readings[count].x,
						data_in, enc_data->selected_range);
				}
				break;
			case SENSOR_CHAN_ACCEL_Y:
				data->readings[count].timestamp_delta = sample_num
					* period_ns;

				/* Convert received data into signeg integer. */
				data_in = sys_le16_to_cpu(*((int16_t *)(buffer + 2)));

				/* Check if this sample contains Y value. */
				if (ADXL362_FIFO_HDR_CHECK_ACCEL_Y(data_in)) {
					adxl362_accel_convert_q31(&data->readings[count].y,
							data_in, enc_data->selected_range);
				}
				break;
			case SENSOR_CHAN_ACCEL_Z:
				data->readings[count].timestamp_delta = sample_num
					* period_ns;
				/* Convert received data into signeg integer. */
				data_in = sys_le16_to_cpu(*((int16_t *)(buffer + 4)));

				/* Check if this sample contains Y value. */
				if (ADXL362_FIFO_HDR_CHECK_ACCEL_Z(data_in)) {
					adxl362_accel_convert_q31(&data->readings[count].z,
							data_in, enc_data->selected_range);
				}
				break;
			case SENSOR_CHAN_ACCEL_XYZ:
				data->readings[count].timestamp_delta = sample_num * period_ns;

				/* Convert received data into signeg integer. */
				data_in = sys_le16_to_cpu(*((int16_t *)buffer));

				/* Check if this sample contains X value. */
				if (ADXL362_FIFO_HDR_CHECK_ACCEL_X(data_in)) {
					adxl362_accel_convert_q31(&data->readings[count].x,
							data_in, enc_data->selected_range);
				}

				/* Convert received data into signeg integer. */
				data_in = sys_le16_to_cpu(*((int16_t *)(buffer + 2)));

				/* Check if this sample contains Y value. */
				if (ADXL362_FIFO_HDR_CHECK_ACCEL_Y(data_in)) {
					adxl362_accel_convert_q31(&data->readings[count].y,
								data_in, enc_data->selected_range);
				}

				/* Convert received data into signeg integer. */
				data_in = sys_le16_to_cpu(*((int16_t *)(buffer + 4)));

				/* Check if this sample contains Z value. */
				if (ADXL362_FIFO_HDR_CHECK_ACCEL_Z(data_in)) {
					adxl362_accel_convert_q31(&data->readings[count].z,
								data_in, enc_data->selected_range);
				}
				break;
			default:
				return -ENOTSUP;
			}
		}

		buffer = sample_end;
		*fit = (uintptr_t)sample_end;
		count++;
	}
	return count;
}

#endif /* CONFIG_ADXL362_STREAM */

static int adxl362_decoder_get_frame_count(const uint8_t *buffer,
					     struct sensor_chan_spec chan_spec,
					     uint16_t *frame_count)
{
	int32_t ret = -ENOTSUP;

	if (chan_spec.chan_idx != 0) {
		return ret;
	}

#ifdef CONFIG_ADXL362_STREAM
	const struct adxl362_fifo_data *data = (const struct adxl362_fifo_data *)buffer;

	if (!data->is_fifo) {
#endif /* CONFIG_ADXL362_STREAM */
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
#ifdef CONFIG_ADXL362_STREAM
	} else {
		if (data->fifo_byte_count == 0)	{
			*frame_count = 0;
			ret = 0;
		} else {
			switch (chan_spec.chan_type) {
			case SENSOR_CHAN_ACCEL_X:
			case SENSOR_CHAN_ACCEL_Y:
			case SENSOR_CHAN_ACCEL_Z:
			case SENSOR_CHAN_ACCEL_XYZ:
				if (data->has_tmp) {
					/*6 bytes for XYZ and 2 bytes for TEMP*/
					*frame_count = data->fifo_byte_count / 8;
					ret = 0;
				} else {
					*frame_count = data->fifo_byte_count / 6;
					ret = 0;
				}
				break;

			case SENSOR_CHAN_DIE_TEMP:
				if (data->has_tmp) {
					/*6 bytes for XYZ and 2 bytes for TEMP*/
					*frame_count = data->fifo_byte_count / 8;
					ret = 0;
				}
				break;

			default:
				break;
			}
		}
	}
#endif /* CONFIG_ADXL362_STREAM */

	return ret;
}

static int adxl362_decode_sample(const struct adxl362_sample_data *data,
	struct sensor_chan_spec chan_spec, uint32_t *fit, uint16_t max_count, void *data_out)
{
	struct sensor_value *out = (struct sensor_value *) data_out;

	if (*fit > 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X: /* Acceleration on the X axis, in m/s^2. */
		adxl362_accel_convert(out, data->acc_x, data->selected_range);
		break;
	case SENSOR_CHAN_ACCEL_Y: /* Acceleration on the Y axis, in m/s^2. */
		adxl362_accel_convert(out, data->acc_y, data->selected_range);
		break;
	case SENSOR_CHAN_ACCEL_Z: /* Acceleration on the Z axis, in m/s^2. */
		adxl362_accel_convert(out, data->acc_z,  data->selected_range);
		break;
	case SENSOR_CHAN_ACCEL_XYZ: /* Acceleration on the XYZ axis, in m/s^2. */
		adxl362_accel_convert(out++, data->acc_x, data->selected_range);
		adxl362_accel_convert(out++, data->acc_y, data->selected_range);
		adxl362_accel_convert(out, data->acc_z,  data->selected_range);
		break;
	case SENSOR_CHAN_DIE_TEMP: /* Temperature in degrees Celsius. */
		adxl362_temp_convert(out, data->temp);
		break;
	default:
		return -ENOTSUP;
	}

	*fit = 1;

	return 0;
}

static int adxl362_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adxl362_sample_data *data = (const struct adxl362_sample_data *)buffer;

#ifdef CONFIG_ADXL362_STREAM
	if (data->is_fifo) {
		return adxl362_decode_stream(buffer, chan_spec, fit, max_count, data_out);
	}
#endif /* CONFIG_ADXL362_STREAM */

	return adxl362_decode_sample(data, chan_spec, fit, max_count, data_out);
}

static bool adxl362_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct adxl362_fifo_data *data = (const struct adxl362_fifo_data *)buffer;

	if (!data->is_fifo) {
		return false;
	}

	switch (trigger) {
	case SENSOR_TRIG_DATA_READY:
		return ADXL362_STATUS_CHECK_DATA_READY(data->int_status);
	case SENSOR_TRIG_FIFO_WATERMARK:
		return ADXL362_STATUS_CHECK_FIFO_WTR(data->int_status);
	case SENSOR_TRIG_FIFO_FULL:
		return ADXL362_STATUS_CHECK_FIFO_OVR(data->int_status);
	default:
		return false;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adxl362_decoder_get_frame_count,
	.decode = adxl362_decoder_decode,
	.has_trigger = adxl362_decoder_has_trigger,
};

int adxl362_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
