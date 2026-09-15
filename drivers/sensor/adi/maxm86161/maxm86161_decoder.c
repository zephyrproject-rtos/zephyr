/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "maxm86161.h"

LOG_MODULE_DECLARE(MAXM86161, CONFIG_SENSOR_LOG_LEVEL);

#define NS_PER_SECOND UINT64_C(1000000000)

static int get_fifo_channel(uint32_t fifo_data)
{
	int tag = FIELD_GET(MAXM86161_FIFO_TAG_MASK, fifo_data);

	switch (tag) {
	case MAXM86161_FIFO_TAG_LEDC1:
		return SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC1;
	case MAXM86161_FIFO_TAG_LEDC2:
		return SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC2;
	case MAXM86161_FIFO_TAG_LEDC3:
		return SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC3;
	case MAXM86161_FIFO_TAG_LEDC4:
		return SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC4;
	case MAXM86161_FIFO_TAG_LEDC5:
		return SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC5;
	case MAXM86161_FIFO_TAG_LEDC6:
		return SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC6;
	case MAXM86161_FIFO_TAG_LEDC1_PF:
		return SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC1_PF;
	case MAXM86161_FIFO_TAG_LEDC2_PF:
		return SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC2_PF;
	case MAXM86161_FIFO_TAG_LEDC3_PF:
		return SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC3_PF;
	case MAXM86161_FIFO_TAG_PROX:
		return SENSOR_CHAN_MAXM86161_FIFO_PROX;
	case MAXM86161_FIFO_TAG_SUB_DAC:
		return SENSOR_CHAN_MAXM86161_FIFO_SUB_DAC;
	case MAXM86161_FIFO_TAG_TIMESTAMP:
		return SENSOR_CHAN_MAXM86161_FIFO_TIMESTAMP;
	case MAXM86161_FIFO_TAG_INVALID:
		LOG_WRN("Read empty FIFO.");
	default:
		return -EINVAL;
	}
}

static int maxm86161_decoder_get_frame_count(const uint8_t *buffer,
					     struct sensor_chan_spec chan_spec,
					     uint16_t *frame_count)
{
	const struct maxm86161_fifo_hdr *hdr = (const struct maxm86161_fifo_hdr *)buffer;
	const uint8_t *samples = buffer + sizeof(struct maxm86161_fifo_hdr);
	const uint8_t *end = samples + hdr->fifo_byte_count;
	uint16_t count = 0;

	while (samples < end) {
		uint32_t fifo_data = sys_get_be24(samples);
		int chan = get_fifo_channel(fifo_data);

		if (chan >= 0 && chan == (int)chan_spec.chan_type) {
			count++;
		}

		samples += MAXM86161_FIFO_SAMPLE_SIZE;
	}

	*frame_count = count;
	return 0;
}

static int maxm86161_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					   size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC1:
	case SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC1_PF:
	case SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC2:
	case SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC2_PF:
	case SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC3:
	case SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC3_PF:
	case SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC4:
	case SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC5:
	case SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC6:
	case SENSOR_CHAN_MAXM86161_FIFO_PROX:
	case SENSOR_CHAN_MAXM86161_FIFO_SUB_DAC:
	case SENSOR_CHAN_MAXM86161_FIFO_TIMESTAMP:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int maxm86161_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				    uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct maxm86161_fifo_hdr *hdr = (const struct maxm86161_fifo_hdr *)buffer;
	struct sensor_q31_data *output = (struct sensor_q31_data *)data_out;
	const uint8_t *samples = buffer + sizeof(struct maxm86161_fifo_hdr);
	const uint8_t *end = samples + hdr->fifo_byte_count;
	uint64_t period_ns = (hdr->odr > 0) ? (NS_PER_SECOND / hdr->odr) : 0;
	int count = 0;
	uint32_t samples_parsed = 0;
	uint32_t start_offset = *fit;
	uint16_t total = 0;

	if (maxm86161_decoder_get_frame_count(buffer, chan_spec, &total) != 0) {
		return -ENOTSUP;
	}

	if (total == 0) {
		return 0;
	}

	output->header.base_timestamp_ns = hdr->timestamp - (uint64_t)(total - 1) * period_ns;
	output->shift = 0;

	while (count < max_count && samples < end) {
		uint32_t fifo_data = sys_get_be24(samples);
		int chan = get_fifo_channel(fifo_data);

		if (chan < 0) {
			return chan;
		}

		if (chan == (int)chan_spec.chan_type) {
			if (samples_parsed >= start_offset) {
				output->readings[count].timestamp_delta =
				    (uint32_t)(samples_parsed * period_ns);
				output->readings[count].value =
				    FIELD_GET(MAXM86161_FIFO_DATA_MASK, fifo_data);
				count++;
			}
			samples_parsed++;
		}
		samples += MAXM86161_FIFO_SAMPLE_SIZE;
	}

	output->header.reading_count = count;
	*fit += count;
	return count;
}

static bool maxm86161_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct maxm86161_fifo_hdr *hdr = (const struct maxm86161_fifo_hdr *)buffer;

	switch (trigger) {
	case SENSOR_TRIG_FIFO_FULL:
	case SENSOR_TRIG_FIFO_WATERMARK:
		return FIELD_GET(MAXM86161_MSK_INT_STATUS1_A_FULL, hdr->int_status);
	default:
		return false;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = maxm86161_decoder_get_frame_count,
	.get_size_info = maxm86161_decoder_get_size_info,
	.decode = maxm86161_decoder_decode,
	.has_trigger = maxm86161_decoder_has_trigger,
};

int maxm86161_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);

	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
