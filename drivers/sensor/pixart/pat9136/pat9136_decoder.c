/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor/pat9136.h>

#include "pat9136.h"
#include "pat9136_reg.h"
#include "pat9136_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(PAT9136_DECODER, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT pixart_pat9136

static int pat9136_get_shift(uint16_t channel,
			     uint16_t res_x,
			     uint16_t res_y,
			     int8_t *shift)
{
	/** Table generated based on calculation of required bits:
	 * - resolution_cpi (per datasheet) = (1 + resolution) * 100
	 * - value_mm = value * 25.4 / resolution_cpi
	 *  - Bits Required = round_up( Log2(value_mm) )
	 */
	struct {
		uint16_t min;
		uint16_t max;
		int8_t shift;
	} const static shift_based_on_ranges[] = {
		{.min = 0, .max = 0, .shift = 14},
		{.min = 1, .max = 1, .shift = 13},
		{.min = 2, .max = 3, .shift = 12},
		{.min = 4, .max = 7, .shift = 11},
		{.min = 8, .max = 15, .shift = 10},
		{.min = 16, .max = 31, .shift = 9},
		{.min = 32, .max = 63, .shift = 8},
		{.min = 64, .max = 127, .shift = 7},
		{.min = 128, .max = 199, .shift = 6},
	};
	/** Going with lowest resolution to be able to represent biggest value */
	uint16_t resolution = MIN(res_x, res_y);

	switch (channel) {
	case SENSOR_CHAN_POS_DX:
	case SENSOR_CHAN_POS_DY:
	case SENSOR_CHAN_POS_DXYZ:
		*shift = 31;
		return 0;
	case SENSOR_CHAN_POS_DX_MM:
	case SENSOR_CHAN_POS_DY_MM:
	case SENSOR_CHAN_POS_DXYZ_MM:
		for (size_t i = 0 ; i < ARRAY_SIZE(shift_based_on_ranges) ; i++) {
			if (resolution >= shift_based_on_ranges[i].min &&
			    resolution <= shift_based_on_ranges[i].max) {
				*shift = shift_based_on_ranges[i].shift;
				return 0;
			}
		}
	default:
		return -EINVAL;
	}

	return -EIO;
}

static void pat9136_convert_raw_to_q31(struct pat9136_encoded_data *edata,
				       uint16_t chan,
				       int32_t reading,
				       q31_t *out)
{
	int8_t shift;
	uint32_t resolution_setting = MIN(edata->header.resolution.x,
					  edata->header.resolution.y);
	int64_t intermediate;

	(void)pat9136_get_shift(chan,
				resolution_setting,
				resolution_setting,
				&shift);

	switch (chan) {
	case SENSOR_CHAN_POS_DX:
	case SENSOR_CHAN_POS_DY:
		*out = (q31_t)reading;
		return;
	case SENSOR_CHAN_POS_DX_MM:
	case SENSOR_CHAN_POS_DY_MM: {
		uint32_t resolution = (chan == SENSOR_CHAN_POS_DX_MM) ?
				      (edata->header.resolution.x + 1) * 100 :
				      (edata->header.resolution.y + 1) * 100;

		intermediate = (((int64_t)reading * INT64_C(1000000) * 25.4));
		intermediate /= resolution;

		intermediate *= ((int64_t)INT32_MAX + 1) / ((1 << shift) * INT64_C(1000000));

		*out = CLAMP(intermediate, INT32_MIN, INT32_MAX);

		return;
	}
	default:
		CODE_UNREACHABLE;
	}
}

uint8_t pat9136_encode_channel(uint16_t chan)
{
	switch (chan) {
	case SENSOR_CHAN_POS_DX:
		return BIT(0);
	case SENSOR_CHAN_POS_DY:
		return BIT(1);
	case SENSOR_CHAN_POS_DXYZ:
		return BIT(2);
	case SENSOR_CHAN_POS_DX_MM:
		return BIT(3);
	case SENSOR_CHAN_POS_DY_MM:
		return BIT(4);
	case SENSOR_CHAN_POS_DXYZ_MM:
		return BIT(5);
	case SENSOR_CHAN_ALL:
		return BIT_MASK(6);
	default:
		return 0;
	}
}

static bool is_data_valid(const struct pat9136_encoded_data *edata)
{
	if (!REG_MOTION_DETECTED(edata->motion)) {
		LOG_WRN("Invalid data - No motion detected");
		return false;
	}

	if (!REG_OBSERVATION_READ_IS_VALID(edata->observation)) {
		LOG_WRN("Invalid data - Observation read is not valid");
		return false;
	}

	return true;
}

static int pat9136_decoder_get_frame_count(const uint8_t *buffer,
					   struct sensor_chan_spec chan_spec,
					   uint16_t *frame_count)
{
	struct pat9136_encoded_data *edata = (struct pat9136_encoded_data *)buffer;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	uint8_t channel_request = pat9136_encode_channel(chan_spec.chan_type);

	if (((edata->header.channels & channel_request) != channel_request) ||
	    !is_data_valid(edata)) {
		return -ENODATA;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_POS_DX:
	case SENSOR_CHAN_POS_DY:
	case SENSOR_CHAN_POS_DXYZ:
	case SENSOR_CHAN_POS_DX_MM:
	case SENSOR_CHAN_POS_DY_MM:
	case SENSOR_CHAN_POS_DXYZ_MM:
		*frame_count = 1;
		return 0;
	default:
		return -ENOTSUP;
	}

	return -1;
}

static int pat9136_decoder_get_size_info(struct sensor_chan_spec chan_spec,
					 size_t *base_size,
					 size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_POS_DX:
	case SENSOR_CHAN_POS_DY:
	case SENSOR_CHAN_POS_DX_MM:
	case SENSOR_CHAN_POS_DY_MM:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	case SENSOR_CHAN_POS_DXYZ:
	case SENSOR_CHAN_POS_DXYZ_MM:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int pat9136_decoder_decode(const uint8_t *buffer,
				  struct sensor_chan_spec chan_spec,
				  uint32_t *fit,
				  uint16_t max_count,
				  void *data_out)
{
	struct pat9136_encoded_data *edata = (struct pat9136_encoded_data *)buffer;
	uint8_t channel_request;

	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_POS_DX_MM:
	case SENSOR_CHAN_POS_DY_MM:
	case SENSOR_CHAN_POS_DX:
	case SENSOR_CHAN_POS_DY: {
		channel_request = pat9136_encode_channel(chan_spec.chan_type);
		if (((edata->header.channels & channel_request) != channel_request) ||
		    !is_data_valid(edata)) {
			LOG_ERR("No data available");
			return -ENODATA;
		}

		struct sensor_q31_data *out = (struct sensor_q31_data *)data_out;
		int16_t raw_value = (chan_spec.chan_type == SENSOR_CHAN_POS_DX ||
				     chan_spec.chan_type == SENSOR_CHAN_POS_DX_MM) ?
				    edata->delta.x :
				    edata->delta.y;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		(void)pat9136_get_shift(chan_spec.chan_type,
					edata->header.resolution.x,
					edata->header.resolution.y,
					&out->shift);

		(void)pat9136_convert_raw_to_q31(edata,
						 chan_spec.chan_type,
						 raw_value,
						 &out->readings->value);

		*fit = 1;
		return 1;
	}
	case SENSOR_CHAN_POS_DXYZ_MM:
	case SENSOR_CHAN_POS_DXYZ: {
		channel_request = pat9136_encode_channel(chan_spec.chan_type);
		if (((edata->header.channels & channel_request) != channel_request) ||
		    !is_data_valid(edata)) {
			LOG_ERR("No data available");
			return -ENODATA;
		}

		struct sensor_three_axis_data *out = (struct sensor_three_axis_data *)data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		(void)pat9136_get_shift(chan_spec.chan_type,
					edata->header.resolution.x,
					edata->header.resolution.y,
					&out->shift);

		(void)pat9136_convert_raw_to_q31(edata,
						 chan_spec.chan_type - 3,
						 edata->delta.x,
						 &out->readings[0].x);
		(void)pat9136_convert_raw_to_q31(edata,
						 chan_spec.chan_type - 2,
						 edata->delta.y,
						 &out->readings[0].y);
		out->readings[0].z = 0;

		*fit = 1;
		return 1;
	}
	default:
		return -EINVAL;
	}

	return -1;
}

static bool pat9136_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	struct pat9136_encoded_data *edata = (struct pat9136_encoded_data *)buffer;

	switch (trigger) {
	case SENSOR_TRIG_DATA_READY:
		return edata->header.events.drdy;
	case SENSOR_TRIG_MOTION:
		return edata->header.events.motion;
	default:
		return false;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = pat9136_decoder_get_frame_count,
	.get_size_info = pat9136_decoder_get_size_info,
	.decode = pat9136_decoder_decode,
	.has_trigger = pat9136_decoder_has_trigger,
};

int pat9136_get_decoder(const struct device *dev,
	const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}

int pat9136_encode(const struct device *dev,
		   const struct sensor_chan_spec *const channels,
		   size_t num_channels,
		   uint8_t *buf)
{
	struct pat9136_encoded_data *edata = (struct pat9136_encoded_data *)buf;
	uint64_t cycles;
	int err;

	edata->header.channels = 0;
	edata->header.events.drdy = 0;
	edata->header.events.motion = 0;

	for (size_t i = 0 ; i < num_channels; i++) {
		edata->header.channels |= pat9136_encode_channel(channels[i].chan_type);
	}

	err = sensor_clock_get_cycles(&cycles);
	if (err != 0) {
		return err;
	}

	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	return 0;
}
