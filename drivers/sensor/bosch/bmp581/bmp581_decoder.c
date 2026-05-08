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

/* Period in ns indexed by BMP581_DT_ODR_* value (0x00 = 240 Hz .. 0x1D = 0.5 Hz) */
static const uint32_t odr_period_ns[] = {
	[0x00] = UINT32_C(4166666),    /* 240 Hz   */
	[0x01] = UINT32_C(4575637),    /* 218.5 Hz */
	[0x02] = UINT32_C(5022603),    /* 199.1 Hz */
	[0x03] = UINT32_C(5580357),    /* 179.2 Hz */
	[0x04] = UINT32_C(6250000),    /* 160 Hz   */
	[0x05] = UINT32_C(6698592),    /* 149.3 Hz */
	[0x06] = UINT32_C(7142857),    /* 140 Hz   */
	[0x07] = UINT32_C(7703559),    /* 129.8 Hz */
	[0x08] = UINT32_C(8333333),    /* 120 Hz   */
	[0x09] = UINT32_C(9082652),    /* 110.1 Hz */
	[0x0A] = UINT32_C(9980040),    /* 100.2 Hz */
	[0x0B] = UINT32_C(11160714),   /* 89.6 Hz  */
	[0x0C] = UINT32_C(12500000),   /* 80 Hz    */
	[0x0D] = UINT32_C(14285714),   /* 70 Hz    */
	[0x0E] = UINT32_C(16666667),   /* 60 Hz    */
	[0x0F] = UINT32_C(20000000),   /* 50 Hz    */
	[0x10] = UINT32_C(22222222),   /* 45 Hz    */
	[0x11] = UINT32_C(25000000),   /* 40 Hz    */
	[0x12] = UINT32_C(28571428),   /* 35 Hz    */
	[0x13] = UINT32_C(33333333),   /* 30 Hz    */
	[0x14] = UINT32_C(40000000),   /* 25 Hz    */
	[0x15] = UINT32_C(50000000),   /* 20 Hz    */
	[0x16] = UINT32_C(66666667),   /* 15 Hz    */
	[0x17] = UINT32_C(100000000),  /* 10 Hz    */
	[0x18] = UINT32_C(200000000),  /* 5 Hz     */
	[0x19] = UINT32_C(250000000),  /* 4 Hz     */
	[0x1A] = UINT32_C(333333333),  /* 3 Hz     */
	[0x1B] = UINT32_C(500000000),  /* 2 Hz     */
	[0x1C] = UINT32_C(1000000000), /* 1 Hz     */
	[0x1D] = UINT32_C(2000000000), /* 0.5 Hz   */
};

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

	edata->header.channels = 0;
	edata->header.press_en = data->osr_odr_press_config.press_en;
	edata->header.odr = data->osr_odr_press_config.odr;

	if (trigger_status) {
		edata->header.channels |= bmp581_encode_channel(SENSOR_CHAN_ALL);
		edata->header.fifo_count = data->stream.fifo_thres;
		edata->header.timestamp = data->stream.timestamp;
	} else {
		const struct sensor_chan_spec *const channels = read_config->channels;
		size_t num_channels = read_config->count;

		for (size_t i = 0; i < num_channels; i++) {
			edata->header.channels |= bmp581_encode_channel(channels[i].chan_type);
		}

		uint64_t cycles;
		int err = sensor_clock_get_cycles(&cycles);

		if (err != 0) {
			return err;
		}
		edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);
	}

	edata->header.events = trigger_status;

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

	if (edata->header.events & BMP581_EVENT_FIFO_WM) {
		*frame_count = edata->header.fifo_count;
	} else {
		*frame_count = 1;
	}
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

static int bmp581_convert_raw_to_q31_value(const struct bmp581_encoded_header *header,
					   struct sensor_chan_spec *chan_spec,
					   const struct bmp581_frame *frame,
					   uint32_t *fit,
					   struct sensor_q31_data *out)
{
	if (((header->events & BMP581_EVENT_FIFO_WM) != 0 && *fit >= header->fifo_count) ||
	     ((header->events & BMP581_EVENT_FIFO_WM) == 0 && *fit != 0)) {
		return -ENODATA;
	}

	switch (chan_spec->chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP: {
		/* Temperature is in data[2:0], data[2] is integer part */
		uint32_t raw_temp = ((uint32_t)frame[*fit].payload[2] << 16) |
				    ((uint16_t)frame[*fit].payload[1] << 8) |
				    frame[*fit].payload[0];
		int32_t raw_temp_signed = sign_extend(raw_temp, 23);

		out->shift = (31 - 16); /* 16 left shifts gives us the value in celsius */
		out->readings[*fit].value = raw_temp_signed;
		break;
	}
	case SENSOR_CHAN_PRESS: {
		if (!header->press_en) {
			return -ENODATA;
		}
		/* Shift by 10 bits because we'll divide by 1000 to make it kPa */
		uint64_t raw_press = (((uint32_t)frame[*fit].payload[5] << 16) |
				       ((uint16_t)frame[*fit].payload[4] << 8) |
				       frame[*fit].payload[3]);

		int64_t raw_press_signed = sign_extend_64(raw_press, 23);

		raw_press_signed *= 1024;
		raw_press_signed /= 1000;

		/* Original value was in Pa by left-shifting 6 spaces, but
		 * we've multiplied by 2^10 to not lose precision when
		 * converting to kPa. Hence, left-shift 16 spaces.
		 */
		out->shift = (31 - 6 - 10);
		out->readings[*fit].value = (int32_t)raw_press_signed;
		break;
	}
	default:
		return -EINVAL;
	}

	*fit = (*fit) + 1;
	return 0;
}

static int bmp581_decoder_decode(const uint8_t *buffer,
				struct sensor_chan_spec chan_spec,
				uint32_t *fit,
				uint16_t max_count,
				void *data_out)
{
	const struct bmp581_encoded_data *edata = (const struct bmp581_encoded_data *)buffer;
	uint8_t channel_request;

	if (max_count == 0 || chan_spec.chan_idx != 0) {
		return -EINVAL;
	}

	channel_request = bmp581_encode_channel(chan_spec.chan_type);
	if ((channel_request & edata->header.channels) != channel_request) {
		return -ENODATA;
	}

	struct sensor_q31_data *out = data_out;
	uint8_t total_frames = (edata->header.events & BMP581_EVENT_FIFO_WM)
			       ? edata->header.fifo_count : 1;
	uint32_t period_ns = (edata->header.odr < ARRAY_SIZE(odr_period_ns))
			     ? odr_period_ns[edata->header.odr] : 0;

	out->header.base_timestamp_ns =
		edata->header.timestamp -
		(uint64_t)(total_frames > 0 ? total_frames - 1 : 0) * period_ns;

	int err;
	uint32_t fit_0 = *fit;
	uint32_t frame_idx = 0;

	do {
		err = bmp581_convert_raw_to_q31_value(&edata->header, &chan_spec,
						      edata->frame, fit, out);
		if (err == 0) {
			out->readings[frame_idx].timestamp_delta = frame_idx * period_ns;
			frame_idx++;
		}
	} while (err == 0 && *fit < max_count);

	if (*fit == fit_0 || err != 0) {
		return err;
	}

	out->header.reading_count = *fit;
	return *fit - fit_0;
}

static bool bmp581_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct bmp581_encoded_data *edata = (const struct bmp581_encoded_data *)buffer;

	if ((trigger == SENSOR_TRIG_DATA_READY &&
	     edata->header.events & BMP581_EVENT_DRDY) ||
	    (trigger == SENSOR_TRIG_FIFO_WATERMARK &&
	     edata->header.events & BMP581_EVENT_FIFO_WM)) {
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
