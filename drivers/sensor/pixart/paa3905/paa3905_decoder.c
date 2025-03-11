/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys/byteorder.h>

#include "paa3905.h"
#include "paa3905_reg.h"
#include "paa3905_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(PAA3905_DECODER, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT pixart_paa3905

uint8_t paa3905_encode_channel(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_POS_DX:
		return BIT(0);
	case SENSOR_CHAN_POS_DY:
		return BIT(1);
	case SENSOR_CHAN_POS_DXYZ:
		return BIT(2);
	default:
		return 0;
	}
}

static bool is_data_valid(const struct paa3905_encoded_data *edata)
{
	uint8_t squal_min;
	uint32_t shutter_max;
	uint32_t shutter;

	if (!REG_MOTION_DETECTED(edata->motion)) {
		LOG_WRN("Invalid data - No motion detected");
		return false;
	}

	if (REG_MOTION_CHALLENGING_COND(edata->motion)) {
		LOG_WRN("Invalid data - Challenging conditions");
		return false;
	}

	switch (REG_OBSERVATION_MODE(edata->observation)) {
	case OBSERVATION_MODE_BRIGHT:
		squal_min = SQUAL_MIN_BRIGHT;
		shutter_max = SHUTTER_MAX_BRIGHT;
		break;
	case OBSERVATION_MODE_LOW_LIGHT:
		squal_min = SQUAL_MIN_LOW_LIGHT;
		shutter_max = SHUTTER_MAX_LOW_LIGHT;
		break;
	case OBSERVATION_MODE_SUPER_LOW_LIGHT:
		squal_min = SQUAL_MIN_SUPER_LOW_LIGHT;
		shutter_max = SHUTTER_MAX_SUPER_LOW_LIGHT;
		break;
	default:
		LOG_ERR("Invalid op mode");
		return false;
	}

	shutter = sys_be24_to_cpu(edata->shutter);

	if (edata->squal < squal_min || shutter >= shutter_max) {
		LOG_WRN("Invalid data - mode: %d squal: 0x%02X shutter: 0x%06X",
			(uint8_t)REG_OBSERVATION_MODE(edata->observation), edata->squal, shutter);

		return false;
	}

	return true;
}

static int paa3905_decoder_get_frame_count(const uint8_t *buffer,
					   struct sensor_chan_spec chan_spec,
					   uint16_t *frame_count)
{
	struct paa3905_encoded_data *edata = (struct paa3905_encoded_data *)buffer;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	uint8_t channel_request = paa3905_encode_channel(chan_spec.chan_type);

	if (((edata->header.channels & channel_request) != channel_request) ||
	    !is_data_valid(edata)) {
		return -ENODATA;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_POS_DX:
	case SENSOR_CHAN_POS_DY:
	case SENSOR_CHAN_POS_DXYZ:
		*frame_count = 1;
		return 0;
	default:
		return -ENOTSUP;
	}

	return -1;
}

static int paa3905_decoder_get_size_info(struct sensor_chan_spec chan_spec,
					 size_t *base_size,
					 size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_POS_DX:
	case SENSOR_CHAN_POS_DY:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	case SENSOR_CHAN_POS_DXYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int paa3905_decoder_decode(const uint8_t *buffer,
				  struct sensor_chan_spec chan_spec,
				  uint32_t *fit,
				  uint16_t max_count,
				  void *data_out)
{
	struct paa3905_encoded_data *edata = (struct paa3905_encoded_data *)buffer;
	uint8_t channel_request;

	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_POS_DX:
	case SENSOR_CHAN_POS_DY: {
		channel_request = paa3905_encode_channel(chan_spec.chan_type);
		if (((edata->header.channels & channel_request) != channel_request) ||
		    !is_data_valid(edata)) {
			LOG_ERR("No data available");
			return -ENODATA;
		}

		struct sensor_q31_data *out = (struct sensor_q31_data *)data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = 31;

		out->readings->value = (chan_spec.chan_type == SENSOR_CHAN_POS_DX) ?
				       edata->delta.x :
				       edata->delta.y;

		*fit = 1;
		return 1;
	}
	case SENSOR_CHAN_POS_DXYZ: {
		channel_request = paa3905_encode_channel(chan_spec.chan_type);
		if (((edata->header.channels & channel_request) != channel_request) ||
		    !is_data_valid(edata)) {
			LOG_ERR("No data available");
			return -ENODATA;
		}

		struct sensor_three_axis_data *out = (struct sensor_three_axis_data *)data_out;

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;
		out->shift = 31;

		out->readings[0].x = edata->delta.x;
		out->readings[0].y = edata->delta.y;
		out->readings[0].z = 0;

		*fit = 1;
		return 1;
	}
	default:
		return -EINVAL;
	}

	return -1;
}

static bool paa3905_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	struct paa3905_encoded_data *edata = (struct paa3905_encoded_data *)buffer;

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
	.get_frame_count = paa3905_decoder_get_frame_count,
	.get_size_info = paa3905_decoder_get_size_info,
	.decode = paa3905_decoder_decode,
	.has_trigger = paa3905_decoder_has_trigger,
};

int paa3905_get_decoder(const struct device *dev,
	const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}

int paa3905_encode(const struct device *dev,
		   const struct sensor_chan_spec *const channels,
		   size_t num_channels,
		   uint8_t *buf)
{
	struct paa3905_encoded_data *edata = (struct paa3905_encoded_data *)buf;
	uint64_t cycles;
	int err;

	edata->header.channels = 0;
	edata->header.events.drdy = false;
	edata->header.events.motion = false;

	for (size_t i = 0 ; i < num_channels; i++) {
		edata->header.channels |= paa3905_encode_channel(channels[i].chan_type);
	}

	err = sensor_clock_get_cycles(&cycles);
	if (err != 0) {
		return err;
	}

	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	return 0;
}
