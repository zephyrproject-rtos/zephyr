/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys/util.h>
#include "bmp581.h"
#include "bmp581_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(BMP581_DECODER, CONFIG_SENSOR_LOG_LEVEL);

static uint8_t bmp581_encode_channel(enum sensor_channel chan)
{
	uint8_t encode_bmask = 0;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		encode_bmask |= BIT(0);
		break;
	case SENSOR_CHAN_PRESS:
		encode_bmask |= BIT(1);
		break;
	case SENSOR_CHAN_ALL:
		encode_bmask |= BIT(0) | BIT(1);
		break;
	default:
		break;
	}

	return encode_bmask;
}

int bmp581_encode(const struct device *dev,
		  const struct sensor_read_config *read_config,
		  uint8_t trigger_status,
		  uint8_t *buf)
{
	struct bmp581_encoded_data *edata = (struct bmp581_encoded_data *)buf;
	struct bmp581_data *data = dev->data;
	uint64_t cycles;
	int err;

	edata->header.channels = 0;
	edata->header.press_en = data->osr_odr_press_config.press_en;

	if (trigger_status) {
		edata->header.channels |= bmp581_encode_channel(SENSOR_CHAN_ALL);
	} else {
		const struct sensor_chan_spec *const channels = read_config->channels;
		size_t num_channels = read_config->count;

		for (size_t i = 0; i < num_channels; i++) {
			edata->header.channels |= bmp581_encode_channel(channels[i].chan_type);
		}
	}

	err = sensor_clock_get_cycles(&cycles);
	if (err != 0) {
		return err;
	}

	edata->header.events = trigger_status ? BIT(0) : 0;
	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);

	return 0;
}

static int bmp581_decoder_get_frame_count(const uint8_t *buffer,
					 struct sensor_chan_spec chan_spec,
					 uint16_t *frame_count)
{
	const struct bmp581_encoded_data *edata = (const struct bmp581_encoded_data *)buffer;

	if (chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	uint8_t channel_request = bmp581_encode_channel(chan_spec.chan_type);

	/* Filter unknown channels and having no data */
	if ((edata->header.channels & channel_request) != channel_request) {
		return -ENODATA;
	}

	*frame_count = 1;
	return 0;
}

static int bmp581_decoder_get_size_info(struct sensor_chan_spec chan_spec,
					size_t *base_size,
					size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_PRESS:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int bmp581_decoder_decode(const uint8_t *buffer,
				struct sensor_chan_spec chan_spec,
				uint32_t *fit,
				uint16_t max_count,
				void *data_out)
{
	const struct bmp581_encoded_data *edata = (const struct bmp581_encoded_data *)buffer;
	uint8_t channel_request;

	if (*fit != 0) {
		return 0;
	}

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	channel_request = bmp581_encode_channel(chan_spec.chan_type);
	if ((channel_request & edata->header.channels) != channel_request) {
		return -ENODATA;
	}

	struct sensor_q31_data *out = data_out;

	out->header.base_timestamp_ns = edata->header.timestamp;
	out->header.reading_count = 1;

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP: {
		/* Temperature is in data[2:0], data[2] is integer part */
		uint32_t raw_temp = ((uint32_t)edata->payload[2] << 16) |
				    ((uint16_t)edata->payload[1] << 8) |
				    edata->payload[0];
		int32_t raw_temp_signed = sign_extend(raw_temp, 23);

		out->shift = (31 - 16); /* 16 left shifts gives us the value in celsius */
		out->readings[0].value = raw_temp_signed;
		break;
	}
	case SENSOR_CHAN_PRESS:
		if (!edata->header.press_en) {
			return -ENODATA;
		}
		/* Shift by 10 bits because we'll divide by 1000 to make it kPa */
		uint64_t raw_press = (((uint32_t)edata->payload[5] << 16) |
				       ((uint16_t)edata->payload[4] << 8) |
				       edata->payload[3]);

		int64_t raw_press_signed = sign_extend_64(raw_press, 23);

		raw_press_signed *= 1024;
		raw_press_signed /= 1000;

		/* Original value was in Pa by left-shifting 6 spaces, but
		 * we've multiplied by 2^10 to not lose precision when
		 * converting to kPa. Hence, left-shift 16 spaces.
		 */
		out->shift = (31 - 6 - 10);
		out->readings[0].value = (int32_t)raw_press_signed;
		break;
	default:
		return -EINVAL;
	}

	*fit = 1;
	return 1;
}

static bool bmp581_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct bmp581_encoded_data *edata = (const struct bmp581_encoded_data *)buffer;

	if ((trigger == SENSOR_TRIG_DATA_READY) && (edata->header.events != 0)) {
		return true;
	}

	return false;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = bmp581_decoder_get_frame_count,
	.get_size_info = bmp581_decoder_get_size_info,
	.decode = bmp581_decoder_decode,
	.has_trigger = bmp581_decoder_has_trigger,
};

int bmp581_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
