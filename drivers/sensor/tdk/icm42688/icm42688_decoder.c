/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm42688_decoder.h"
#include "icm42688_reg.h"
#include "icm42688.h"
#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor_clock.h>

LOG_MODULE_REGISTER(ICM42688_DECODER, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT invensense_icm42688

static int icm42688_get_shift(enum sensor_channel channel, int accel_fs, int gyro_fs, int8_t *shift)
{
	switch (channel) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		switch (accel_fs) {
		case ICM42688_DT_ACCEL_FS_2:
			*shift = 5;
			return 0;
		case ICM42688_DT_ACCEL_FS_4:
			*shift = 6;
			return 0;
		case ICM42688_DT_ACCEL_FS_8:
			*shift = 7;
			return 0;
		case ICM42688_DT_ACCEL_FS_16:
			*shift = 8;
			return 0;
		default:
			return -EINVAL;
		}
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		switch (gyro_fs) {
		case ICM42688_DT_GYRO_FS_15_625:
			*shift = -1;
			return 0;
		case ICM42688_DT_GYRO_FS_31_25:
			*shift = 0;
			return 0;
		case ICM42688_DT_GYRO_FS_62_5:
			*shift = 1;
			return 0;
		case ICM42688_DT_GYRO_FS_125:
			*shift = 2;
			return 0;
		case ICM42688_DT_GYRO_FS_250:
			*shift = 3;
			return 0;
		case ICM42688_DT_GYRO_FS_500:
			*shift = 4;
			return 0;
		case ICM42688_DT_GYRO_FS_1000:
			*shift = 5;
			return 0;
		case ICM42688_DT_GYRO_FS_2000:
			*shift = 6;
			return 0;
		default:
			return -EINVAL;
		}
	case SENSOR_CHAN_DIE_TEMP:
		*shift = 9;
		return 0;
	default:
		return -EINVAL;
	}
}

int icm42688_convert_raw_to_q31(struct icm42688_cfg *cfg, enum sensor_channel chan, int32_t reading,
				q31_t *out)
{
	int32_t whole;
	int32_t fraction;
	int64_t intermediate;
	int8_t shift;
	int rc;

	rc = icm42688_get_shift(chan, cfg->accel_fs, cfg->gyro_fs, &shift);
	if (rc != 0) {
		return rc;
	}

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		icm42688_accel_ms(cfg, reading, &whole, &fraction);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		icm42688_gyro_rads(cfg, reading, &whole, &fraction);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm42688_temp_c(reading, &whole, &fraction);
		break;
	default:
		return -ENOTSUP;
	}
	intermediate = ((int64_t)whole * INT64_C(1000000) + fraction);
	if (shift < 0) {
		intermediate =
			intermediate * ((int64_t)INT32_MAX + 1) * (1 << -shift) / INT64_C(1000000);
	} else if (shift > 0) {
		intermediate =
			intermediate * ((int64_t)INT32_MAX + 1) / ((1 << shift) * INT64_C(1000000));
	}
	*out = CLAMP(intermediate, INT32_MIN, INT32_MAX);

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
		encode_bmask = BIT(icm42688_get_channel_position(SENSOR_CHAN_ACCEL_X)) |
			       BIT(icm42688_get_channel_position(SENSOR_CHAN_ACCEL_Y)) |
			       BIT(icm42688_get_channel_position(SENSOR_CHAN_ACCEL_Z));
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		encode_bmask = BIT(icm42688_get_channel_position(SENSOR_CHAN_GYRO_X)) |
			       BIT(icm42688_get_channel_position(SENSOR_CHAN_GYRO_Y)) |
			       BIT(icm42688_get_channel_position(SENSOR_CHAN_GYRO_Z));
		break;
	default:
		break;
	}

	return encode_bmask;
}

int icm42688_encode(const struct device *dev, const struct sensor_chan_spec *const channels,
		    const size_t num_channels, uint8_t *buf)
{
	struct icm42688_dev_data *data = dev->data;
	struct icm42688_encoded_data *edata = (struct icm42688_encoded_data *)buf;
	uint64_t cycles;
	int rc;

	edata->channels = 0;

	for (int i = 0; i < num_channels; i++) {
		edata->channels |= icm42688_encode_channel(channels[i].chan_type);
	}

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		return rc;
	}

	edata->header.is_fifo = false;
	edata->header.accel_fs = data->cfg.accel_fs;
	edata->header.gyro_fs = data->cfg.gyro_fs;
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	return 0;
}

#define IS_ACCEL(chan) ((chan) >= SENSOR_CHAN_ACCEL_X && (chan) <= SENSOR_CHAN_ACCEL_XYZ)
#define IS_GYRO(chan)  ((chan) >= SENSOR_CHAN_GYRO_X && (chan) <= SENSOR_CHAN_GYRO_XYZ)

static inline q31_t icm42688_read_temperature_from_packet(const uint8_t *pkt)
{
	int32_t temperature;
	int32_t whole;
	int32_t fraction;

	/* Temperature always assumes a shift of 9 for a range of (-273,273) C */
	if (FIELD_GET(FIFO_HEADER_20, pkt[0]) == 1) {
		temperature = (pkt[0xd] << 8) | pkt[0xe];

		icm42688_temp_c(temperature, &whole, &fraction);
	} else {
		if (FIELD_GET(FIFO_HEADER_ACCEL, pkt[0]) == 1 &&
		    FIELD_GET(FIFO_HEADER_GYRO, pkt[0]) == 1) {
			temperature = pkt[0xd];
		} else {
			temperature = pkt[0x7];
		}

		int64_t sensitivity = 207;
		int64_t temperature100 = (temperature * 100) + (25 * sensitivity);

		whole = temperature100 / sensitivity;
		fraction =
			((temperature100 - whole * sensitivity) * INT64_C(1000000)) / sensitivity;
	}
	__ASSERT_NO_MSG(whole >= -512 && whole <= 511);
	return FIELD_PREP(GENMASK(31, 22), whole) | (fraction * GENMASK64(21, 0) / 1000000);
}

static int icm42688_read_imu_from_packet(const uint8_t *pkt, bool is_accel, int fs,
					 uint8_t axis_offset, q31_t *out)
{
	uint32_t unsigned_value;
	int32_t signed_value;
	bool is_hires = FIELD_GET(FIFO_HEADER_ACCEL, pkt[0]) == 1;
	int offset = 1 + (axis_offset * 2);

	const uint32_t scale[2][2] = {
		/* low-res,	hi-res */
		{35744,		8936}, /* gyro */
		{40168,		2511}, /* accel */
	};

	if (!is_accel && FIELD_GET(FIFO_HEADER_ACCEL, pkt[0]) == 1) {
		offset += 6;
	}

	unsigned_value = (pkt[offset] << 8) | pkt[offset + 1];

	if (is_hires) {
		uint32_t mask = is_accel ? GENMASK(7, 4) : GENMASK(3, 0);
		offset = 17 + axis_offset;
		unsigned_value = (unsigned_value << 4) | FIELD_GET(mask, pkt[offset]);
		signed_value = unsigned_value | (0 - (unsigned_value & BIT(19)));
	} else {
		signed_value = unsigned_value | (0 - (unsigned_value & BIT(16)));
	}

	*out = (q31_t)(signed_value * scale[is_accel][is_hires]);
	return 0;
}

static uint32_t accel_period_ns[] = {
	[ICM42688_DT_ACCEL_ODR_1_5625] = UINT32_C(10000000000000) / 15625,
	[ICM42688_DT_ACCEL_ODR_3_125] = UINT32_C(10000000000000) / 31250,
	[ICM42688_DT_ACCEL_ODR_6_25] = UINT32_C(10000000000000) / 62500,
	[ICM42688_DT_ACCEL_ODR_12_5] = UINT32_C(10000000000000) / 12500,
	[ICM42688_DT_ACCEL_ODR_25] = UINT32_C(1000000000) / 25,
	[ICM42688_DT_ACCEL_ODR_50] = UINT32_C(1000000000) / 50,
	[ICM42688_DT_ACCEL_ODR_100] = UINT32_C(1000000000) / 100,
	[ICM42688_DT_ACCEL_ODR_200] = UINT32_C(1000000000) / 200,
	[ICM42688_DT_ACCEL_ODR_500] = UINT32_C(1000000000) / 500,
	[ICM42688_DT_ACCEL_ODR_1000] = UINT32_C(1000000),
	[ICM42688_DT_ACCEL_ODR_2000] = UINT32_C(1000000) / 2,
	[ICM42688_DT_ACCEL_ODR_4000] = UINT32_C(1000000) / 4,
	[ICM42688_DT_ACCEL_ODR_8000] = UINT32_C(1000000) / 8,
	[ICM42688_DT_ACCEL_ODR_16000] = UINT32_C(1000000) / 16,
	[ICM42688_DT_ACCEL_ODR_32000] = UINT32_C(1000000) / 32,
};

static uint32_t gyro_period_ns[] = {
	[ICM42688_DT_GYRO_ODR_12_5] = UINT32_C(10000000000000) / 12500,
	[ICM42688_DT_GYRO_ODR_25] = UINT32_C(1000000000) / 25,
	[ICM42688_DT_GYRO_ODR_50] = UINT32_C(1000000000) / 50,
	[ICM42688_DT_GYRO_ODR_100] = UINT32_C(1000000000) / 100,
	[ICM42688_DT_GYRO_ODR_200] = UINT32_C(1000000000) / 200,
	[ICM42688_DT_GYRO_ODR_500] = UINT32_C(1000000000) / 500,
	[ICM42688_DT_GYRO_ODR_1000] = UINT32_C(1000000),
	[ICM42688_DT_GYRO_ODR_2000] = UINT32_C(1000000) / 2,
	[ICM42688_DT_GYRO_ODR_4000] = UINT32_C(1000000) / 4,
	[ICM42688_DT_GYRO_ODR_8000] = UINT32_C(1000000) / 8,
	[ICM42688_DT_GYRO_ODR_16000] = UINT32_C(1000000) / 16,
	[ICM42688_DT_GYRO_ODR_32000] = UINT32_C(1000000) / 32,
};

static int icm42688_fifo_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct icm42688_fifo_data *edata = (const struct icm42688_fifo_data *)buffer;
	const uint8_t *buffer_end = buffer + sizeof(struct icm42688_fifo_data) + edata->fifo_count;
	int accel_frame_count = 0;
	int gyro_frame_count = 0;
	int count = 0;
	int rc;

	if ((uintptr_t)buffer_end <= *fit || chan_spec.chan_idx != 0) {
		return 0;
	}

	((struct sensor_data_header *)data_out)->base_timestamp_ns = edata->header.timestamp;

	buffer += sizeof(struct icm42688_fifo_data);
	while (count < max_count && buffer < buffer_end) {
		const bool is_20b = FIELD_GET(FIFO_HEADER_20, buffer[0]) == 1;
		const bool has_accel = FIELD_GET(FIFO_HEADER_ACCEL, buffer[0]) == 1;
		const bool has_gyro = FIELD_GET(FIFO_HEADER_GYRO, buffer[0]) == 1;
		const uint8_t *frame_end = buffer;

		if (is_20b) {
			frame_end += 20;
		} else if (has_accel && has_gyro) {
			frame_end += 16;
		} else {
			frame_end += 8;
		}
		if (has_accel) {
			accel_frame_count++;
		}
		if (has_gyro) {
			gyro_frame_count++;
		}

		if ((uintptr_t)buffer < *fit) {
			/* This frame was already decoded, move on to the next frame */
			buffer = frame_end;
			continue;
		}
		if (chan_spec.chan_type == SENSOR_CHAN_DIE_TEMP) {
			struct sensor_q31_data *data = (struct sensor_q31_data *)data_out;

			data->shift = 9;
			if (has_accel) {
				data->readings[count].timestamp_delta =
					accel_period_ns[edata->accel_odr] * (accel_frame_count - 1);
			} else {
				data->readings[count].timestamp_delta =
					gyro_period_ns[edata->gyro_odr] * (gyro_frame_count - 1);
			}
			data->readings[count].temperature =
				icm42688_read_temperature_from_packet(buffer);
		} else if (IS_ACCEL(chan_spec.chan_type) && has_accel) {
			/* Decode accel */
			struct sensor_three_axis_data *data =
				(struct sensor_three_axis_data *)data_out;
			uint64_t period_ns = accel_period_ns[edata->accel_odr];

			icm42688_get_shift(SENSOR_CHAN_ACCEL_XYZ, edata->header.accel_fs,
					   edata->header.gyro_fs, &data->shift);

			data->readings[count].timestamp_delta = (accel_frame_count - 1) * period_ns;
			rc = icm42688_read_imu_from_packet(buffer, true, edata->header.accel_fs, 0,
							   &data->readings[count].x);
			rc |= icm42688_read_imu_from_packet(buffer, true, edata->header.accel_fs, 1,
							    &data->readings[count].y);
			rc |= icm42688_read_imu_from_packet(buffer, true, edata->header.accel_fs, 2,
							    &data->readings[count].z);
			if (rc != 0) {
				accel_frame_count--;
				buffer = frame_end;
				continue;
			}
		} else if (IS_GYRO(chan_spec.chan_type) && has_gyro) {
			/* Decode gyro */
			struct sensor_three_axis_data *data =
				(struct sensor_three_axis_data *)data_out;
			uint64_t period_ns = gyro_period_ns[edata->gyro_odr];

			icm42688_get_shift(SENSOR_CHAN_GYRO_XYZ, edata->header.accel_fs,
					   edata->header.gyro_fs, &data->shift);

			data->readings[count].timestamp_delta = (gyro_frame_count - 1) * period_ns;
			rc = icm42688_read_imu_from_packet(buffer, false, edata->header.gyro_fs, 0,
							   &data->readings[count].x);
			rc |= icm42688_read_imu_from_packet(buffer, false, edata->header.gyro_fs, 1,
							    &data->readings[count].y);
			rc |= icm42688_read_imu_from_packet(buffer, false, edata->header.gyro_fs, 2,
							    &data->readings[count].z);
			if (rc != 0) {
				gyro_frame_count--;
				buffer = frame_end;
				continue;
			}
		}
		buffer = frame_end;
		*fit = (uintptr_t)frame_end;
		count++;
	}
	return count;
}

static int icm42688_one_shot_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct icm42688_encoded_data *edata = (const struct icm42688_encoded_data *)buffer;
	const struct icm42688_decoder_header *header = &edata->header;
	struct icm42688_cfg cfg = {
		.accel_fs = edata->header.accel_fs,
		.gyro_fs = edata->header.gyro_fs,
	};
	uint8_t channel_request;
	int rc;

	if (*fit != 0) {
		return 0;
	}
	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_DIE_TEMP: {
		channel_request = icm42688_encode_channel(chan_spec.chan_type);
		if ((channel_request & edata->channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		rc = icm42688_get_shift(chan_spec.chan_type, header->accel_fs, header->gyro_fs,
					&out->shift);
		if (rc != 0) {
			return -EINVAL;
		}

		icm42688_convert_raw_to_q31(
			&cfg, chan_spec.chan_type,
			edata->readings[icm42688_get_channel_position(chan_spec.chan_type)],
			&out->readings[0].value);
		*fit = 1;
		return 1;
	}
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_XYZ: {
		channel_request = icm42688_encode_channel(chan_spec.chan_type);
		if ((channel_request & edata->channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		rc = icm42688_get_shift(chan_spec.chan_type, header->accel_fs, header->gyro_fs,
					&out->shift);
		if (rc != 0) {
			return -EINVAL;
		}

		icm42688_convert_raw_to_q31(
			&cfg, chan_spec.chan_type - 3,
			edata->readings[icm42688_get_channel_position(chan_spec.chan_type - 3)],
			&out->readings[0].x);
		icm42688_convert_raw_to_q31(
			&cfg, chan_spec.chan_type - 2,
			edata->readings[icm42688_get_channel_position(chan_spec.chan_type - 2)],
			&out->readings[0].y);
		icm42688_convert_raw_to_q31(
			&cfg, chan_spec.chan_type - 1,
			edata->readings[icm42688_get_channel_position(chan_spec.chan_type - 1)],
			&out->readings[0].z);
		*fit = 1;
		return 1;
	}
	default:
		return -EINVAL;
	}
}

static int icm42688_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct icm42688_decoder_header *header =
		(const struct icm42688_decoder_header *)buffer;

	if (header->is_fifo) {
		return icm42688_fifo_decode(buffer, chan_spec, fit, max_count, data_out);
	}
	return icm42688_one_shot_decode(buffer, chan_spec, fit, max_count, data_out);
}

static int icm42688_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	const struct icm42688_fifo_data *data = (const struct icm42688_fifo_data *)buffer;
	const struct icm42688_encoded_data *enc_data = (const struct icm42688_encoded_data *)buffer;
	const struct icm42688_decoder_header *header = &data->header;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	uint8_t channel_request = icm42688_encode_channel(chan_spec.chan_type);


	if ((!enc_data->header.is_fifo) &&
	    (enc_data->channels & channel_request) != channel_request) {
		return -ENODATA;
	}

	if (!header->is_fifo) {
		switch (chan_spec.chan_type) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
		case SENSOR_CHAN_GYRO_X:
		case SENSOR_CHAN_GYRO_Y:
		case SENSOR_CHAN_GYRO_Z:
		case SENSOR_CHAN_GYRO_XYZ:
		case SENSOR_CHAN_DIE_TEMP:
			*frame_count = 1;
			return 0;
		default:
			return -ENOTSUP;
		}
		return 0;
	}

	/* Skip the header */
	buffer += sizeof(struct icm42688_fifo_data);

	uint16_t count = 0;
	const uint8_t *end = buffer + data->fifo_count;

	while (buffer < end) {
		bool is_20b = FIELD_GET(FIFO_HEADER_20, buffer[0]);
		int size = is_20b ? 3 : 2;

		if (FIELD_GET(FIFO_HEADER_ACCEL, buffer[0])) {
			size += 6;
		}
		if (FIELD_GET(FIFO_HEADER_GYRO, buffer[0])) {
			size += 6;
		}
		if (FIELD_GET(FIFO_HEADER_TIMESTAMP_FSYNC, buffer[0])) {
			size += 2;
		}
		if (is_20b) {
			size += 3;
		}

		buffer += size;
		++count;
	}

	*frame_count = count;
	return 0;
}

static int icm42688_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					  size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_DIE_TEMP:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static bool icm24688_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct icm42688_fifo_data *edata = (const struct icm42688_fifo_data *)buffer;

	if (!edata->header.is_fifo) {
		return false;
	}

	switch (trigger) {
	case SENSOR_TRIG_DATA_READY:
		return FIELD_GET(BIT_INT_STATUS_DATA_RDY, edata->int_status);
	case SENSOR_TRIG_FIFO_WATERMARK:
		return FIELD_GET(BIT_INT_STATUS_FIFO_THS, edata->int_status);
	case SENSOR_TRIG_FIFO_FULL:
		return FIELD_GET(BIT_INT_STATUS_FIFO_FULL, edata->int_status);
	default:
		return false;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = icm42688_decoder_get_frame_count,
	.get_size_info = icm42688_decoder_get_size_info,
	.decode = icm42688_decoder_decode,
	.has_trigger = icm24688_decoder_has_trigger,
};

int icm42688_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
