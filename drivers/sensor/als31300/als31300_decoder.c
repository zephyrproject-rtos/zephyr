/*
 * Copyright (c) 2025 Croxel
 * SPDX-License-Identifier: Apache-2.0
 */

#include "als31300.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/dsp/types.h>

LOG_MODULE_DECLARE(als31300, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Encode channel flags for the given sensor channel
 */
static uint8_t als31300_encode_channel(enum sensor_channel chan)
{
	uint8_t encode_bmask = 0;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		encode_bmask |= BIT(0);
		break;
	case SENSOR_CHAN_MAGN_Y:
		encode_bmask |= BIT(1);
		break;
	case SENSOR_CHAN_MAGN_Z:
		encode_bmask |= BIT(2);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		encode_bmask |= BIT(0) | BIT(1) | BIT(2);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		encode_bmask |= BIT(3);
		break;
	case SENSOR_CHAN_ALL:
		encode_bmask |= BIT(0) | BIT(1) | BIT(2) | BIT(3);
		break;
	default:
		break;
	}

	return encode_bmask;
}

/**
 * @brief Convert raw magnetic field value to Q31 format
 * @param raw_value Signed 12-bit magnetic field value
 * @param q31_out Pointer to store Q31 value
 */
static void als31300_convert_raw_to_q31_magn(int16_t raw_value, q31_t *q31_out)
{
	/* Convert to microgauss using integer arithmetic */
	int32_t microgauss = als31300_convert_to_gauss(raw_value);

	/* Convert to Q31 format: Q31 = (value * 2^shift) / 1000000
	 * For magnetic field, we use shift=16, so the full scale is ±2^(31-16) = ±32768 gauss
	 * This gives us good resolution for the ±500G range of the ALS31300
	 * microgauss * 2^16 / 1000000 = microgauss * 65536 / 1000000
	 */
	*q31_out = (q31_t)(((int64_t)microgauss << ALS31300_MAGN_SHIFT) / 1000000);
}

/**
 * @brief Convert raw temperature value to Q31 format
 * @param raw_temp 12-bit raw temperature value
 * @param q31_out Pointer to store Q31 value
 */
static void als31300_convert_temp_to_q31(uint16_t raw_temp, q31_t *q31_out)
{
	/* Convert to microcelsius using integer arithmetic */
	int32_t microcelsius = als31300_convert_temperature(raw_temp);

	/* Convert to Q31 format: Q31 = (value * 2^shift) / 1000000
	 * For temperature, we use shift=16, so the full scale is ±2^(31-16) = ±32768°C
	 * This gives us good resolution for typical temperature ranges (-40°C to +125°C)
	 * microcelsius * 2^16 / 1000000 = microcelsius * 65536 / 1000000
	 */
	*q31_out = (q31_t)(((int64_t)microcelsius << ALS31300_TEMP_SHIFT) / 1000000);
}

/**
 * @brief Get frame count for decoder
 */
static int als31300_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	const struct als31300_encoded_data *edata = (const struct als31300_encoded_data *)buffer;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	uint8_t channel_request = als31300_encode_channel(chan_spec.chan_type);

	/* Filter unknown channels and having no data */
	if ((edata->header.channels & channel_request) != channel_request) {
		return -ENODATA;
	}

	*frame_count = 1;
	return 0;
}

/**
 * @brief Get size info for decoder
 */
static int als31300_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					  size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	case SENSOR_CHAN_AMBIENT_TEMP:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Decode function for RTIO
 */
static int als31300_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct als31300_encoded_data *edata = (const struct als31300_encoded_data *)buffer;

	if (*fit != 0) {
		return 0;
	}

	/* Parse raw payload data using common helper */
	struct als31300_readings readings;

	als31300_parse_registers(edata->payload, &readings);

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ: {
		struct sensor_three_axis_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = ALS31300_MAGN_SHIFT;

		/* Convert raw readings to Q31 format */
		als31300_convert_raw_to_q31_magn(readings.x, &out->readings[0].x);
		als31300_convert_raw_to_q31_magn(readings.y, &out->readings[0].y);
		als31300_convert_raw_to_q31_magn(readings.z, &out->readings[0].z);
		*fit = 1;
		return 1;
	}
	case SENSOR_CHAN_AMBIENT_TEMP: {
		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = ALS31300_TEMP_SHIFT;

		als31300_convert_temp_to_q31(readings.temp, &out->readings[0].temperature);
		*fit = 1;
		return 1;
	}
	default:
		return -ENOTSUP;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = als31300_decoder_get_frame_count,
	.get_size_info = als31300_decoder_get_size_info,
	.decode = als31300_decoder_decode,
};

int als31300_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
