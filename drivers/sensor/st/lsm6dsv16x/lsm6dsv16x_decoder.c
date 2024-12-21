/* ST Microelectronics LSM6DSV16X 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lsm6dsv16x.h"
#include "lsm6dsv16x_decoder.h"
#include <zephyr/dt-bindings/sensor/lsm6dsv16x.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LSM6DSV16X_DECODER, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_LSM6DSV16X_STREAM
static const uint32_t accel_period_ns[] = {
	[LSM6DSV16X_XL_BATCHED_AT_1Hz875] = UINT32_C(1000000000000) / 1875,
	[LSM6DSV16X_XL_BATCHED_AT_7Hz5] = UINT32_C(1000000000000) / 7500,
	[LSM6DSV16X_XL_BATCHED_AT_15Hz] = UINT32_C(1000000000) / 15,
	[LSM6DSV16X_XL_BATCHED_AT_30Hz] = UINT32_C(1000000000) / 30,
	[LSM6DSV16X_XL_BATCHED_AT_60Hz] = UINT32_C(1000000000) / 60,
	[LSM6DSV16X_XL_BATCHED_AT_120Hz] = UINT32_C(1000000000) / 120,
	[LSM6DSV16X_XL_BATCHED_AT_240Hz] = UINT32_C(1000000000) / 240,
	[LSM6DSV16X_XL_BATCHED_AT_480Hz] = UINT32_C(1000000000) / 480,
	[LSM6DSV16X_XL_BATCHED_AT_960Hz] = UINT32_C(1000000000) / 960,
	[LSM6DSV16X_XL_BATCHED_AT_1920Hz] = UINT32_C(1000000000) / 1920,
	[LSM6DSV16X_XL_BATCHED_AT_3840Hz] = UINT32_C(1000000000) / 3840,
	[LSM6DSV16X_XL_BATCHED_AT_7680Hz] = UINT32_C(1000000000) / 7680,
};

static const uint32_t gyro_period_ns[] = {
	[LSM6DSV16X_GY_BATCHED_AT_1Hz875] = UINT32_C(1000000000000) / 1875,
	[LSM6DSV16X_GY_BATCHED_AT_7Hz5] = UINT32_C(1000000000000) / 7500,
	[LSM6DSV16X_GY_BATCHED_AT_15Hz] = UINT32_C(1000000000) / 15,
	[LSM6DSV16X_GY_BATCHED_AT_30Hz] = UINT32_C(1000000000) / 30,
	[LSM6DSV16X_GY_BATCHED_AT_60Hz] = UINT32_C(1000000000) / 60,
	[LSM6DSV16X_GY_BATCHED_AT_120Hz] = UINT32_C(1000000000) / 120,
	[LSM6DSV16X_GY_BATCHED_AT_240Hz] = UINT32_C(1000000000) / 240,
	[LSM6DSV16X_GY_BATCHED_AT_480Hz] = UINT32_C(1000000000) / 480,
	[LSM6DSV16X_GY_BATCHED_AT_960Hz] = UINT32_C(1000000000) / 960,
	[LSM6DSV16X_GY_BATCHED_AT_1920Hz] = UINT32_C(1000000000) / 1920,
	[LSM6DSV16X_GY_BATCHED_AT_3840Hz] = UINT32_C(1000000000) / 3840,
	[LSM6DSV16X_GY_BATCHED_AT_7680Hz] = UINT32_C(1000000000) / 7680,
};

#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
static const uint32_t temp_period_ns[] = {
	[LSM6DSV16X_TEMP_BATCHED_AT_1Hz875] = UINT32_C(1000000000000) / 1875,
	[LSM6DSV16X_TEMP_BATCHED_AT_15Hz] = UINT32_C(1000000000) / 15,
	[LSM6DSV16X_TEMP_BATCHED_AT_60Hz] = UINT32_C(1000000000) / 60,
};
#endif
#endif /* CONFIG_LSM6DSV16X_STREAM */

/*
 * Expand micro_val (a generic micro unit) to q31_t according to its range; this is achieved
 * multiplying by 2^31/2^range. Then transform it to val.
 */
#define Q31_SHIFT_MICROVAL(micro_val, range) \
	(q31_t) ((int64_t)(micro_val) * ((int64_t)1 << (31 - (range))) / 1000000LL)

/* bit range for Accelerometer for a given fs */
static const int8_t accel_range[] = {
	[LSM6DSV16X_DT_FS_2G] = 5,
	[LSM6DSV16X_DT_FS_4G] = 6,
	[LSM6DSV16X_DT_FS_8G] = 7,
	[LSM6DSV16X_DT_FS_16G] = 8,
};

/* bit range for Gyroscope for a given fs */
static const int8_t gyro_range[] = {
	[LSM6DSV16X_DT_FS_125DPS] = 2,
	[LSM6DSV16X_DT_FS_250DPS] = 3,
	[LSM6DSV16X_DT_FS_500DPS] = 4,
	[LSM6DSV16X_DT_FS_1000DPS] = 5,
	[LSM6DSV16X_DT_FS_2000DPS] = 6,
	[LSM6DSV16X_DT_FS_4000DPS] = 7,
};

#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
/* bit range for Temperature sensor */
static const int8_t temp_range = 9;

/* transform temperature LSB into micro-Celsius */
#define SENSOR_TEMP_UCELSIUS(t_lsb) \
	(int64_t) (25000000LL + (((int64_t)(t_lsb) * 1000000LL) / 256LL))

#endif

/* Calculate scaling factor to transform micro-g/LSB unit into micro-ms2/LSB */
#define SENSOR_SCALE_UG_TO_UMS2(ug_lsb)	\
	(int32_t)((ug_lsb) * SENSOR_G / 1000000LL)

/*
 * Accelerometer scaling factors table (indexed by full scale)
 * GAIN_UNIT_XL is expressed in ug/LSB.
 */
static const int32_t accel_scaler[] = {
	[LSM6DSV16X_DT_FS_2G] = SENSOR_SCALE_UG_TO_UMS2(GAIN_UNIT_XL),
	[LSM6DSV16X_DT_FS_4G] = SENSOR_SCALE_UG_TO_UMS2(2 * GAIN_UNIT_XL),
	[LSM6DSV16X_DT_FS_8G] = SENSOR_SCALE_UG_TO_UMS2(4 * GAIN_UNIT_XL),
	[LSM6DSV16X_DT_FS_16G] = SENSOR_SCALE_UG_TO_UMS2(8 * GAIN_UNIT_XL),
};

/* Calculate scaling factor to transform micro-dps/LSB unit into micro-rads/LSB */
#define SENSOR_SCALE_UDPS_TO_URADS(udps_lsb) \
	(int32_t)(((udps_lsb) * SENSOR_PI / 180LL) / 1000000LL)

/*
 * Accelerometer scaling factors table (indexed by full scale)
 * GAIN_UNIT_G is expressed in udps/LSB.
 */
static const int32_t gyro_scaler[] = {
	[LSM6DSV16X_DT_FS_125DPS] = SENSOR_SCALE_UDPS_TO_URADS(GAIN_UNIT_G),
	[LSM6DSV16X_DT_FS_250DPS] = SENSOR_SCALE_UDPS_TO_URADS(2 * GAIN_UNIT_G),
	[LSM6DSV16X_DT_FS_500DPS] = SENSOR_SCALE_UDPS_TO_URADS(4 * GAIN_UNIT_G),
	[LSM6DSV16X_DT_FS_1000DPS] = SENSOR_SCALE_UDPS_TO_URADS(8 * GAIN_UNIT_G),
	[LSM6DSV16X_DT_FS_2000DPS] = SENSOR_SCALE_UDPS_TO_URADS(16 * GAIN_UNIT_G),
	[LSM6DSV16X_DT_FS_4000DPS] = SENSOR_SCALE_UDPS_TO_URADS(32 * GAIN_UNIT_G),
};

static int lsm6dsv16x_decoder_get_frame_count(const uint8_t *buffer,
					      struct sensor_chan_spec chan_spec,
					      uint16_t *frame_count)
{
	struct lsm6dsv16x_fifo_data *data = (struct lsm6dsv16x_fifo_data *)buffer;
	const struct lsm6dsv16x_decoder_header *header = &data->header;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
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

#ifdef CONFIG_LSM6DSV16X_STREAM
	*frame_count = data->fifo_count;
#endif
	return 0;
}

#ifdef CONFIG_LSM6DSV16X_STREAM
static int lsm6dsv16x_decode_fifo(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				  uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct lsm6dsv16x_fifo_data *edata = (const struct lsm6dsv16x_fifo_data *)buffer;
	const uint8_t *buffer_end, *tmp_buffer;
	const struct lsm6dsv16x_decoder_header *header = &edata->header;
	int count = 0;
	uint8_t fifo_tag;
	uint16_t xl_count = 0, gy_count = 0;
	uint8_t tot_accel_fifo_words = 0, tot_gyro_fifo_words = 0;

#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
	uint8_t tot_temp_fifo_words = 0;
	uint16_t temp_count = 0;
#endif

	buffer += sizeof(struct lsm6dsv16x_fifo_data);
	buffer_end = buffer + LSM6DSV16X_FIFO_SIZE(edata->fifo_count);

	/* count total FIFO word for each tag */
	tmp_buffer = buffer;
	while (tmp_buffer < buffer_end) {
		fifo_tag = (tmp_buffer[0] >> 3);

		switch (fifo_tag) {
		case LSM6DSV16X_XL_NC_TAG:
			tot_accel_fifo_words++;
			break;
		case LSM6DSV16X_GY_NC_TAG:
			tot_gyro_fifo_words++;
			break;
#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
		case LSM6DSV16X_TEMPERATURE_TAG:
			tot_temp_fifo_words++;
			break;
#endif
		default:
			break;
		}

		tmp_buffer += LSM6DSV16X_FIFO_ITEM_LEN;
	}

	/*
	 * Timestamp in header is set when FIFO threshold is reached, so
	 * set time baseline going back in past according to total number
	 * of FIFO word for each type.
	 */
	if (SENSOR_CHANNEL_IS_ACCEL(chan_spec.chan_type)) {
		((struct sensor_data_header *)data_out)->base_timestamp_ns =
			edata->header.timestamp -
			(tot_accel_fifo_words - 1) * accel_period_ns[edata->accel_batch_odr];
	} else if (SENSOR_CHANNEL_IS_GYRO(chan_spec.chan_type)) {
		((struct sensor_data_header *)data_out)->base_timestamp_ns =
			edata->header.timestamp -
			(tot_gyro_fifo_words - 1) * gyro_period_ns[edata->gyro_batch_odr];
#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
	} else if (chan_spec.chan_type == SENSOR_CHAN_DIE_TEMP) {
		((struct sensor_data_header *)data_out)->base_timestamp_ns =
			edata->header.timestamp -
			(tot_temp_fifo_words - 1) * temp_period_ns[edata->temp_batch_odr];
#endif
	}

	while (count < max_count && buffer < buffer_end) {
		const uint8_t *frame_end = buffer;
		uint8_t skip_frame;

		skip_frame = 0;
		frame_end += LSM6DSV16X_FIFO_ITEM_LEN;

		fifo_tag = (buffer[0] >> 3);

		switch (fifo_tag) {
		case LSM6DSV16X_XL_NC_TAG: {
			struct sensor_three_axis_data *out = data_out;
			int16_t x, y, z;
			const int32_t scale = accel_scaler[header->accel_fs];

			xl_count++;
			if ((uintptr_t)buffer < *fit) {
				/* This frame was already decoded, move on to the next frame */
				buffer = frame_end;
				continue;
			}

			if (!SENSOR_CHANNEL_IS_ACCEL(chan_spec.chan_type)) {
				buffer = frame_end;
				continue;
			}

			out->readings[count].timestamp_delta =
				(xl_count - 1) * accel_period_ns[edata->accel_batch_odr];

			x = *(int16_t *)&buffer[1];
			y = *(int16_t *)&buffer[3];
			z = *(int16_t *)&buffer[5];

			out->shift = accel_range[header->accel_fs];

			out->readings[count].x = Q31_SHIFT_MICROVAL(scale * x, out->shift);
			out->readings[count].y = Q31_SHIFT_MICROVAL(scale * y, out->shift);
			out->readings[count].z = Q31_SHIFT_MICROVAL(scale * z, out->shift);
			break;
		}
		case LSM6DSV16X_GY_NC_TAG: {
			struct sensor_three_axis_data *out = data_out;
			int16_t x, y, z;
			const int32_t scale = gyro_scaler[header->gyro_fs];

			gy_count++;
			if ((uintptr_t)buffer < *fit) {
				/* This frame was already decoded, move on to the next frame */
				buffer = frame_end;
				continue;
			}

			if (!SENSOR_CHANNEL_IS_GYRO(chan_spec.chan_type)) {
				buffer = frame_end;
				continue;
			}

			out->readings[count].timestamp_delta =
				(gy_count - 1) * gyro_period_ns[edata->gyro_batch_odr];

			x = *(int16_t *)&buffer[1];
			y = *(int16_t *)&buffer[3];
			z = *(int16_t *)&buffer[5];

			out->shift = gyro_range[header->gyro_fs];

			out->readings[count].x = Q31_SHIFT_MICROVAL(scale * x, out->shift);
			out->readings[count].y = Q31_SHIFT_MICROVAL(scale * y, out->shift);
			out->readings[count].z = Q31_SHIFT_MICROVAL(scale * z, out->shift);
			break;
		}
#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
		case LSM6DSV16X_TEMPERATURE_TAG: {
			struct sensor_q31_data *out = data_out;
			int16_t t;
			int64_t t_uC;

			temp_count++;
			if ((uintptr_t)buffer < *fit) {
				/* This frame was already decoded, move on to the next frame */
				buffer = frame_end;
				continue;
			}

			if (chan_spec.chan_type != SENSOR_CHAN_DIE_TEMP) {
				buffer = frame_end;
				continue;
			}

			out->readings[count].timestamp_delta =
				(temp_count - 1) * temp_period_ns[edata->temp_batch_odr];

			t = *(int16_t *)&buffer[1];
			t_uC = SENSOR_TEMP_UCELSIUS(t);

			out->shift = temp_range;

			out->readings[count].temperature = Q31_SHIFT_MICROVAL(t_uC, out->shift);
			break;
		}
#endif
		default:
			/* skip unhandled FIFO tag */
			buffer = frame_end;
			LOG_DBG("unknown FIFO tag %02x", fifo_tag);
			continue;
		}

		buffer = frame_end;
		*fit = (uintptr_t)frame_end;
		count++;
	}

	return count;
}
#endif /* CONFIG_LSM6DSV16X_STREAM */

static int lsm6dsv16x_decode_sample(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct lsm6dsv16x_rtio_data *edata = (const struct lsm6dsv16x_rtio_data *)buffer;
	const struct lsm6dsv16x_decoder_header *header = &edata->header;

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
	case SENSOR_CHAN_ACCEL_XYZ: {
		const int32_t scale = accel_scaler[header->accel_fs];

		if (edata->has_accel == 0) {
			return -ENODATA;
		}

		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		out->shift = accel_range[header->accel_fs];

		out->readings[0].x = Q31_SHIFT_MICROVAL(scale * edata->acc[0], out->shift);
		out->readings[0].y = Q31_SHIFT_MICROVAL(scale * edata->acc[1], out->shift);
		out->readings[0].z = Q31_SHIFT_MICROVAL(scale * edata->acc[2], out->shift);
		*fit = 1;
		return 1;
	}
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ: {
		const int32_t scale = gyro_scaler[header->gyro_fs];

		if (edata->has_gyro == 0) {
			return -ENODATA;
		}

		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		out->shift = gyro_range[header->gyro_fs];

		out->readings[0].x = Q31_SHIFT_MICROVAL(scale * edata->gyro[0], out->shift);
		out->readings[0].y = Q31_SHIFT_MICROVAL(scale * edata->gyro[1], out->shift);
		out->readings[0].z = Q31_SHIFT_MICROVAL(scale * edata->gyro[2], out->shift);
		*fit = 1;
		return 1;
	}
#if defined(CONFIG_LSM6DSV16X_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP: {
		int64_t t_uC;

		if (edata->has_temp == 0) {
			return -ENODATA;
		}

		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		out->shift = temp_range;

		/* transform temperature LSB into micro-Celsius */
		t_uC = SENSOR_TEMP_UCELSIUS(edata->temp);

		out->readings[0].temperature = Q31_SHIFT_MICROVAL(t_uC, out->shift);
		*fit = 1;
		return 1;
	}
#endif
	default:
		return -EINVAL;
	}
}

static int lsm6dsv16x_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				     uint32_t *fit, uint16_t max_count, void *data_out)
{
#ifdef CONFIG_LSM6DSV16X_STREAM
	const struct lsm6dsv16x_decoder_header *header =
		(const struct lsm6dsv16x_decoder_header *)buffer;

	if (header->is_fifo) {
		return lsm6dsv16x_decode_fifo(buffer, chan_spec, fit, max_count, data_out);
	}
#endif

	return lsm6dsv16x_decode_sample(buffer, chan_spec, fit, max_count, data_out);
}

static int lsm6dsv16x_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					    size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	case SENSOR_CHAN_DIE_TEMP:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static bool lsm6dsv16x_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = lsm6dsv16x_decoder_get_frame_count,
	.get_size_info = lsm6dsv16x_decoder_get_size_info,
	.decode = lsm6dsv16x_decoder_decode,
	.has_trigger = lsm6dsv16x_decoder_has_trigger,
};

int lsm6dsv16x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
