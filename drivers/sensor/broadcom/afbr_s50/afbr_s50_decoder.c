/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_afbr_s50

#include <zephyr/drivers/sensor_clock.h>
#include <api/argus_res.h>

#include "afbr_s50_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(AFBR_S50_DECODER, CONFIG_SENSOR_LOG_LEVEL);

uint8_t afbr_s50_encode_channel(uint16_t chan)
{
	switch (chan) {
	case SENSOR_CHAN_DISTANCE:
		return BIT(0);
	default:
		return 0;
	}
}

uint8_t afbr_s50_encode_event(enum sensor_trigger_type trigger)
{
	if (trigger == SENSOR_TRIG_DATA_READY) {
		return BIT(0);
	}

	return 0;
}

static int afbr_s50_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	const struct afbr_s50_edata *edata = (const struct afbr_s50_edata *)buffer;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_DISTANCE:
		if (edata->header.channels & afbr_s50_encode_channel(SENSOR_CHAN_DISTANCE)) {
			*frame_count = 1;
			return 0;
		}
		break;
	default:
		break;
	}

	*frame_count = 0;
	return 0;
}

static int afbr_s50_decoder_get_size_info(struct sensor_chan_spec chan_spec,
					  size_t *base_size,
					  size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_DISTANCE:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int afbr_s50_decoder_decode(const uint8_t *buffer,
				   struct sensor_chan_spec chan_spec,
				   uint32_t *fit,
				   uint16_t max_count,
				   void *data_out)
{
	const struct afbr_s50_edata *edata = (const struct afbr_s50_edata *)buffer;

	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_DISTANCE: {
		struct sensor_q31_data *out = data_out;

		if ((edata->header.channels & afbr_s50_encode_channel(SENSOR_CHAN_DISTANCE)) == 0) {
			return -ENODATA;
		}

		out->header.base_timestamp_ns = edata->header.timestamp;
		out->header.reading_count = 1;

		/* Result comes encoded in Q9.22 format */
		out->shift = 9;
		out->readings[0].value = edata->payload.Bin.Range;

		*fit = 1;
		return 1;
	}
	default:
		return -EINVAL;
	}
}

static bool afbr_s50_decoder_has_trigger(const uint8_t *buffer,
					 enum sensor_trigger_type trigger)
{
	const struct afbr_s50_edata *edata = (const struct afbr_s50_edata *)buffer;

	if (trigger == SENSOR_TRIG_DATA_READY) {
		return edata->header.events & afbr_s50_encode_event(SENSOR_TRIG_DATA_READY);
	} else {
		return false;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = afbr_s50_decoder_get_frame_count,
	.get_size_info = afbr_s50_decoder_get_size_info,
	.decode = afbr_s50_decoder_decode,
	.has_trigger = afbr_s50_decoder_has_trigger,
};

int afbr_s50_get_decoder(const struct device *dev,
			 const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
