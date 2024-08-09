/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adxl34x.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "adxl34x_decoder.h"
#include "adxl34x_convert.h"

LOG_MODULE_DECLARE(adxl34x, CONFIG_SENSOR_LOG_LEVEL);

#if defined(CONFIG_ADXL34X_DATA_TYPE_Q31)

/**
 * Convert the raw sensor data from the registery to the q31 format
 *
 * @param[in] value The raw value to convert
 * @param[in] range_scale The scale of range
 * @param[in] shift The shift value to use in the q31 number
 * @param[out] out Pointer to the converted q31 number
 */
static void adxl34x_convert_raw_to_q31(const uint8_t *value, uint16_t range_scale, int8_t shift,
				       q31_t *out)
{
	const int16_t raw_value = sys_get_le16(value);
	const int32_t ug = (int32_t)raw_value * range_scale * 100;
	struct sensor_value ms2;

	sensor_ug_to_ms2(ug, &ms2);
	const int64_t q31_temp = ((int64_t)ms2.val1 * INT64_C(1000000) + ms2.val2) *
				 ((int64_t)INT32_MAX + 1) / ((1 << shift) * INT64_C(1000000));
	*out = CLAMP(q31_temp, INT32_MIN, INT32_MAX);
}

#elif defined(CONFIG_ADXL34X_DATA_TYPE_SENSOR_VALUE)

/**
 * Convert the raw sensor data from the registery to the sensor value format
 *
 * @param[in] value The raw value to convert
 * @param[in] range_scale The scale of range
 * @param[out] out Pointer to the converted sensor value
 */
static void adxl34x_convert_raw_to_sensor_value(const uint8_t *value, uint16_t range_scale,
						struct sensor_value *out)
{
	const int16_t raw_value = sys_get_le16(value);
	const int32_t ug = (int32_t)raw_value * range_scale * 100;

	sensor_ug_to_ms2(ug, out);
}

#elif defined(CONFIG_ADXL34X_DATA_TYPE_DOUBLE)

/**
 * Convert the raw sensor data from the registery to the double format
 *
 * @param[in] value The raw value to convert
 * @param[in] range_scale The scale of range
 * @param[out] out Pointer to the converted double
 */
static void adxl34x_convert_raw_to_double(const uint8_t *value, uint16_t range_scale, double *out)
{
	const int16_t raw_value = sys_get_le16(value);

	*out = (double)raw_value * range_scale / 10000 * SENSOR_G / 1000000LL;
}

#endif /* CONFIG_ADXL34X_DATA_TYPE_Q31 */

/**
 * Get the number of x-y-z samples when reading one packet when using the decode
 * function. One frame equals one tuple of a x, y and z value.
 *
 * @param[in] buffer The buffer provided on the @ref rtio context.
 * @param[in] channel The channel to get the count for
 * @param[out] frame_count The number of frames on the buffer (at least 1)
 * @return 0 if successful, negative errno code if failure.
 * @see sensor_get_decoder(), sensor_decode(), get_frame_count()
 */
static int adxl34x_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec channel,
					   uint16_t *frame_count)
{
	const struct adxl34x_encoded_data *edata = (const struct adxl34x_encoded_data *)buffer;
	const struct adxl34x_decoder_header *header = &edata->header;

	if (channel.chan_idx != 0) {
		return -ENOTSUP;
	}
	if (header->entries == 0) {
		return -ENOTSUP;
	}

	switch (channel.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		*frame_count = edata->header.entries;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

/**
 * Get the size required to decode a given channel
 *
 * @param[in] channel The channel to query
 * @param[out] base_size The size of decoding the first frame
 * @param[out] frame_size The additional size of every additional frame
 * @return 0 if successful, negative errno code if failure.
 * @see sensor_get_decoder(), sensor_decode(), get_size_info()
 */
static int adxl34x_decoder_get_size_info(struct sensor_chan_spec channel, size_t *base_size,
					 size_t *frame_size)
{
	switch (channel.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
#if defined(CONFIG_ADXL34X_DATA_TYPE_Q31)
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
#elif defined(CONFIG_ADXL34X_DATA_TYPE_SENSOR_VALUE)
		*base_size = sizeof(struct sensor_value[3]);
		*frame_size = sizeof(struct sensor_value[3]);
#elif defined(CONFIG_ADXL34X_DATA_TYPE_DOUBLE)
		*base_size = sizeof(struct adxl343_sensor_data);
		*frame_size = sizeof(struct adxl343_sample_value);
#endif
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

/**
 * Decode up to @p max_count samples from the buffer
 *
 * @param[in] buffer The buffer provided on the @ref rtio context
 * @param[in] channel The channel to decode
 * @param[out] fit The current frame iterator
 * @param[in] max_count The maximum number of channels to decode.
 * @param[out] data_out The decoded data
 * @return 0 if successful, negative errno code if failure.
 * @see sensor_get_decoder(), sensor_decode(), get_frame_count(), get_size_info()
 */
static int adxl34x_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec channel,
				  uint32_t *fit, uint16_t max_count, void *data_out)
{
	ARG_UNUSED(fit);
	ARG_UNUSED(max_count);
	const struct adxl34x_encoded_data *edata = (const struct adxl34x_encoded_data *)buffer;
	const struct adxl34x_decoder_header *header = &edata->header;
	int nr_of_decoded_samples = 0;

	if (channel.chan_idx != 0) {
		return -ENOTSUP;
	}
	if (header->entries == 0) {
		return -ENOTSUP;
	}
	if (channel.chan_type != SENSOR_CHAN_ACCEL_X && channel.chan_type != SENSOR_CHAN_ACCEL_Y &&
	    channel.chan_type != SENSOR_CHAN_ACCEL_Z &&
	    channel.chan_type != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	const uint16_t range_scale = adxl34x_range_conv[header->range];
	uint8_t offset = 0;

#if defined(CONFIG_ADXL34X_DATA_TYPE_Q31)
	const uint8_t shift = adxl34x_shift_conv[header->range];
	struct sensor_three_axis_data *out = (struct sensor_three_axis_data *)data_out;

	out->header.base_timestamp_ns = edata->header.timestamp;
	out->header.reading_count = header->entries;
	out->shift = shift;

#elif defined(CONFIG_ADXL34X_DATA_TYPE_SENSOR_VALUE)
	struct sensor_value *out = (struct sensor_value *)data_out;

#elif defined(CONFIG_ADXL34X_DATA_TYPE_DOUBLE)
	struct adxl343_sensor_data *out = (struct adxl343_sensor_data *)data_out;

	out->header.base_timestamp_ns = edata->header.timestamp;
	out->header.reading_count = header->entries;
#endif

	for (int i = 0; i < header->entries; i++) {
#if defined(CONFIG_ADXL34X_DATA_TYPE_Q31)
		adxl34x_convert_raw_to_q31(edata->fifo_data + offset, range_scale, shift,
					   &out->readings[i].x);
		adxl34x_convert_raw_to_q31(edata->fifo_data + offset + 2, range_scale, shift,
					   &out->readings[i].y);
		adxl34x_convert_raw_to_q31(edata->fifo_data + offset + 4, range_scale, shift,
					   &out->readings[i].z);

#elif defined(CONFIG_ADXL34X_DATA_TYPE_SENSOR_VALUE)
		adxl34x_convert_raw_to_sensor_value(edata->fifo_data + offset, range_scale,
						    &out[i * 3]);
		adxl34x_convert_raw_to_sensor_value(edata->fifo_data + offset + 2, range_scale,
						    &out[i * 3 + 1]);
		adxl34x_convert_raw_to_sensor_value(edata->fifo_data + offset + 4, range_scale,
						    &out[i * 3 + 2]);

#elif defined(CONFIG_ADXL34X_DATA_TYPE_DOUBLE)
		adxl34x_convert_raw_to_double(edata->fifo_data + offset, range_scale,
					      &out->readings[i].x);
		adxl34x_convert_raw_to_double(edata->fifo_data + offset + 2, range_scale,
					      &out->readings[i].y);
		adxl34x_convert_raw_to_double(edata->fifo_data + offset + 4, range_scale,
					      &out->readings[i].z);
#endif
		offset += sizeof(edata->fifo_data);
		nr_of_decoded_samples++;
	}
	return nr_of_decoded_samples;
}

/**
 * Check if the given trigger type is present
 *
 * @param[in] buffer The buffer provided on the @ref rtio context
 * @param[in] trigger The trigger type in question
 * @return 0 if successful, negative errno code if failure.
 */
static bool adxl34x_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct adxl34x_encoded_data *edata = (const struct adxl34x_encoded_data *)buffer;
	const struct adxl34x_decoder_header *header = &edata->header;
	bool has_trigger = false;

	switch (trigger) {
	case SENSOR_TRIG_DATA_READY:     /* New data is ready */
	case SENSOR_TRIG_FIFO_WATERMARK: /* The FIFO watermark has been reached */
	case SENSOR_TRIG_FIFO_FULL:      /* The FIFO becomes full */
		has_trigger = header->trigger.data_ready || header->trigger.watermark ||
			      header->trigger.overrun;
		break;
	case SENSOR_TRIG_TAP: /* A single tap is detected. */
		has_trigger = header->trigger.single_tap;
		break;
	case SENSOR_TRIG_DOUBLE_TAP: /* A double tap is detected. */
		has_trigger = header->trigger.double_tap;
		break;
	case SENSOR_TRIG_FREEFALL: /* A free fall is detected. */
		has_trigger = header->trigger.free_fall;
		break;
	case SENSOR_TRIG_MOTION: /* Motion is detected. */
		has_trigger = header->trigger.activity;
		break;
	case SENSOR_TRIG_STATIONARY: /* No motion has been detected for a while. */
		has_trigger = header->trigger.inactivity;
		break;
	case SENSOR_TRIG_TIMER:
	case SENSOR_TRIG_DELTA:
	case SENSOR_TRIG_NEAR_FAR:
	case SENSOR_TRIG_THRESHOLD:
	case SENSOR_TRIG_COMMON_COUNT:
	case SENSOR_TRIG_MAX:
	default:
		return false;
	}
	return has_trigger;
}

/**
 * @brief The sensor driver decoder API callbacks
 */
SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adxl34x_decoder_get_frame_count,
	.get_size_info = adxl34x_decoder_get_size_info,
	.decode = adxl34x_decoder_decode,
	.has_trigger = adxl34x_decoder_has_trigger,
};

/**
 * Callback API to get the decoder associated with the given device
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] decoder Pointer to the decoder which will be set upon success
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
