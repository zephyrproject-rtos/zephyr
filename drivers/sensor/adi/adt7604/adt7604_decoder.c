/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "adt7604.h"

LOG_MODULE_DECLARE(adt7604, CONFIG_SENSOR_LOG_LEVEL);

/*
 * NOTE: Temperature values are returned in the configured unit (°C or °F).
 *       Applications should check temp_unit_celsius flag or use sensor attributes
 *       to determine the actual unit if needed.
 */
#define ADT7604_Q31_SHIFT             10
#define ADT7604_Q31_LEAK_SHIFT        31
#define ADT7604_Q31_SCALE_MOHM_TO_OHM 1000 /* Convert mΩ to Ω */
#define ADT7604_Q31_SCALE_SHIFT       21   /* (31 - 10) for 1/1024 resolution */

/**
 * @brief Check if a sensor type matches the requested channel specification
 *
 * @param sensor_type ADT7604 sensor type
 * @param chan_spec Requested channel specification
 * @return true if the sensor type matches the channel type, false otherwise
 */
static bool channel_matches_spec(uint8_t sensor_type, struct sensor_chan_spec chan_spec)
{
	enum adt7604_measurement_type meas_type = adt7604_get_measurement_type(sensor_type);

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		return (meas_type == ADT7604_MEASUREMENT_TEMPERATURE);
	case SENSOR_CHAN_RESISTANCE:
		return (meas_type == ADT7604_MEASUREMENT_RESISTANCE);
	default:
		return false;
	}
}

/**
 * @brief Convert raw temperature value to Q31 format
 *
 * @param raw Raw temperature reading from ADT7604 (1/1024 °C or °F per LSB)
 * @return Q31 formatted temperature value
 */
static q31_t temp_raw_to_q31(int32_t raw)
{
	/* Temperature: 1/1024 °C or °F per LSB, scale to Q31.10 */
	return (q31_t)((int64_t)raw * (1 << ADT7604_Q31_SCALE_SHIFT) / 1024);
}

/**
 * @brief Convert raw resistance value to Q31 format
 *
 * @param sensor_type Type of resistance sensor
 * @param raw Raw resistance reading from ADT7604
 * @param shift Pointer to store the Q31 shift value
 * @return Q31 formatted resistance value
 */
static q31_t resistance_raw_to_q31(uint8_t sensor_type, int32_t raw, int8_t *shift)
{
	if (sensor_type == ADT7604_SENSOR_TYPE_COPPER_TRACE) {
		/* Copper trace: 1/1024 mΩ per LSB, convert to Ω */
		*shift = ADT7604_Q31_SHIFT;
		return (q31_t)((int64_t)raw * (1 << ADT7604_Q31_SCALE_SHIFT) / 1024 /
			       ADT7604_Q31_SCALE_MOHM_TO_OHM);
	}

	/* Leak detector: 1/1024 Ω per LSB */
	*shift = ADT7604_Q31_LEAK_SHIFT;
	return (q31_t)((int64_t)raw / 1024);
}

static int adt7604_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
					   uint16_t *frame_count)
{
	const struct adt7604_encoded_data *edata = (const struct adt7604_encoded_data *)buffer;
	int match_count = 0;

	if (chan_spec.chan_type != SENSOR_CHAN_AMBIENT_TEMP &&
	    chan_spec.chan_type != SENSOR_CHAN_RESISTANCE) {
		return -ENOTSUP;
	}

	for (int i = 0; i < edata->num_channels; i++) {
		if (channel_matches_spec(edata->readings[i].sensor_type, chan_spec)) {
			if (match_count == chan_spec.chan_idx) {
				*frame_count = 1;
				return 0;
			}
			match_count++;
		}
	}
	return -ENOTSUP;
}

static int adt7604_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
				 size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_RESISTANCE:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int adt7604_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				  uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct adt7604_encoded_data *edata = (const struct adt7604_encoded_data *)buffer;

	if (*fit != 0) {
		return 0;
	}

	struct sensor_q31_data *out = (struct sensor_q31_data *)data_out;
	int match_count = 0;

	for (int i = 0; i < edata->num_channels; i++) {
		uint8_t stype = edata->readings[i].sensor_type;
		enum adt7604_measurement_type meas_type = adt7604_get_measurement_type(stype);

		if (!channel_matches_spec(stype, chan_spec)) {
			continue;
		}

		if (match_count == chan_spec.chan_idx) {
			q31_t q31_value;

			/* Convert raw value to Q31 format based on measurement type */
			if (meas_type == ADT7604_MEASUREMENT_TEMPERATURE) {
				q31_value = temp_raw_to_q31(edata->readings[i].raw);
				out->shift = ADT7604_Q31_SHIFT;
			} else if (meas_type == ADT7604_MEASUREMENT_RESISTANCE) {
				q31_value = resistance_raw_to_q31(stype, edata->readings[i].raw,
								  &out->shift);
			} else {
				return -ENOTSUP;
			}

			out->header.base_timestamp_ns = edata->timestamp_ns;
			out->header.reading_count = 1;
			out->readings[0].timestamp_delta = 0;

			/* Use appropriate union field for semantic clarity */
			if (meas_type == ADT7604_MEASUREMENT_TEMPERATURE) {
				out->readings[0].temperature = q31_value;
			} else {
				out->readings[0].resistance = q31_value;
			}

			*fit = 1;
			return 1;
		}
		match_count++;
	}
	return -ENODATA;
}

static bool adt7604_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(trigger);
	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = adt7604_decoder_get_frame_count,
	.get_size_info = adt7604_get_size_info,
	.decode = adt7604_decoder_decode,
	.has_trigger = adt7604_decoder_has_trigger,
};

int adt7604_get_decoder(const struct device *sensor, const struct sensor_decoder_api **api)
{
	ARG_UNUSED(sensor);

	*api = &SENSOR_DECODER_NAME();

	return 0;
}
