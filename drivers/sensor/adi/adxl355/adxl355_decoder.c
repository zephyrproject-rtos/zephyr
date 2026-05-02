/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "adxl355.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ADXL355);

#ifdef CONFIG_ADXL355_STREAM
/**
 * @brief Accelerometer output data rate periods in nanoseconds
 *
 */
static const uint32_t accel_period_ns[] = {
	[ADXL355_ODR_4000HZ] = UINT32_C(1000000000) / 4000,
	[ADXL355_ODR_2000HZ] = UINT32_C(1000000000) / 2000,
	[ADXL355_ODR_1000HZ] = UINT32_C(1000000000) / 1000,
	[ADXL355_ODR_500HZ] = UINT32_C(1000000000) / 500,
	[ADXL355_ODR_250HZ] = UINT32_C(1000000000) / 250,
	[ADXL355_ODR_125HZ] = UINT32_C(1000000000) / 125,
	[ADXL355_ODR_62_5HZ] = UINT32_C(1000000000) * 10 / 625,
	[ADXL355_ODR_31_25HZ] = UINT32_C(1000000000) * 100 / 3125,
	[ADXL355_ODR_15_625HZ] = UINT32_C(1000000000) * 1000 / 15625,
	[ADXL355_ODR_7_813HZ] = UINT32_C(1000000000) * 10000 / 78130,
	[ADXL355_ODR_3_906HZ] = UINT32_C(1000000000) * 100000 / 390600,
};
#endif /* CONFIG_ADXL355_STREAM */

/**
 * @brief Get frame count from encoded buffer
 *
 * @param buffer Buffer pointer
 * @param channel Channel specification
 * @param frame_count Pointer to store frame count
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec channel,
					   uint16_t *frame_count)
{
#ifndef CONFIG_ADXL355_STREAM
	return -ENOTSUP;
#endif
	const struct adxl355_fifo_data *data = (const struct adxl355_fifo_data *)buffer;

	if (!data->is_fifo) {
		return -ENOTSUP;
	}
	switch (channel.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		*frame_count = data->fifo_samples;
		return 0;
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Convert raw accelerometer data to Q31 format
 *
 * @param out Output Q31 pointer
 * @param buff Raw data buffer pointer
 * @param range Measurement range
 */
static inline void adxl355_accel_convert_q31(q31_t *out, const uint8_t *buff,
					     const enum adxl355_range range)
{
	int32_t sensitivity;
	int32_t sample = (int32_t)((((buff[0] << 16) | (buff[1] << 8) | buff[2]) << 8) >> 12);

	switch (range) {
	case ADXL355_RANGE_2G:
		sensitivity = SENSOR_G / 256000;
		break;
	case ADXL355_RANGE_4G:
		sensitivity = SENSOR_G / 128000;
		break;
	case ADXL355_RANGE_8G:
		sensitivity = SENSOR_G / 64000;
		break;
	default:
		LOG_ERR("Invalid range setting");
		return;
	}
	*out = sample * sensitivity;
}

#ifdef CONFIG_ADXL355_STREAM
/**
 * @brief Decode stream data from buffer
 *
 * @param buffer Buffer pointer
 * @param chan_spec Channel specification
 * @param fit fit pointer
 * @param max_count Maximum number of samples to decode
 * @param data_out Output data pointer
 * @return int count of decoded samples, negative error code otherwise
 */
static int adxl355_decode_stream(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				 uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adxl355_fifo_data *enc_data = (const struct adxl355_fifo_data *)buffer;
	const uint8_t *buffer_end =
		buffer + enc_data->fifo_byte_count + sizeof(struct adxl355_fifo_data);
	int count = 0;
	uint8_t sample_num = 0;

	if ((uintptr_t)buffer_end <= *fit || chan_spec.chan_idx != 0) {
		return 0;
	}

	struct sensor_three_axis_data *data = (struct sensor_three_axis_data *)data_out;

	memset(data, 0, sizeof(struct sensor_three_axis_sample_data));
	data->header.base_timestamp_ns = enc_data->timestamp;
	data->header.reading_count = 1;
	data->shift = 11;

	buffer += sizeof(struct adxl355_fifo_data);
	uint8_t sample_set_size = enc_data->sample_set_size * 3;
	uint64_t period_ns = accel_period_ns[enc_data->accel_odr];

	/* Calculate which sample is decoded. */
	if ((uint8_t *)*fit >= buffer) {
		sample_num = ((uint8_t *)*fit - buffer) / sample_set_size;
	}
	uint8_t x_offset = 0;
	/* Determine which sample corresponds to X axis */
	for (uint8_t i = 0; i < 3; i++) {
		if (*(buffer + i * 3 + 2) & BIT(0)) {
			x_offset = i;
			break;
		}
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
			data->readings[count].timestamp_delta = sample_num * period_ns;
			adxl355_accel_convert_q31(&data->readings[count].x, buffer + (x_offset * 3),
						  enc_data->range);
			break;
		case SENSOR_CHAN_ACCEL_Y:
			data->readings[count].timestamp_delta = sample_num * period_ns;
			adxl355_accel_convert_q31(&data->readings[count].y,
						  buffer + (x_offset * 3) + 3, enc_data->range);
			break;
		case SENSOR_CHAN_ACCEL_Z:
			data->readings[count].timestamp_delta = sample_num * period_ns;
			adxl355_accel_convert_q31(&data->readings[count].z,
						  buffer + (x_offset * 3) + 6, enc_data->range);
			break;
		case SENSOR_CHAN_ACCEL_XYZ:
			data->readings[count].timestamp_delta = sample_num * period_ns;
			adxl355_accel_convert_q31(&data->readings[count].x, buffer + (x_offset * 3),
						  enc_data->range);
			adxl355_accel_convert_q31(&data->readings[count].y,
						  (buffer + (x_offset * 3) + 3), enc_data->range);
			adxl355_accel_convert_q31(&data->readings[count].z,
						  (buffer + (x_offset * 3) + 6), enc_data->range);
			break;
		default:
			return -ENOTSUP;
		}

		count++;
		sample_num++;
		buffer = sample_end;
		*fit = (uintptr_t)sample_end;
	}
	return count;
}
#endif /* CONFIG_ADXL355_STREAM */

/**
 * @brief Decode single sample data
 *
 * @param data Sample data pointer
 * @param chan_spec Channel specification
 * @param fit fit pointer
 * @param max_count Maximum number of samples to decode
 * @param data_out Output data pointer
 * @return int count of decoded samples, negative error code otherwise
 */
static int adxl355_decode_sample(const struct adxl355_sample *data,
				 struct sensor_chan_spec chan_spec, uint32_t *fit,
				 uint16_t max_count, void *data_out)
{
	struct sensor_three_axis_data *out = (struct sensor_three_axis_data *)data_out;
	const uint8_t *x_bytes = (const uint8_t *)&data->x;
	const uint8_t *y_bytes = (const uint8_t *)&data->y;
	const uint8_t *z_bytes = (const uint8_t *)&data->z;

	memset(out, 0, sizeof(struct sensor_three_axis_data));
	out->header.base_timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());
	out->header.reading_count = 1;
	out->shift = 11;

	if (*fit > 0) {
		return -ENOTSUP;
	}

	if (chan_spec.chan_type == SENSOR_CHAN_ACCEL_XYZ) {
		adxl355_accel_convert_q31(&out->readings[0].x, x_bytes, data->range);
		adxl355_accel_convert_q31(&out->readings[0].y, y_bytes, data->range);
		adxl355_accel_convert_q31(&out->readings[0].z, z_bytes, data->range);
	} else {
		return -ENOTSUP;
	}
	*fit = 1;

	return 1;
}

/**
 * @brief Decode data from buffer
 *
 * @param buffer Buffer pointer
 * @param channel Channel specification
 * @param fit fit pointer
 * @param max_count Maximum number of samples to decode
 * @param data_out Output data pointer
 * @return int count of decoded samples, negative error code otherwise
 */
static int adxl355_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec channel,
				  uint32_t *fit, uint16_t max_count, void *data_out)
{
#ifdef CONFIG_ADXL355_STREAM
	const struct adxl355_fifo_data *fifo = (const struct adxl355_fifo_data *)buffer;

	if (fifo->is_fifo) {
		return adxl355_decode_stream(buffer, channel, fit, max_count, data_out);
	}
#endif
	const struct adxl355_sample *sample = (const struct adxl355_sample *)buffer;

	return adxl355_decode_sample(sample, channel, fit, max_count, data_out);
}

/**
 * @brief Get size information for specified channel
 *
 * @param channel Channel specification
 * @param base_size Base size pointer
 * @param frame_size Frame size pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_get_size_info(struct sensor_chan_spec channel, size_t *base_size,
				 size_t *frame_size)
{
	__ASSERT_NO_MSG(base_size != NULL);
	__ASSERT_NO_MSG(frame_size != NULL);

	if (channel.chan_type >= SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	if (channel.chan_type == SENSOR_CHAN_ACCEL_XYZ) {
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	}
	return -ENOTSUP;
}

/**
 * @brief Check if buffer has trigger event
 *
 * @param buffer Buffer pointer
 * @param trigger Trigger type
 * @return true if trigger event is present, false otherwise
 * @return false otherwise
 */
static bool adxl355_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
#ifdef CONFIG_ADXL355_STREAM
	return false;
#endif
	const struct adxl355_fifo_data *fifo_data = (const struct adxl355_fifo_data *)buffer;

	switch (trigger) {
	case SENSOR_TRIG_FIFO_WATERMARK:
	case SENSOR_TRIG_FIFO_FULL:
		return FIELD_GET(ADXL355_STATUS_FIFO_FULL_MSK, fifo_data->status1);
	default:
		return false;
	}
}
SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adxl355_decoder_get_frame_count,
	.get_size_info = adxl355_get_size_info,
	.decode = adxl355_decoder_decode,
	.has_trigger = adxl355_decoder_has_trigger,
};

/**
 * @brief Get sensor decoder
 *
 * @param dev Device pointer
 * @param decoder Decoder API pointer
 * @return int 0 on success, negative error code otherwise
 */
int adxl355_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
