/*
 * Copyright (c) 2025, CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "max30101.h"

LOG_MODULE_REGISTER(MAX30101_DECODER, CONFIG_SENSOR_LOG_LEVEL);

static int max30101_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	const struct max30101_encoded_data *edata = (const struct max30101_encoded_data *)buffer;

	if ((chan_spec.chan_type == SENSOR_CHAN_DIE_TEMP) && (chan_spec.chan_idx != 0)) {
		return -ENOTSUP;
	}

	if ((chan_spec.chan_idx < 0) || (chan_spec.chan_idx > 2)) {
		return -ENOTSUP;
	}

	/* Todo: This sensor lacks a FIFO; there will always only be one frame at a time. */
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_RED:
		*frame_count = !!edata->has_red ? 1 : 0;
		break;
	case SENSOR_CHAN_IR:
		*frame_count = !!edata->has_ir ? 1 : 0;
		break;
	case SENSOR_CHAN_GREEN:
		*frame_count = !!edata->has_green ? 1 : 0;
		break;
#if CONFIG_MAX30101_DIE_TEMPERATURE
	case SENSOR_CHAN_DIE_TEMP:
		*frame_count = edata->has_temp ? 1 : 0;
		break;
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
	default:
		return -ENOTSUP;
	}

	return (!*frame_count)? -ENODATA : 0;
}

static int max30101_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					  size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_RED:
	case SENSOR_CHAN_IR:
	case SENSOR_CHAN_GREEN:
	case SENSOR_CHAN_DIE_TEMP:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int max30101_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct max30101_encoded_data *edata = (const struct max30101_encoded_data *)buffer;
	const struct device *dev = edata->sensor;
	const struct max30101_config *config = dev->config;
	struct max30101_data *data = dev->data;

	if (*fit != 0) {
		return 0;
	}

	enum max30101_led_channel led_chan;
	struct sensor_q31_data *out = data_out;

	out->header.base_timestamp_ns = edata->header.timestamp;
	out->header.reading_count = 1;

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_RED:
		if (edata->has_red) {
			led_chan = MAX30101_LED_CHANNEL_RED;
		} else {
			return -ENODATA;
		}
		break;
	case SENSOR_CHAN_IR:
		if (edata->has_ir) {
			led_chan = MAX30101_LED_CHANNEL_IR;
		} else {
			return -ENODATA;
		}
		break;
	case SENSOR_CHAN_GREEN:
		if (edata->has_green) {
			led_chan = MAX30101_LED_CHANNEL_GREEN;
		} else {
			return -ENODATA;
		}
		break;
#if CONFIG_MAX30101_DIE_TEMPERATURE
	case SENSOR_CHAN_DIE_TEMP:
		if (edata->has_temp) {
			out->readings[0].temperature =
				(edata->reading.die_temp[0] << MAX30101_TEMP_FRAC_SHIFT) |
				(edata->reading.die_temp[1] & 0x0f);
			out->shift = MAX30101_ASYNC_RESOLUTION - MAX30101_TEMP_FRAC_SHIFT;
			*fit = 1;
			return 1;
		} else {
			return -ENODATA;
		}
		break;
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */
	default:
		return -EINVAL;
	}

	if (chan_spec.chan_idx >= data->num_channels[led_chan]) {
		LOG_ERR("Channel index out of range [%d/%d]", chan_spec.chan_idx,
			data->num_channels[led_chan]);
		return -EINVAL;
	}

	uint8_t index = MAX30101_BYTES_PER_CHANNEL * data->map[led_chan][chan_spec.chan_idx];
	out->readings[0].value = (edata->reading.raw[index] << 16) | (edata->reading.raw[index + 1] << 8) | (edata->reading.raw[index + 2]);
	out->readings[0].value = (out->readings[0].value & MAX30101_FIFO_DATA_MASK) >> config->data_shift;
	out->shift = MAX30101_ASYNC_RESOLUTION - MAX30101_LIGHT_SHIFT;
	*fit = 1;

	return 1;
}

#if CONFIG_MAX30101_STREAM
static bool max30101_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	struct max30101_encoded_data *edata = (struct max30101_encoded_data *)buffer;
	bool has_trigger = false;

	switch (trigger)
	{
	case SENSOR_TRIG_DATA_READY:
		has_trigger = edata->has_data_rdy;
		edata->has_data_rdy = 0;
		break;
	case SENSOR_TRIG_FIFO_WATERMARK:
		has_trigger = edata->has_watermark;
		edata->has_watermark = 0;
		break;
	case SENSOR_TRIG_OVERFLOW:
		has_trigger = edata->has_overflow;
		edata->has_overflow = 0;
		break;
	default:
		LOG_ERR("Unsupported trigger type %d", trigger);
		return false;
	}

	return has_trigger;
}
#endif

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = max30101_decoder_get_frame_count,
	.get_size_info = max30101_decoder_get_size_info,
	.decode = max30101_decoder_decode,
#if CONFIG_MAX30101_STREAM
	.has_trigger = max30101_decoder_has_trigger,
#endif /* CONFIG_MAX30101_STREAM */
};

int max30101_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
