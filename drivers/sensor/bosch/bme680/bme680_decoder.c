/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>

#include "bme680.h"

/* Get frame count for specified channel from encoded data */
static int bme680_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec chan,
					  uint16_t *frame_count)
{
	const struct bme680_encoded_data *edata =
		(const struct bme680_encoded_data *)buffer;

	if (chan.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (chan.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		*frame_count = edata->has_temp ? 1 : 0;
		return 0;
	case SENSOR_CHAN_PRESS:
		*frame_count = edata->has_press ? 1 : 0;
		return 0;
	case SENSOR_CHAN_HUMIDITY:
		*frame_count = edata->has_humidity ? 1 : 0;
		return 0;
	case SENSOR_CHAN_GAS_RES:
		*frame_count = edata->has_gas ? 1 : 0;
		return 0;
	case SENSOR_CHAN_ALL:
		*frame_count = 1;
		return 0;
	default:
		return -ENOTSUP;
	}
}

/* Get size information for decoded channel data */
static int bme680_decoder_get_size_info(struct sensor_chan_spec chan,
					size_t *base_size,
					size_t *frame_size)
{
	switch (chan.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_PRESS:
	case SENSOR_CHAN_HUMIDITY:
	case SENSOR_CHAN_GAS_RES:
		*base_size = sizeof(struct sensor_q31_sample_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

/* Decode sensor data to Q31 format */
static int bme680_decoder_decode(const uint8_t *buffer,
				 struct sensor_chan_spec chan,
				 uint32_t *fit,
				 uint16_t max_count,
				 void *data_out)
{
	const struct bme680_encoded_data *edata =
		(const struct bme680_encoded_data *)buffer;
	struct sensor_q31_data *out = data_out;

	ARG_UNUSED(max_count);

	if (*fit != 0) {
		return 0;
	}

	out->header.base_timestamp_ns = edata->header.timestamp;
	out->header.reading_count = 1;
	out->readings[0].timestamp_delta = 0;

	switch (chan.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		if (edata->has_temp) {
			/* BME680 temperature is in centi-degrees (x100), convert to Q31 */
			out->readings[0].temperature =
				BME680_Q31_CONV(edata->reading.comp_temp,
						BME680_TEMP_Q31_MULT, 100);
			out->shift = BME680_TEMP_SHIFT;
		} else {
			return -ENODATA;
		}
		break;
	case SENSOR_CHAN_PRESS:
		if (edata->has_press) {
			/* BME680 pressure is in Pa, convert to kPa in Q31 */
			out->readings[0].pressure =
				BME680_Q31_CONV(edata->reading.comp_press,
						BME680_PRESS_Q31_MULT, 1000);
			out->shift = BME680_PRESS_SHIFT;
		} else {
			return -ENODATA;
		}
		break;
	case SENSOR_CHAN_HUMIDITY:
		if (edata->has_humidity) {
			/* BME680 humidity is in milli-% (x1000), convert to Q31 */
			out->readings[0].humidity =
				BME680_Q31_CONV(edata->reading.comp_humidity,
						BME680_HUM_Q31_MULT, 1000);
			out->shift = BME680_HUM_SHIFT;
		} else {
			return -ENODATA;
		}
		break;
	case SENSOR_CHAN_GAS_RES:
		if (edata->has_gas) {
			/* Gas resistance in ohms, output as integer */
			out->readings[0].resistance = edata->reading.comp_gas;
			out->shift = BME680_GAS_SHIFT;
		} else {
			return -ENODATA;
		}
		break;
	default:
		return -ENOTSUP;
	}

	*fit = 1;
	return 1;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = bme680_decoder_get_frame_count,
	.get_size_info = bme680_decoder_get_size_info,
	.decode = bme680_decoder_decode,
};

/* Get decoder API for device */
int bme680_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
