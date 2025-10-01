/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/dt-bindings/sensor/rm3100.h>
#include "rm3100.h"

uint8_t rm3100_encode_channel(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		return BIT(0);
	case SENSOR_CHAN_MAGN_Y:
		return BIT(1);
	case SENSOR_CHAN_MAGN_Z:
		return BIT(2);
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_MAGN_XYZ:
		return BIT(0) | BIT(1) | BIT(2);
	default:
		return 0;
	}
}

int rm3100_encode(const struct device *dev,
		  const struct sensor_chan_spec *const channels,
		  size_t num_channels,
		  uint8_t *buf)
{
	const struct rm3100_data *data = dev->data;
	struct rm3100_encoded_data *edata = (struct rm3100_encoded_data *)buf;
	uint64_t cycles;
	int err;

	edata->header.channels = 0;

	if (data->settings.odr == RM3100_DT_ODR_600) {
		edata->header.cycle_count = RM3100_CYCLE_COUNT_HIGH_ODR;
	} else {
		edata->header.cycle_count = RM3100_CYCLE_COUNT_DEFAULT;
	}

	for (size_t i = 0; i < num_channels; i++) {
		edata->header.channels |= rm3100_encode_channel(channels[i].chan_type);
	}

	err = sensor_clock_get_cycles(&cycles);
	if (err != 0) {
		return err;
	}

	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	return 0;
}

static int rm3100_decoder_get_size_info(struct sensor_chan_spec chan_spec,
					 size_t *base_size,
					 size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	case SENSOR_CHAN_MAGN_XYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_data);
		return 0;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int rm3100_decoder_get_frame_count(const uint8_t *buffer,
					  struct sensor_chan_spec chan_spec,
					  uint16_t *frame_count)
{
	const struct rm3100_encoded_data *edata = (const struct rm3100_encoded_data *)buffer;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	uint8_t channel_request = rm3100_encode_channel(chan_spec.chan_type);

	if (((edata->header.channels & channel_request) != channel_request)) {
		return -ENODATA;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		*frame_count = 1;
		return 0;
	default:
		return -ENOTSUP;
	}

	return -1;
}

static int rm3100_convert_raw_to_q31(uint16_t cycle_count, uint32_t raw_reading,
				     q31_t *out, int8_t *shift)
{
	int64_t value;
	uint8_t divider;

	raw_reading = sys_be24_to_cpu(raw_reading);
	value = sign_extend(raw_reading, 23);

	/** Convert to Gauss, assuming 1 LSB = 75 uT, given default Cycle-Counting (200).
	 * We can represent the largest sample (2^23 LSB) in Gauss with 11 bits.
	 */
	if (cycle_count == RM3100_CYCLE_COUNT_DEFAULT) {
		*shift = 11;
		divider = 75;
	} else {
		/** Otherwise, it's 1 LSB = 38 uT at Cycle-counting for 600 Hz ODR (100):
		 * 12-bits max value.
		 */
		*shift = 12;
		divider = 38;
	}

	int64_t micro_tesla_scaled = ((int64_t)value << (31 - *shift)) / divider;
	int64_t gauss_scaled = (int64_t)micro_tesla_scaled / 100;

	*out = gauss_scaled;

	return 0;
}

static int rm3100_decoder_decode(const uint8_t *buffer,
				 struct sensor_chan_spec chan_spec,
				 uint32_t *fit,
				 uint16_t max_count,
				 void *data_out)
{
	const struct rm3100_encoded_data *edata = (const struct rm3100_encoded_data *)buffer;
	uint8_t channel_request;

	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z: {
		channel_request = rm3100_encode_channel(chan_spec.chan_type);
		if ((edata->header.channels & channel_request) != channel_request) {
			return -ENODATA;
		}

		struct sensor_q31_data *out = (struct sensor_q31_data *)data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		uint32_t raw_reading;

		if (chan_spec.chan_type == SENSOR_CHAN_MAGN_X) {
			raw_reading = edata->magn.x;
		} else if (chan_spec.chan_type == SENSOR_CHAN_MAGN_Y) {
			raw_reading = edata->magn.y;
		} else {
			raw_reading = edata->magn.z;
		}

		rm3100_convert_raw_to_q31(
			edata->header.cycle_count, raw_reading, &out->readings->value, &out->shift);

		*fit = 1;
		return 1;
	}
	case SENSOR_CHAN_MAGN_XYZ: {
		channel_request = rm3100_encode_channel(chan_spec.chan_type);
		if ((edata->header.channels & channel_request) != channel_request) {
			return -ENODATA;
		}

		struct sensor_three_axis_data *out = (struct sensor_three_axis_data *)data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		rm3100_convert_raw_to_q31(
			edata->header.cycle_count, edata->magn.x, &out->readings[0].x, &out->shift);
		rm3100_convert_raw_to_q31(
			edata->header.cycle_count, edata->magn.y, &out->readings[0].y, &out->shift);
		rm3100_convert_raw_to_q31(
			edata->header.cycle_count, edata->magn.z, &out->readings[0].z, &out->shift);

		*fit = 1;
		return 1;
	}
	default:
		return -EINVAL;
	}

	return -1;
}

static bool rm3100_decoder_has_trigger(const uint8_t *buffer,
					enum sensor_trigger_type trigger)
{
	const struct rm3100_encoded_data *edata = (const struct rm3100_encoded_data *)buffer;

	return edata->header.events.drdy && trigger == SENSOR_TRIG_DATA_READY;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = rm3100_decoder_get_frame_count,
	.get_size_info = rm3100_decoder_get_size_info,
	.decode = rm3100_decoder_decode,
	.has_trigger = rm3100_decoder_has_trigger,
};

int rm3100_get_decoder(const struct device *dev,
	const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);

	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
