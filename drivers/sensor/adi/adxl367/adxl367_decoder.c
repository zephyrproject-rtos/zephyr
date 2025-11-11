/*
 * Copyright (c) 2024 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adxl367.h"
#include <zephyr/sys/byteorder.h>

#ifdef CONFIG_ADXL367_STREAM

#define ADXL367_COMPLEMENT		0xC000
/* Scale factor is the same for all ranges. */
/* (1.0 / sensor sensitivity) * (2^31 / 2^sensor shift ) * SENSOR_G / 1000000 */
#define SENSOR_QSCALE_FACTOR UINT32_C(164584)

/* (2^31 / 2^8(shift) */
#define ADXL367_TEMP_QSCALE   8388608
#define ADXL367_TEMP_SENSITIVITY		54 /* LSB/C */
#define ADXL367_TEMP_BIAS_TEST_CONDITION 25 /*C*/

static const uint32_t accel_period_ns[] = {
	[ADXL367_ODR_12P5HZ] = UINT32_C(10000000000) / 125,
	[ADXL367_ODR_25HZ] = UINT32_C(1000000000) / 25,
	[ADXL367_ODR_50HZ] = UINT32_C(1000000000) / 50,
	[ADXL367_ODR_100HZ] = UINT32_C(1000000000) / 100,
	[ADXL367_ODR_200HZ] = UINT32_C(1000000000) / 200,
	[ADXL367_ODR_400HZ] = UINT32_C(1000000000) / 400,
};

static const uint32_t range_to_shift[] = {
	[ADXL367_2G_RANGE] = 5,
	[ADXL367_4G_RANGE] = 6,
	[ADXL367_8G_RANGE] = 7,
};

enum adxl367_12b_packet_start {
	ADXL367_12B_PACKET_ALIGNED		= 0,
	ADXL367_12B_PACKET_NOT_ALIGNED	= 1,
};

static inline void adxl367_temp_convert_q31(q31_t *out, const uint8_t *buff,
	const enum adxl367_fifo_read_mode read_mode, uint8_t sample_aligned)
{
	int16_t data_in;
	unsigned int convert_value = 0;

	switch (read_mode) {
	case ADXL367_8B:
		/* Because full resolution is 14b and this is 8 MSB bits. */
		data_in = ((*((int8_t *)buff)) << 6) & 0x3FC0;
		convert_value = 1;
		break;

	case ADXL367_12B:
		/* Sample starts from the bit 4 of a first byte in buff. */
		if (sample_aligned == 0) {
			data_in = ((int16_t)(*buff & 0x0F) << 8) | *(buff + 1);
		} else {
			data_in = ((int16_t)(*buff) << 4) | (*(buff + 1) >> 4);
		}
		/* Because full resolution is 14b and this is 12 MSB bits. */
		data_in = (data_in << 2) & 0x3FFC;
		convert_value = 1;
		break;

	case ADXL367_12B_CHID:
		data_in = sys_le16_to_cpu(*((int16_t *)(buff)));
		if (ADXL367_FIFO_HDR_CHECK_TEMP(data_in)) {
			/* Remove channel ID. */
			data_in &= 0x3FFF;
			/* Because full resolution is 14b and this is 12 MSB bits. */
			data_in = (data_in << 2) & 0x3FFC;
			convert_value = 1;
		}
		break;

	case ADXL367_14B_CHID: {
		uint16_t *tmp_buf = (uint16_t *)buff;

		data_in = (int16_t)(((*tmp_buf >> 8) & 0xFF) | ((*tmp_buf << 8) & 0xFF00));
		if (ADXL367_FIFO_HDR_CHECK_TEMP(data_in)) {
			/* Remove channel ID. */
			data_in &= 0x3FFF;
			convert_value = 1;
		}
		break;
	}

	default:
		break;
	}

	if (convert_value) {
		if (data_in & BIT(13)) {
			data_in |= ADXL367_COMPLEMENT;
		}

		*out = ((data_in - ADXL367_TEMP_25C) / ADXL367_TEMP_SENSITIVITY
				+ ADXL367_TEMP_BIAS_TEST_CONDITION) * ADXL367_TEMP_QSCALE;
	}
}

static inline void adxl367_accel_convert_q31(q31_t *out, const uint8_t *buff,
	const enum adxl367_fifo_read_mode read_mode, uint8_t axis, uint8_t sample_aligned)
{
	int16_t data_in;
	unsigned int convert_value = 0;

	switch (read_mode) {
	case ADXL367_8B:
		/* Because full resolution is 14b and this is 8 MSB bits. */
		data_in = ((*((int8_t *)buff)) << 6) & 0x3FC0;
		convert_value = 1;
		break;

	case ADXL367_12B:
		/* Sample starts from the bit 4 of a first byte in buff. */
		if (sample_aligned == 0) {
			data_in = ((int16_t)(*buff & 0x0F) << 8) | *(buff + 1);
		} else {
			data_in = ((int16_t)(*buff) << 4) | (*(buff + 1) >> 4);
		}
		/* Because full resolution is 14b and this is 12 MSB bits. */
		data_in = (data_in << 2) & 0x3FFC;
		convert_value = 1;
		break;

	case ADXL367_12B_CHID:
		data_in = sys_le16_to_cpu(*((int16_t *)(buff)));
		if (ADXL367_FIFO_HDR_GET_ACCEL_AXIS(data_in) == axis) {
			/* Remove channel ID. */
			data_in &= 0x3FFF;
			/* Because full resolution is 14b and this is 12 MSB bits. */
			data_in = (data_in << 2) & 0x3FFC;
			convert_value = 1;
		}
		break;

	case ADXL367_14B_CHID: {
		uint16_t *tmp_buf = (uint16_t *)buff;

		data_in = (int16_t)(((*tmp_buf >> 8) & 0xFF) | ((*tmp_buf << 8) & 0xFF00));
		if (ADXL367_FIFO_HDR_GET_ACCEL_AXIS(data_in) == axis) {
			/* Remove channel ID. */
			data_in &= 0x3FFF;
			convert_value = 1;
		}
		break;
	}

	default:
		break;
	}

	if (convert_value) {
		if (data_in & BIT(13)) {
			data_in |= ADXL367_COMPLEMENT;
		}

		*out = data_in * SENSOR_QSCALE_FACTOR;
	}
}

static int adxl367_get_accel(const struct adxl367_fifo_data *enc_data,
	struct sensor_three_axis_data *data, const uint8_t *buffer, int count,
	uint8_t sample_size, struct sensor_chan_spec chan_spec,
	uint64_t period_ns, uint8_t sample_num)
{
	int ret = 0;

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
		if (enc_data->has_x) {
			data->readings[count].timestamp_delta =
				sample_num * period_ns;
			adxl367_accel_convert_q31(&data->readings[count].x,
				buffer,	enc_data->fifo_read_mode,
				ADXL367_X_AXIS, 1);
		}
		break;
	case SENSOR_CHAN_ACCEL_Y:
		if (enc_data->has_y) {
			uint8_t buff_offset = 0;

			/* If packet has X channel, then Y channel has offset. */
			if (enc_data->has_x) {
				buff_offset = sample_size;
			}
			data->readings[count].timestamp_delta = sample_num * period_ns;
			adxl367_accel_convert_q31(&data->readings[count].y,
				(buffer + buff_offset), enc_data->fifo_read_mode,
				ADXL367_Y_AXIS, 1);
		}
		break;
	case SENSOR_CHAN_ACCEL_Z:
		if (enc_data->has_z) {
			uint8_t buff_offset = 0;

			/* If packet has X channel and/or Y channel,
			 * then Z channel has offset.
			 */

			if (enc_data->has_x) {
				buff_offset = sample_size;
			}

			if (enc_data->has_y) {
				buff_offset += sample_size;
			}
			data->readings[count].timestamp_delta =
				sample_num * period_ns;
			adxl367_accel_convert_q31(&data->readings[count].z,
				(buffer + buff_offset), enc_data->fifo_read_mode,
				ADXL367_Z_AXIS, 1);
		}
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		data->readings[count].timestamp_delta =
			sample_num * period_ns;
		uint8_t buff_offset = 0;

		if (enc_data->has_x) {
			adxl367_accel_convert_q31(&data->readings[count].x,
				buffer, enc_data->fifo_read_mode,
				ADXL367_X_AXIS, 1);
			buff_offset = sample_size;
		}

		if (enc_data->has_y) {
			adxl367_accel_convert_q31(&data->readings[count].y,
				(buffer + buff_offset), enc_data->fifo_read_mode,
				ADXL367_Y_AXIS, 1);

			buff_offset += sample_size;
		}

		if (enc_data->has_z) {
			adxl367_accel_convert_q31(&data->readings[count].z,
				(buffer + buff_offset), enc_data->fifo_read_mode,
				ADXL367_Z_AXIS, 1);
		}
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int adxl367_get_12b_accel(const struct adxl367_fifo_data *enc_data,
	struct sensor_three_axis_data *data, const uint8_t *buffer, int count,
	uint8_t packet_size, struct sensor_chan_spec chan_spec, uint8_t packet_alignment,
	uint64_t period_ns, uint8_t sample_num)
{
	int ret = 0;
	uint8_t sample_aligned = 1;
	uint8_t buff_offset = 0;
	uint8_t samples_before = 0;

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
		if (enc_data->has_x) {
			if (packet_alignment ==	ADXL367_12B_PACKET_NOT_ALIGNED) {
				sample_aligned = 0;
			}

			data->readings[count].timestamp_delta = sample_num * period_ns;
			adxl367_accel_convert_q31(&data->readings[count].x,
				buffer, enc_data->fifo_read_mode,
				ADXL367_X_AXIS, sample_aligned);
		}
		break;
	case SENSOR_CHAN_ACCEL_Y:
		if (enc_data->has_y) {
			buff_offset = 0;

			/* If packet has X channel,
			 * then Y channel has offset.
			 */
			if (enc_data->has_x) {
				buff_offset = 2;
				if (packet_alignment == ADXL367_12B_PACKET_ALIGNED) {
					sample_aligned = 0;
					buff_offset = 1;
				}
			}
			data->readings[count].timestamp_delta = sample_num * period_ns;
			adxl367_accel_convert_q31(&data->readings[count].y,
				(buffer + buff_offset), enc_data->fifo_read_mode,
				ADXL367_Y_AXIS, sample_aligned);
		}
		break;
	case SENSOR_CHAN_ACCEL_Z:
		if (enc_data->has_z) {
			buff_offset = 0;
			samples_before = 0;

			/* If packet has X channel and/or Y channel,
			 * then Z channel has offset.
			 */
			if (enc_data->has_x) {
				samples_before++;
			}

			if (enc_data->has_y) {
				samples_before++;
			}

			if (samples_before == 0) {
				if (packet_alignment == ADXL367_12B_PACKET_NOT_ALIGNED) {
					sample_aligned = 0;
				}
			} else {
				buff_offset = (samples_before * 12) / 8;
				if (samples_before == 1) {
					if (packet_alignment == ADXL367_12B_PACKET_ALIGNED) {
						sample_aligned = 0;
					} else {
						buff_offset++;
					}
				} else {
					if (packet_alignment == ADXL367_12B_PACKET_NOT_ALIGNED) {
						sample_aligned = 0;
					}
				}
			}
			data->readings[count].timestamp_delta =	sample_num * period_ns;
			adxl367_accel_convert_q31(&data->readings[count].z,
				(buffer + buff_offset), enc_data->fifo_read_mode,
				ADXL367_Z_AXIS,	sample_aligned);
		}
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		data->readings[count].timestamp_delta = sample_num * period_ns;
		buff_offset = 0;
		samples_before = 0;

		if (enc_data->has_x) {
			if (packet_alignment ==	ADXL367_12B_PACKET_NOT_ALIGNED) {
				sample_aligned = 0;
			}
			samples_before++;

			adxl367_accel_convert_q31(&data->readings[count].x,
				buffer, enc_data->fifo_read_mode, ADXL367_X_AXIS,
				sample_aligned);
		}

		if (enc_data->has_y) {
			if (samples_before) {
				if (packet_alignment ==	ADXL367_12B_PACKET_ALIGNED) {
					buff_offset = 1;
					sample_aligned = 0;
				} else {
					buff_offset = 2;
					sample_aligned = 1;
				}
			} else {
				if (packet_alignment ==	ADXL367_12B_PACKET_NOT_ALIGNED) {
					sample_aligned = 0;
					buff_offset = 0;
				}
			}
			samples_before++;

			adxl367_accel_convert_q31(&data->readings[count].y,
				(buffer + buff_offset), enc_data->fifo_read_mode,
				ADXL367_Y_AXIS,	sample_aligned);
		}

		if (enc_data->has_z) {
			/* Z can have 2 samples in the packet before it or 0. */
			if (samples_before) {
				if (packet_alignment ==	ADXL367_12B_PACKET_ALIGNED) {
					sample_aligned = 1;
				} else {
					sample_aligned = 0;
				}
				buff_offset = 3;
			} else {
				if (packet_alignment ==	ADXL367_12B_PACKET_NOT_ALIGNED) {
					sample_aligned = 0;
					buff_offset = 0;
				}
			}

			adxl367_accel_convert_q31(&data->readings[count].z,
				(buffer + buff_offset),	enc_data->fifo_read_mode,
				ADXL367_Z_AXIS,	sample_aligned);
		}
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static void adxl367_get_12b_temp(const struct adxl367_fifo_data *enc_data,
	struct sensor_q31_data *data, const uint8_t *buffer, int count, uint8_t packet_size)
{
	if (enc_data->has_tmp) {
		uint8_t offset = ((packet_size - 1) * 12) / 8;
		uint8_t start_offset = ((packet_size - 1) * 12) % 8;
		uint8_t sample_aligned = 1;

		if (start_offset) {
			sample_aligned = 0;
		}

		adxl367_temp_convert_q31(&data->readings[count].temperature,
			(buffer + offset), enc_data->fifo_read_mode,
			sample_aligned);
	}
}

static int adxl367_decode_12b_stream(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				uint32_t *fit, uint16_t max_count, void *data_out,
				const struct adxl367_fifo_data *enc_data)
{
	const uint8_t *buffer_end =
		buffer + sizeof(struct adxl367_fifo_data) + enc_data->fifo_byte_count;
	uint8_t packet_size = enc_data->packet_size;
	uint64_t period_ns = accel_period_ns[enc_data->accel_odr];
	uint8_t sample_num = 0;
	int count = 0;
	uint8_t packet_alignment = ADXL367_12B_PACKET_ALIGNED;

	while (count < max_count && buffer < buffer_end) {
		const uint8_t *sample_end = buffer;

		/* For ADXL367_12B mode packet_size is number of samples in one
		 * packet. If packet size is not aligned, sample_end will be on
		 * the byte that contains part of a last sample.
		 */
		if (packet_alignment == ADXL367_12B_PACKET_ALIGNED) {
			sample_end += (packet_size * 12) / 8;
		} else {
			sample_end += (packet_size * 12) / 8 + 1;
		}

		/* If fit is larger than buffer this frame was already decoded,
		 * move on to the next frame.
		 */
		if ((uintptr_t)buffer < *fit) {
			/* If number of samples in one packet is odd number,
			 * alignment changes for each packet.
			 */
			if (enc_data->packet_size % 2) {
				if (packet_alignment == ADXL367_12B_PACKET_ALIGNED) {
					packet_alignment = ADXL367_12B_PACKET_NOT_ALIGNED;
				} else {
					packet_alignment = ADXL367_12B_PACKET_ALIGNED;
				}
			}

			buffer = sample_end;

			sample_num++;
			continue;
		}

		if (chan_spec.chan_type == SENSOR_CHAN_DIE_TEMP) {
			struct sensor_q31_data *data = (struct sensor_q31_data *)data_out;

			memset(data, 0, sizeof(struct sensor_three_axis_data));
			data->header.base_timestamp_ns = enc_data->timestamp;
			data->header.reading_count = 1;
			data->shift = 8;

			data->readings[count].timestamp_delta =
					period_ns * sample_num;

			adxl367_get_12b_temp(enc_data, data, buffer, count, packet_size);
		} else {
			struct sensor_three_axis_data *data =
				(struct sensor_three_axis_data *)data_out;

			memset(data, 0, sizeof(struct sensor_three_axis_data));
			data->header.base_timestamp_ns = enc_data->timestamp;
			data->header.reading_count = 1;
			data->shift = range_to_shift[enc_data->range];

			int ret = adxl367_get_12b_accel(enc_data, data, buffer, count, packet_size,
				chan_spec, packet_alignment, period_ns, sample_num);

			if (ret != 0) {
				return ret;
			}
		}

		buffer = sample_end;
		*fit = (uintptr_t)sample_end;
		count++;
	}

	return count;
}

static int adxl367_decode_stream(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adxl367_fifo_data *enc_data = (const struct adxl367_fifo_data *)buffer;
	const uint8_t *buffer_end =
		buffer + sizeof(struct adxl367_fifo_data) + enc_data->fifo_byte_count;
	int count = 0;
	uint8_t sample_num = 0;

	if ((uintptr_t)buffer_end <= *fit || chan_spec.chan_idx != 0) {
		return 0;
	}

	buffer += sizeof(struct adxl367_fifo_data);

	uint8_t packet_size = enc_data->packet_size;
	uint64_t period_ns = accel_period_ns[enc_data->accel_odr];
	uint8_t sample_size = 2;

	if (enc_data->fifo_read_mode == ADXL367_8B) {
		sample_size = 1;
	}

	if (enc_data->fifo_read_mode == ADXL367_12B) {
		count = adxl367_decode_12b_stream(buffer, chan_spec, fit, max_count,
			data_out, enc_data);
	} else {
		/* Calculate which sample is decoded. */
		if ((uint8_t *)*fit >= buffer) {
			sample_num = ((uint8_t *)*fit - buffer) / packet_size;
		}

		while (count < max_count && buffer < buffer_end) {
			const uint8_t *sample_end = buffer;

			sample_end += packet_size;

			if ((uintptr_t)buffer < *fit) {
				/* This frame was already decoded, move on to the next frame */
				buffer = sample_end;
				continue;
			}

			if (chan_spec.chan_type == SENSOR_CHAN_DIE_TEMP) {
				struct sensor_q31_data *data = (struct sensor_q31_data *)data_out;

				memset(data, 0, sizeof(struct sensor_three_axis_data));
				data->header.base_timestamp_ns = enc_data->timestamp;
				data->header.reading_count = 1;
				data->shift = 8;

				data->readings[count].timestamp_delta =
						period_ns * sample_num;

				if (enc_data->has_tmp) {
					uint8_t offset = (packet_size - 1) * sample_size;

					adxl367_temp_convert_q31(&data->readings[count].temperature,
						(buffer + offset), enc_data->fifo_read_mode, 1);
				}
			} else {
				struct sensor_three_axis_data *data =
					(struct sensor_three_axis_data *)data_out;

				memset(data, 0, sizeof(struct sensor_three_axis_data));
				data->header.base_timestamp_ns = enc_data->timestamp;
				data->header.reading_count = 1;
				data->shift = range_to_shift[enc_data->range];

				int ret = adxl367_get_accel(enc_data, data, buffer, count,
							sample_size, chan_spec,
							period_ns, sample_num);

				if (ret != 0) {
					return ret;
				}
			}

			buffer = sample_end;
			*fit = (uintptr_t)sample_end;
			count++;
		}
	}

	return count;
}

uint16_t adxl367_get_frame_count(const struct adxl367_fifo_data *data)
{
	uint16_t frame_count = 0;

	if (data->fifo_read_mode == ADXL367_12B) {
		frame_count = data->fifo_byte_count * 8 / (data->packet_size * 12);
	} else {
		frame_count = data->fifo_byte_count / data->packet_size;
	}

	return frame_count;
}

#endif /* CONFIG_ADXL367_STREAM */

static int adxl367_decoder_get_frame_count(const uint8_t *buffer,
					     struct sensor_chan_spec chan_spec,
					     uint16_t *frame_count)
{
	int32_t ret = -ENOTSUP;

	if (chan_spec.chan_idx != 0) {
		return ret;
	}

#ifdef CONFIG_ADXL367_STREAM
	const struct adxl367_fifo_data *data = (const struct adxl367_fifo_data *)buffer;

	if (!data->is_fifo) {
#endif /* CONFIG_ADXL367_STREAM */
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
#ifdef CONFIG_ADXL367_STREAM
	} else {
		if (data->fifo_byte_count == 0)	{
			*frame_count = 0;
			ret = 0;
		} else {
			switch (chan_spec.chan_type) {
			case SENSOR_CHAN_ACCEL_X:
				if (data->has_x) {
					*frame_count = adxl367_get_frame_count(data);
					ret = 0;
				}
				break;

			case SENSOR_CHAN_ACCEL_Y:
				if (data->has_y) {
					*frame_count = adxl367_get_frame_count(data);
					ret = 0;
				}
				break;

			case SENSOR_CHAN_ACCEL_Z:
				if (data->has_z) {
					*frame_count = adxl367_get_frame_count(data);
					ret = 0;
				}
				break;

			case SENSOR_CHAN_ACCEL_XYZ:
				if (data->has_x || data->has_y || data->has_z) {
					*frame_count = adxl367_get_frame_count(data);
					ret = 0;
				}
				break;

			case SENSOR_CHAN_DIE_TEMP:
				if (data->has_tmp) {
					*frame_count = adxl367_get_frame_count(data);
					ret = 0;
					break;
				}
				break;

			default:
				break;
			}
		}
	}
#endif /* CONFIG_ADXL367_STREAM */

	return ret;
}

static int adxl367_decode_sample(const struct adxl367_sample_data *data,
	struct sensor_chan_spec chan_spec, uint32_t *fit, uint16_t max_count, void *data_out)
{
	struct sensor_value *out = (struct sensor_value *) data_out;

	if (*fit > 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X: /* Acceleration on the X axis, in m/s^2. */
		adxl367_accel_convert(out, data->xyz.x, data->xyz.range);
		break;
	case SENSOR_CHAN_ACCEL_Y: /* Acceleration on the Y axis, in m/s^2. */
		adxl367_accel_convert(out, data->xyz.y, data->xyz.range);
		break;
	case SENSOR_CHAN_ACCEL_Z: /* Acceleration on the Z axis, in m/s^2. */
		adxl367_accel_convert(out, data->xyz.z, data->xyz.range);
		break;
	case SENSOR_CHAN_ACCEL_XYZ: /* Acceleration on the XYZ axis, in m/s^2. */
		adxl367_accel_convert(out++, data->xyz.x, data->xyz.range);
		adxl367_accel_convert(out++, data->xyz.y, data->xyz.range);
		adxl367_accel_convert(out, data->xyz.z, data->xyz.range);
		break;
	case SENSOR_CHAN_DIE_TEMP: /* Temperature in degrees Celsius. */
		adxl367_temp_convert(out, data->raw_temp);
		break;
	default:
		return -ENOTSUP;
	}

	*fit = 1;

	return 0;
}

static int adxl367_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adxl367_sample_data *data = (const struct adxl367_sample_data *)buffer;

#ifdef CONFIG_ADXL367_STREAM
	if (data->is_fifo) {
		return adxl367_decode_stream(buffer, chan_spec, fit, max_count, data_out);
	}
#endif /* CONFIG_ADXL367_STREAM */

	return adxl367_decode_sample(data, chan_spec, fit, max_count, data_out);
}

static bool adxl367_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct adxl367_fifo_data *data = (const struct adxl367_fifo_data *)buffer;

	if (!data->is_fifo) {
		return false;
	}

	switch (trigger) {
	case SENSOR_TRIG_DATA_READY:
		return (ADXL367_STATUS_DATA_RDY & data->int_status);
	case SENSOR_TRIG_FIFO_WATERMARK:
		return (ADXL367_STATUS_FIFO_WATERMARK & data->int_status);
	case SENSOR_TRIG_FIFO_FULL:
		return (ADXL367_STATUS_FIFO_WATERMARK & data->int_status);
	default:
		return false;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adxl367_decoder_get_frame_count,
	.decode = adxl367_decoder_decode,
	.has_trigger = adxl367_decoder_has_trigger,
};

int adxl367_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
