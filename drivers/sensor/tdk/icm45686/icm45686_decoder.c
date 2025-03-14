/*
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor_clock.h>

#include "icm45686.h"
#include "icm45686_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM45686_DECODER, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT invensense_icm45686

static int icm45686_get_shift(enum sensor_channel channel,
			      int accel_fs,
			      int gyro_fs,
			      int8_t *shift)
{
	switch (channel) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		switch (accel_fs) {
		case ICM45686_DT_ACCEL_FS_32:
			*shift = 9;
			return 0;
		case ICM45686_DT_ACCEL_FS_16:
			*shift = 8;
			return 0;
		case ICM45686_DT_ACCEL_FS_8:
			*shift = 7;
			return 0;
		case ICM45686_DT_ACCEL_FS_4:
			*shift = 6;
			return 0;
		case ICM45686_DT_ACCEL_FS_2:
			*shift = 5;
			return 0;
		default:
			return -EINVAL;
		}
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		switch (gyro_fs) {
		case ICM45686_DT_GYRO_FS_4000:
			*shift = 12;
			return 0;
		case ICM45686_DT_GYRO_FS_2000:
			*shift = 11;
			return 0;
		case ICM45686_DT_GYRO_FS_1000:
			*shift = 10;
			return 0;
		case ICM45686_DT_GYRO_FS_500:
			*shift = 9;
			return 0;
		case ICM45686_DT_GYRO_FS_250:
			*shift = 8;
			return 0;
		case ICM45686_DT_GYRO_FS_125:
			*shift = 7;
			return 0;
		case ICM45686_DT_GYRO_FS_62_5:
			*shift = 6;
			return 0;
		case ICM45686_DT_GYRO_FS_31_25:
			*shift = 5;
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

int icm45686_convert_raw_to_q31(struct icm45686_encoded_data *edata,
				enum sensor_channel chan,
				int32_t reading,
				q31_t *out)
{
	int32_t whole;
	int32_t fraction;
	int64_t intermediate;
	int8_t shift;
	int rc;

	rc = icm45686_get_shift(chan, edata->header.accel_fs, edata->header.gyro_fs, &shift);
	if (rc != 0) {
		return rc;
	}

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		icm45686_accel_ms(edata, reading, &whole, &fraction);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		icm45686_gyro_rads(edata, reading, &whole, &fraction);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm45686_temp_c(reading, &whole, &fraction);
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

static int icm45686_get_channel_position(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
		return offsetof(struct icm45686_encoded_payload, accel.x) / sizeof(int16_t);
	case SENSOR_CHAN_ACCEL_Y:
		return offsetof(struct icm45686_encoded_payload, accel.y) / sizeof(int16_t);
	case SENSOR_CHAN_ACCEL_Z:
		return offsetof(struct icm45686_encoded_payload, accel.z) / sizeof(int16_t);
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
		return offsetof(struct icm45686_encoded_payload, gyro.x) / sizeof(int16_t);
	case SENSOR_CHAN_GYRO_Y:
		return offsetof(struct icm45686_encoded_payload, gyro.y) / sizeof(int16_t);
	case SENSOR_CHAN_GYRO_Z:
		return offsetof(struct icm45686_encoded_payload, gyro.z) / sizeof(int16_t);
	case SENSOR_CHAN_DIE_TEMP:
		return offsetof(struct icm45686_encoded_payload, temp) / sizeof(int16_t);
	default:
		return 0;
	}
}

static uint8_t icm45686_encode_channel(enum sensor_channel chan)
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
		encode_bmask = BIT(icm45686_get_channel_position(chan));
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		encode_bmask = BIT(icm45686_get_channel_position(SENSOR_CHAN_ACCEL_X)) |
			       BIT(icm45686_get_channel_position(SENSOR_CHAN_ACCEL_Y)) |
			       BIT(icm45686_get_channel_position(SENSOR_CHAN_ACCEL_Z));
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		encode_bmask = BIT(icm45686_get_channel_position(SENSOR_CHAN_GYRO_X)) |
			       BIT(icm45686_get_channel_position(SENSOR_CHAN_GYRO_Y)) |
			       BIT(icm45686_get_channel_position(SENSOR_CHAN_GYRO_Z));
		break;
	default:
		break;
	}

	return encode_bmask;
}

int icm45686_encode(const struct device *dev,
		    const struct sensor_chan_spec *const channels,
		    const size_t num_channels,
		    uint8_t *buf)
{
	struct icm45686_encoded_data *edata = (struct icm45686_encoded_data *)buf;
	const struct icm45686_config *dev_config = dev->config;
	uint64_t cycles;
	int err;

	edata->header.channels = 0;

	for (size_t i = 0 ; i < num_channels ; i++) {
		edata->header.channels |= icm45686_encode_channel(channels[i].chan_type);
	}

	err = sensor_clock_get_cycles(&cycles);
	if (err != 0) {
		return err;
	}

	edata->header.is_fifo = false;
	edata->header.accel_fs = dev_config->settings.accel.fs;
	edata->header.gyro_fs = dev_config->settings.gyro.fs;
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	return 0;
}

static int icm45686_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	struct icm45686_encoded_data *edata = (struct icm45686_encoded_data *)buffer;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	uint8_t channel_request = icm45686_encode_channel(chan_spec.chan_type);

	if ((!edata->header.is_fifo) &&
	    (edata->header.channels & channel_request) != channel_request) {
		return -ENODATA;
	}

	if (!edata->header.is_fifo) {
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

	LOG_ERR("FIFO Data frame not supported");
	return -1;
}

static int icm45686_decoder_get_size_info(struct sensor_chan_spec chan_spec,
					  size_t *base_size,
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

static int icm45686_one_shot_decode(const uint8_t *buffer,
				    struct sensor_chan_spec chan_spec,
				    uint32_t *fit,
				    uint16_t max_count,
				    void *data_out)
{
	struct icm45686_encoded_data *edata = (struct icm45686_encoded_data *)buffer;
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
		channel_request = icm45686_encode_channel(chan_spec.chan_type);
		if ((channel_request & edata->header.channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		err = icm45686_get_shift(chan_spec.chan_type,
					 edata->header.accel_fs,
					 edata->header.gyro_fs,
					 &out->shift);
		if (err != 0) {
			return -EINVAL;
		}

		icm45686_convert_raw_to_q31(
			edata,
			chan_spec.chan_type,
			edata->payload.readings[icm45686_get_channel_position(chan_spec.chan_type)],
			&out->readings[0].value);
		*fit = 1;
		return 1;
	}
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_XYZ: {
		channel_request = icm45686_encode_channel(chan_spec.chan_type);
		if ((channel_request & edata->header.channels) != channel_request) {
			return -ENODATA;
		}

		struct sensor_three_axis_data *out = data_out;
		struct icm45686_encoded_payload *payload = &edata->payload;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		icm45686_convert_raw_to_q31(
			edata,
			chan_spec.chan_type - 3,
			payload->readings[icm45686_get_channel_position(chan_spec.chan_type - 3)],
			&out->readings[0].x);
		icm45686_convert_raw_to_q31(
			edata,
			chan_spec.chan_type - 2,
			payload->readings[icm45686_get_channel_position(chan_spec.chan_type - 2)],
			&out->readings[0].y);
		icm45686_convert_raw_to_q31(
			edata,
			chan_spec.chan_type - 1,
			payload->readings[icm45686_get_channel_position(chan_spec.chan_type - 1)],
			&out->readings[0].z);
		*fit = 1;
		return 1;
	}
	default:
		return -EINVAL;
	}
}

static int icm45686_decoder_decode(const uint8_t *buffer,
				   struct sensor_chan_spec chan_spec,
				   uint32_t *fit,
				   uint16_t max_count,
				   void *data_out)
{
	struct icm45686_encoded_data *edata = (struct icm45686_encoded_data *)buffer;

	if (edata->header.is_fifo) {
		return -ENOTSUP;
	} else {
		return icm45686_one_shot_decode(buffer, chan_spec, fit, max_count, data_out);
	}
}

static bool icm45686_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = icm45686_decoder_get_frame_count,
	.get_size_info = icm45686_decoder_get_size_info,
	.decode = icm45686_decoder_decode,
	.has_trigger = icm45686_decoder_has_trigger,
};

int icm45686_get_decoder(const struct device *dev,
			 const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
