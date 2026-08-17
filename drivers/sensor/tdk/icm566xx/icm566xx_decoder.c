/*
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys/check.h>

#include "icm566xx.h"
#include "icm566xx_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM566XX_DECODER, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT invensense_icm566xx

#if defined(CONFIG_DT_HAS_INVENSENSE_ICM56686_ENABLED)
static int32_t get_raw_reading_by_position(struct icm566xx_encoded_data *edata, int pos)
{
	int32_t high_16 = (int32_t)edata->payload.readings[pos];
	int32_t low_4 = 0;

	if (pos >= 0 && pos <= 2) {
		low_4 = edata->payload.ext_data[pos] & 0x0F;
	} else if (pos >= 3 && pos <= 5) {
		low_4 = edata->payload.ext_data[pos - 3] >> 4;
	} else {
		return -EINVAL;
	}

	int32_t raw_val = (high_16 << 4) | low_4;

	if (raw_val & 0x80000) {
		raw_val |= 0xFFF00000;
	}

	return raw_val;
}
#endif

static int icm566xx_get_shift(enum sensor_channel channel, int accel_fs, int gyro_fs, int8_t *shift)
{
	int8_t val = 0;

	switch (channel) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		switch (accel_fs) {
		case ICM566XX_DT_ACCEL_FS_32:
			val = 9;
			break;
		case ICM566XX_DT_ACCEL_FS_16:
			val = 8;
			break;
		case ICM566XX_DT_ACCEL_FS_8:
			val = 7;
			break;
		case ICM566XX_DT_ACCEL_FS_4:
			val = 6;
			break;
		case ICM566XX_DT_ACCEL_FS_2:
			val = 5;
			break;
		default:
			return -EINVAL;
		}

#if defined(CONFIG_DT_HAS_INVENSENSE_ICM56686_ENABLED)
		val += 4;
#endif
		*shift = val;
		return 0;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		switch (gyro_fs) {
		case ICM566XX_DT_GYRO_FS_4000:
			val = 12;
			break;
		case ICM566XX_DT_GYRO_FS_2000:
			val = 11;
			break;
		case ICM566XX_DT_GYRO_FS_1000:
			val = 10;
			break;
		case ICM566XX_DT_GYRO_FS_500:
			val = 9;
			break;
		case ICM566XX_DT_GYRO_FS_250:
			val = 8;
			break;
		case ICM566XX_DT_GYRO_FS_125:
			val = 7;
			break;
		case ICM566XX_DT_GYRO_FS_62_5:
			val = 6;
			break;
		case ICM566XX_DT_GYRO_FS_31_25:
			val = 5;
			break;
		default:
			return -EINVAL;
		}

#if defined(CONFIG_DT_HAS_INVENSENSE_ICM56686_ENABLED)
		val += 4;
#endif
		*shift = val;
		return 0;
	case SENSOR_CHAN_DIE_TEMP:
		*shift = 9;
		return 0;
	default:
		return -EINVAL;
	}
}

int icm566xx_convert_raw_to_q31(struct icm566xx_encoded_data *edata, enum sensor_channel chan,
				int32_t reading, q31_t *out)
{
	int32_t whole;
	int32_t fraction;
	int64_t intermediate;
	int8_t shift;
	int rc;
	bool is_high_res = false;

#if INV_IMU_20BIT_REG_DATA_SUPPORTED
	is_high_res = true;
#endif

	rc = icm566xx_get_shift(chan, edata->header.accel_fs, edata->header.gyro_fs, &shift);
	if (rc != 0) {
		return rc;
	}

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		icm566xx_accel_ms(edata->header.accel_fs, reading, is_high_res, &whole, &fraction);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		icm566xx_gyro_rads(edata->header.gyro_fs, reading, is_high_res, &whole, &fraction);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm566xx_temp_c(reading, &whole, &fraction);
		break;
	default:
		return -ENOTSUP;
	}

	if (shift < 0) {
		intermediate = ((int64_t)whole * INT64_C(1000000) + fraction);
		intermediate = intermediate * (1 << -shift);
		intermediate = (intermediate * ((int64_t)INT32_MAX + 1)) / INT64_C(1000000);
	} else {
		int64_t w_q31 = ((int64_t)whole * ((int64_t)INT32_MAX + 1)) >> shift;
		int64_t f_q31 = ((int64_t)fraction * ((int64_t)INT32_MAX + 1)) / INT64_C(1000000);

		f_q31 = f_q31 >> shift;
		intermediate = w_q31 + f_q31;
	}

	*out = CLAMP(intermediate, INT32_MIN, INT32_MAX);

	return 0;
}

static int icm566xx_get_channel_position(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
		return 0;
	case SENSOR_CHAN_ACCEL_Y:
		return 1;
	case SENSOR_CHAN_ACCEL_Z:
		return 2;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
		return 3;
	case SENSOR_CHAN_GYRO_Y:
		return 4;
	case SENSOR_CHAN_GYRO_Z:
		return 5;
	case SENSOR_CHAN_DIE_TEMP:
		return 6;
	default:
		return 0;
	}
}

static uint8_t icm566xx_encode_channel(enum sensor_channel chan)
{
	uint8_t encode_bmask = 0;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_DIE_TEMP:
		encode_bmask = BIT(icm566xx_get_channel_position(chan));
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		encode_bmask = BIT(icm566xx_get_channel_position(SENSOR_CHAN_ACCEL_X)) |
			       BIT(icm566xx_get_channel_position(SENSOR_CHAN_ACCEL_Y)) |
			       BIT(icm566xx_get_channel_position(SENSOR_CHAN_ACCEL_Z));
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		encode_bmask = BIT(icm566xx_get_channel_position(SENSOR_CHAN_GYRO_X)) |
			       BIT(icm566xx_get_channel_position(SENSOR_CHAN_GYRO_Y)) |
			       BIT(icm566xx_get_channel_position(SENSOR_CHAN_GYRO_Z));
		break;
	default:
		break;
	}

	return encode_bmask;
}

int icm566xx_encode(const struct device *dev, const struct sensor_chan_spec *const channels,
		    const size_t num_channels, uint8_t *buf)
{
	struct icm566xx_encoded_data *edata = (struct icm566xx_encoded_data *)buf;
	const struct icm566xx_config *dev_config = dev->config;
	uint64_t cycles;
	int err;

	edata->header.channels = 0;

	for (size_t i = 0; i < num_channels; i++) {
		edata->header.channels |= icm566xx_encode_channel(channels[i].chan_type);
	}

	err = sensor_clock_get_cycles(&cycles);
	if (err != 0) {
		return err;
	}

	edata->header.events = 0;
	edata->header.accel_fs = dev_config->settings.accel.fs;
	edata->header.gyro_fs = dev_config->settings.gyro.fs;
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	return 0;
}

static int icm566xx_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	struct icm566xx_encoded_data *edata = (struct icm566xx_encoded_data *)buffer;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	uint8_t channel_request = icm566xx_encode_channel(chan_spec.chan_type);

	if ((edata->header.channels & channel_request) != channel_request) {
		return -ENODATA;
	}

	uint8_t ev_raw = edata->header.events;
	encoded_events_t *ev_status = (encoded_events_t *)&ev_raw;

	if (ev_raw == 0 || ev_status->int1_status_drdy) {
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
	}

	if (ev_status->int1_status_fifo_ths ||
		ev_status->int1_status_fifo_full) {
		switch (chan_spec.chan_type) {
		case SENSOR_CHAN_ACCEL_XYZ:
		case SENSOR_CHAN_GYRO_XYZ:
		case SENSOR_CHAN_DIE_TEMP:
			*frame_count = edata->header.fifo_count;
			return 0;
		/** We're skipping individual axis for fifo packets */
		default:
			return -ENOTSUP;
		}
	}

	return -ENODATA;
}

static int icm566xx_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
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

static int icm566xx_one_shot_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	struct icm566xx_encoded_data *edata = (struct icm566xx_encoded_data *)buffer;
	uint8_t channel_request;
	int err;

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
		channel_request = icm566xx_encode_channel(chan_spec.chan_type);
		if ((channel_request & edata->header.channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		err = icm566xx_get_shift(chan_spec.chan_type, edata->header.accel_fs,
					 edata->header.gyro_fs, &out->shift);
		if (err != 0) {
			return -EINVAL;
		}

		int32_t raw_reading;
#if defined(CONFIG_DT_HAS_INVENSENSE_ICM56686_ENABLED)
		int pos = icm566xx_get_channel_position(chan_spec.chan_type);

		raw_reading = get_raw_reading_by_position(edata, pos);
#else
		raw_reading =
			edata->payload.readings[icm566xx_get_channel_position(chan_spec.chan_type)];
#endif

		icm566xx_convert_raw_to_q31(edata, chan_spec.chan_type, raw_reading,
					    &out->readings[0].value);
		*fit = 1;
		return 1;
	}
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_XYZ: {
		channel_request = icm566xx_encode_channel(chan_spec.chan_type);
		if ((channel_request & edata->header.channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		err = icm566xx_get_shift(chan_spec.chan_type, edata->header.accel_fs,
					 edata->header.gyro_fs, &out->shift);
		if (err != 0) {
			return -EINVAL;
		}

#if defined(CONFIG_DT_HAS_INVENSENSE_ICM56686_ENABLED)
		int base_pos = (chan_spec.chan_type == SENSOR_CHAN_ACCEL_XYZ) ? 0 : 3;
		enum sensor_channel base_chan = (chan_spec.chan_type == SENSOR_CHAN_ACCEL_XYZ)
							? SENSOR_CHAN_ACCEL_X
							: SENSOR_CHAN_GYRO_X;
		int32_t rx = get_raw_reading_by_position(edata, base_pos);
		int32_t ry = get_raw_reading_by_position(edata, base_pos + 1);
		int32_t rz = get_raw_reading_by_position(edata, base_pos + 2);

		err |= icm566xx_convert_raw_to_q31(edata, base_chan, rx, &out->readings[0].x);
		err |= icm566xx_convert_raw_to_q31(edata, base_chan + 1, ry, &out->readings[0].y);
		err |= icm566xx_convert_raw_to_q31(edata, base_chan + 2, rz, &out->readings[0].z);
#else
		struct icm566xx_encoded_payload *payload = &edata->payload;

		err |= icm566xx_convert_raw_to_q31(
			edata, chan_spec.chan_type - 3,
			payload->readings[icm566xx_get_channel_position(chan_spec.chan_type - 3)],
			&out->readings[0].x);
		err |= icm566xx_convert_raw_to_q31(
			edata, chan_spec.chan_type - 2,
			payload->readings[icm566xx_get_channel_position(chan_spec.chan_type - 2)],
			&out->readings[0].y);
		err |= icm566xx_convert_raw_to_q31(
			edata, chan_spec.chan_type - 1,
			payload->readings[icm566xx_get_channel_position(chan_spec.chan_type - 1)],
			&out->readings[0].z);
#endif
		if (err != 0) {
			return -EINVAL;
		}

		*fit = 1;
		return 1;
	}
	default:
		return -EINVAL;
	}
}

static q31_t icm566xx_fifo_read_temp_from_packet(const uint8_t *pkt)
{
	struct icm566xx_encoded_fifo_payload *fdata = (struct icm566xx_encoded_fifo_payload *)pkt;

	int32_t whole;
	uint32_t fraction;
	int64_t intermediate;
	int8_t shift;
	int err;

	err = icm566xx_get_shift(SENSOR_CHAN_DIE_TEMP, 0, 0, &shift);
	if (err != 0) {
		return -1;
	}

	icm566xx_temp_c(fdata->temp, &whole, &fraction);

	intermediate = ((int64_t)whole * INT64_C(1000000) + fraction);
	if (shift < 0) {
		intermediate =
			intermediate * ((int64_t)INT32_MAX + 1) * (1 << -shift) / INT64_C(1000000);
	} else {
		intermediate =
			intermediate * ((int64_t)INT32_MAX + 1) / ((1 << shift) * INT64_C(1000000));
	}

	return CLAMP(intermediate, INT32_MIN, INT32_MAX);
}

static int icm566xx_fifo_read_imu_from_packet(const uint8_t *pkt, bool is_accel,
					      uint8_t axis_offset, q31_t *out)
{
	uint32_t unsigned_value;
	int32_t signed_value;
	int offset = 1 + (axis_offset * 2) + (is_accel ? 0 : 6);
	uint32_t mask = is_accel ? GENMASK(7, 4) : GENMASK(3, 0);
	uint8_t accel_fs = ICM566XX_DT_ACCEL_FS_32;
	uint8_t gyro_fs = ICM566XX_DT_GYRO_FS_4000;

	int32_t whole;
	int32_t fraction;
	int64_t intermediate;
	int8_t shift;

	unsigned_value = (pkt[offset] | (pkt[offset + 1] << 8));
	if (unsigned_value == FIFO_NO_DATA) {
		return -ENODATA;
	}

	unsigned_value =
		(unsigned_value << 4) | ((pkt[17 + axis_offset] & mask) >> (is_accel ? 4 : 0));
	signed_value = sign_extend(unsigned_value, 19);

	if (!is_accel) {
		icm566xx_get_shift(SENSOR_CHAN_GYRO_XYZ, accel_fs, gyro_fs, &shift);
		icm566xx_gyro_rads(gyro_fs, signed_value, true, &whole, &fraction);
	} else {
		icm566xx_get_shift(SENSOR_CHAN_ACCEL_XYZ, accel_fs, gyro_fs, &shift);
		icm566xx_accel_ms(accel_fs, signed_value, true, &whole, &fraction);
	}

	intermediate = ((int64_t)whole * INT64_C(1000000) + fraction);
	if (shift < 0) {
		intermediate =
			intermediate * ((int64_t)INT32_MAX + 1) * (1 << -shift) / INT64_C(1000000);
	} else {
		intermediate =
			intermediate * ((int64_t)INT32_MAX + 1) / ((1 << shift) * INT64_C(1000000));
	}

	*out = CLAMP(intermediate, INT32_MIN, INT32_MAX);

	return 0;
}

static int icm566xx_fifo_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				uint32_t *fit, uint16_t max_count, void *data_out)
{
	struct icm566xx_encoded_data *edata = (struct icm566xx_encoded_data *)buffer;
	struct icm566xx_encoded_fifo_payload *frame_begin = edata->fifo_payload;
	int count = 0;
	int err;

	if (*fit >= edata->header.fifo_count || chan_spec.chan_idx != 0) {
		return 0;
	}

	while (count < max_count && (*fit < edata->header.fifo_count)) {
		struct icm566xx_encoded_fifo_payload *fdata = &frame_begin[*fit];
		fifo_header_t *header = (fifo_header_t *)&fdata->header;

		/** This driver assumes 20-byte fifo packets, with both accel and gyro,
		 * and no auxiliary sensors.
		 */
		CHECKIF(header->bits.ext_header    ||
				!header->bits.accel_bit    ||
				!header->bits.gyro_bit     ||
				!header->bits.twentybits_bit) {
			LOG_ERR("Unsupported FIFO packet format 0x%02x", fdata->header);
			return -ENOTSUP;
		}

		switch (chan_spec.chan_type) {
		case SENSOR_CHAN_ACCEL_XYZ:
		case SENSOR_CHAN_GYRO_XYZ: {
			struct sensor_three_axis_data *out = data_out;
			bool is_accel = chan_spec.chan_type == SENSOR_CHAN_ACCEL_XYZ;

			icm566xx_get_shift(chan_spec.chan_type, edata->header.accel_fs,
					   edata->header.gyro_fs, &out->shift);

			out->header.base_timestamp_ns = edata->header.timestamp;

			err = icm566xx_fifo_read_imu_from_packet((uint8_t *)fdata, is_accel, 0,
								 &out->readings[count].x);
			err |= icm566xx_fifo_read_imu_from_packet((uint8_t *)fdata, is_accel, 1,
								  &out->readings[count].y);
			err |= icm566xx_fifo_read_imu_from_packet((uint8_t *)fdata, is_accel, 2,
								  &out->readings[count].z);
			if (err != 0) {
				count--;
			}
			break;
		}
		case SENSOR_CHAN_DIE_TEMP: {
			struct sensor_q31_data *out = data_out;

			icm566xx_get_shift(chan_spec.chan_type, edata->header.accel_fs,
					   edata->header.gyro_fs, &out->shift);

			out->header.base_timestamp_ns = edata->header.timestamp;
			out->readings[count].temperature =
				icm566xx_fifo_read_temp_from_packet((uint8_t *)fdata);
			break;
		}
		default:
			return 0;
		}
		*fit = *fit + 1;
		count++;
	}

	return count;
}

static int icm566xx_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	struct icm566xx_encoded_data *edata = (struct icm566xx_encoded_data *)buffer;
	uint8_t ev_raw = edata->header.events;
	encoded_events_t ev_status = *(encoded_events_t *)&ev_raw;

	if (ev_status.int1_status_fifo_ths ||
		ev_status.int1_status_fifo_full) {
		return icm566xx_fifo_decode(buffer, chan_spec, fit, max_count, data_out);
	} else {
		return icm566xx_one_shot_decode(buffer, chan_spec, fit, max_count, data_out);
	}
}

static bool icm566xx_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	struct icm566xx_encoded_data *edata = (struct icm566xx_encoded_data *)buffer;
	uint8_t ev_raw = edata->header.events;
	encoded_events_t ev_status = *(encoded_events_t *)&ev_raw;

	switch (trigger) {
	case SENSOR_TRIG_DATA_READY:
		return ev_status.int1_status_drdy;
	case SENSOR_TRIG_FIFO_WATERMARK:
		return ev_status.int1_status_fifo_ths;
	case SENSOR_TRIG_FIFO_FULL:
		return ev_status.int1_status_fifo_full;
	default:
		break;
	}

	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = icm566xx_decoder_get_frame_count,
	.get_size_info = icm566xx_decoder_get_size_info,
	.decode = icm566xx_decoder_decode,
	.has_trigger = icm566xx_decoder_has_trigger,
};

int icm566xx_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
