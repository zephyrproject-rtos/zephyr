/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adxl38x.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ADXL38X);

/* Q31 shift for acceleration: value(m/s^2) = q31 / 2^(31 - shift). The per-sample
 * scale (adxl38x_accel_convert_q31) bakes in this shift; 2^20 bounds ~2048 m/s^2.
 */
#define ADXL38X_Q31_SHIFT 11

/* Die temperature is encoded as q31 with shift 8: value_C = q31 / 2^(31-8). */
#define ADXL38X_TEMP_Q31_SHIFT 8
#define ADXL38X_TEMP_Q31_SCALE (1 << (31 - ADXL38X_TEMP_Q31_SHIFT))

/**
 * @brief Look up sensitivity for a chip variant and range
 *
 * @param chip_type Chip variant
 * @param range Measurement range setting
 * @return int32_t Sensitivity in LSB/g
 */
static int32_t adxl38x_sensitivity_lsb(enum adxl38x_chip_type chip_type, uint8_t range)
{
	if (chip_type == ADXL38X_CHIP_ADXL382) {
		switch (range) {
		case ADXL382_DT_RANGE_15G:
			return ADXL382_SENSITIVITY_15G;
		case ADXL382_DT_RANGE_30G:
			return ADXL382_SENSITIVITY_30G;
		case ADXL382_DT_RANGE_60G:
			return ADXL382_SENSITIVITY_60G;
		default:
			return ADXL382_SENSITIVITY_15G;
		}
	}

	switch (range) {
	case ADXL380_DT_RANGE_4G:
		return ADXL380_SENSITIVITY_4G;
	case ADXL380_DT_RANGE_8G:
		return ADXL380_SENSITIVITY_8G;
	case ADXL380_DT_RANGE_16G:
		return ADXL380_SENSITIVITY_16G;
	default:
		return ADXL380_SENSITIVITY_4G;
	}
}

/**
 * @brief Convert one big-endian 16-bit sample word to Q31 (m/s^2)
 *
 * qscale folds the Q31 shift and the g->m/s^2 conversion (SENSOR_G is in
 * micro-m/s^2, hence the 1e6 divisor) into one per-LSB scale, matching
 * channel_get().
 *
 * @param out Output Q31 pointer
 * @param buff Raw sample bytes
 * @param chip_type Chip variant
 * @param range Measurement range
 */
static inline void adxl38x_accel_convert_q31(q31_t *out, const uint8_t *buff,
					     enum adxl38x_chip_type chip_type, uint8_t range)
{
	int16_t sample = (int16_t)((buff[0] << 8) | buff[1]);
	int32_t lsb = adxl38x_sensitivity_lsb(chip_type, range);
	int32_t qscale = (int32_t)(((int64_t)SENSOR_G << (31 - ADXL38X_Q31_SHIFT)) /
				   ((int64_t)lsb * 1000000));

	*out = sample * qscale;
}

/**
 * @brief Convert a raw die-temperature code to Q31 (degrees Celsius)
 *
 * @param out Output Q31 pointer (shift = ADXL38X_TEMP_Q31_SHIFT)
 * @param raw Sign-extended raw temperature code
 */
static inline void adxl38x_temp_convert_q31(q31_t *out, int16_t raw)
{
	/* T(degC) = 25 + (raw - 550) / 10.2 */
	int64_t q = (int64_t)ADXL38X_TEMP_REF_DEG_C * ADXL38X_TEMP_Q31_SCALE +
		    ((int64_t)(raw - ADXL38X_TEMP_25C_INTERCEPT) * ADXL38X_TEMP_Q31_SCALE *
		     ADXL38X_TEMP_SENS_SCALE) /
			    ADXL38X_TEMP_SENS_X10;

	*out = (q31_t)q;
}

/**
 * @brief Get frame count from encoded buffer
 *
 * @param buffer Buffer pointer
 * @param channel Channel specification
 * @param frame_count Output frame count
 * @return int 0 on success, negative errno otherwise
 */
static int adxl38x_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec channel,
					   uint16_t *frame_count)
{
	const struct adxl38x_fifo_data *data = (const struct adxl38x_fifo_data *)buffer;

	/* Only instance 0 exists; reject higher indices so chan_idx iteration ends. */
	if (channel.chan_idx != 0) {
		return -ENOTSUP;
	}

	if (!data->is_fifo) {
		*frame_count = 1;
		return 0;
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

#ifdef CONFIG_ADXL38X_STREAM
/**
 * @brief Decode FIFO stream data from buffer
 *
 * @param buffer Buffer pointer
 * @param chan_spec Channel specification
 * @param fit Frame iterator, updated in place
 * @param max_count Maximum number of samples to decode
 * @param data_out Output data pointer
 * @return int Count of decoded samples, negative errno otherwise
 */
static int adxl38x_decode_stream(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				 uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adxl38x_fifo_data *enc = (const struct adxl38x_fifo_data *)buffer;
	const uint8_t *samples = buffer + sizeof(struct adxl38x_fifo_data);
	uint8_t sample_set_size = enc->sample_set_size;
	uint32_t byte_count = enc->fifo_byte_count;
	struct sensor_three_axis_data *data = (struct sensor_three_axis_data *)data_out;
	uint64_t period_ns;
	uint32_t odr;
	int count = 0;

	/* *fit is a byte offset into the payload (0 on the first call), not a pointer:
	 * the decoder API fixes it at 32 bits, so a pointer would truncate on 64-bit
	 * hosts. An offset from the uint16_t fifo_byte_count always fits.
	 */
	if (chan_spec.chan_idx != 0 || sample_set_size == 0U || *fit >= byte_count) {
		return 0;
	}

	memset(data, 0, sizeof(struct sensor_three_axis_data));
	data->header.base_timestamp_ns = enc->timestamp;
	data->shift = ADXL38X_Q31_SHIFT;

	odr = enc->accel_odr;
	period_ns = (odr != 0) ? (NSEC_PER_SEC / odr) : 0;

	while (count < max_count && (*fit + sample_set_size) <= byte_count) {
		const uint8_t *sample = samples + *fit;
		uint32_t sample_num = *fit / sample_set_size;

		data->readings[count].timestamp_delta = sample_num * period_ns;

		switch (chan_spec.chan_type) {
		case SENSOR_CHAN_ACCEL_X:
			adxl38x_accel_convert_q31(&data->readings[count].x, sample, enc->chip_type,
						  enc->range);
			break;
		case SENSOR_CHAN_ACCEL_Y:
			adxl38x_accel_convert_q31(&data->readings[count].y,
						  sample + ADXL38X_SAMPLE_SIZE, enc->chip_type,
						  enc->range);
			break;
		case SENSOR_CHAN_ACCEL_Z:
			adxl38x_accel_convert_q31(&data->readings[count].z,
						  sample + (2 * ADXL38X_SAMPLE_SIZE),
						  enc->chip_type, enc->range);
			break;
		case SENSOR_CHAN_ACCEL_XYZ:
			adxl38x_accel_convert_q31(&data->readings[count].x, sample, enc->chip_type,
						  enc->range);
			adxl38x_accel_convert_q31(&data->readings[count].y,
						  sample + ADXL38X_SAMPLE_SIZE, enc->chip_type,
						  enc->range);
			adxl38x_accel_convert_q31(&data->readings[count].z,
						  sample + (2 * ADXL38X_SAMPLE_SIZE),
						  enc->chip_type, enc->range);
			break;
		default:
			return -ENOTSUP;
		}

		count++;
		*fit += sample_set_size;
	}

	data->header.reading_count = count;

	return count;
}
#endif /* CONFIG_ADXL38X_STREAM */

/**
 * @brief Decode a single (non-FIFO) sample
 *
 * @param data Sample data pointer
 * @param chan_spec Channel specification
 * @param fit Frame iterator, updated in place
 * @param max_count Maximum number of samples to decode
 * @param data_out Output data pointer
 * @return int Count of decoded samples, negative errno otherwise
 */
static int adxl38x_decode_sample(const struct adxl38x_sample *data,
				 struct sensor_chan_spec chan_spec, uint32_t *fit,
				 uint16_t max_count, void *data_out)
{
	if (*fit > 0) {
		return 0;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_XYZ: {
		struct sensor_three_axis_data *out = (struct sensor_three_axis_data *)data_out;
		uint8_t xb[2] = {(data->x >> 8) & 0xFF, data->x & 0xFF};
		uint8_t yb[2] = {(data->y >> 8) & 0xFF, data->y & 0xFF};
		uint8_t zb[2] = {(data->z >> 8) & 0xFF, data->z & 0xFF};

		memset(out, 0, sizeof(struct sensor_three_axis_data));
		out->header.base_timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());
		out->header.reading_count = 1;
		out->shift = ADXL38X_Q31_SHIFT;

		adxl38x_accel_convert_q31(&out->readings[0].x, xb, data->chip_type, data->range);
		adxl38x_accel_convert_q31(&out->readings[0].y, yb, data->chip_type, data->range);
		adxl38x_accel_convert_q31(&out->readings[0].z, zb, data->chip_type, data->range);
		break;
	}
	case SENSOR_CHAN_DIE_TEMP: {
		struct sensor_q31_data *out = (struct sensor_q31_data *)data_out;

		memset(out, 0, sizeof(struct sensor_q31_data));
		out->header.base_timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());
		out->header.reading_count = 1;
		out->shift = ADXL38X_TEMP_Q31_SHIFT;

		adxl38x_temp_convert_q31(&out->readings[0].temperature, data->temp);
		break;
	}
	default:
		return -ENOTSUP;
	}

	*fit = 1;

	return 1;
}

/**
 * @brief Decode a one-shot or FIFO buffer
 *
 * @param buffer Buffer pointer
 * @param channel Channel specification
 * @param fit Frame iterator, updated in place
 * @param max_count Maximum number of samples to decode
 * @param data_out Output data pointer
 * @return int Count of decoded samples, negative errno otherwise
 */
static int adxl38x_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec channel,
				  uint32_t *fit, uint16_t max_count, void *data_out)
{
#ifdef CONFIG_ADXL38X_STREAM
	const struct adxl38x_fifo_data *fifo = (const struct adxl38x_fifo_data *)buffer;

	if (fifo->is_fifo) {
		return adxl38x_decode_stream(buffer, channel, fit, max_count, data_out);
	}
#endif
	const struct adxl38x_sample *sample = (const struct adxl38x_sample *)buffer;

	return adxl38x_decode_sample(sample, channel, fit, max_count, data_out);
}

/**
 * @brief Get size information for specified channel
 *
 * @param channel Channel specification
 * @param base_size Base size pointer
 * @param frame_size Frame size pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl38x_get_size_info(struct sensor_chan_spec channel, size_t *base_size,
				 size_t *frame_size)
{
	__ASSERT_NO_MSG(base_size != NULL);
	__ASSERT_NO_MSG(frame_size != NULL);

	switch (channel.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
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

/**
 * @brief Check if buffer has trigger event
 *
 * @param buffer Buffer pointer
 * @param trigger Trigger type
 * @return true if trigger event is present, false otherwise
 */
static bool adxl38x_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct adxl38x_fifo_data *fifo_data = (const struct adxl38x_fifo_data *)buffer;

	if (!fifo_data->is_fifo) {
		return false;
	}

	switch (trigger) {
	case SENSOR_TRIG_FIFO_WATERMARK:
		return FIELD_GET(ADXL38X_STATUS0_FIFO_WATERMARK_MSK, fifo_data->status0);
	case SENSOR_TRIG_FIFO_FULL:
		return FIELD_GET(ADXL38X_STATUS0_FIFO_FULL_MSK, fifo_data->status0);
	default:
		return false;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adxl38x_decoder_get_frame_count,
	.get_size_info = adxl38x_get_size_info,
	.decode = adxl38x_decoder_decode,
	.has_trigger = adxl38x_decoder_has_trigger,
};

/**
 * @brief Get sensor decoder
 *
 * @param dev Device pointer
 * @param decoder Decoder API pointer
 * @return int 0 on success, negative error code otherwise
 */
int adxl38x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
