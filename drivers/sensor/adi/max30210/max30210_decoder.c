/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "max30210.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(MAX30210, CONFIG_SENSOR_LOG_LEVEL);

#define MAX30210_TEMP_Q31_SHIFT 8

static int max30210_decoder_get_frame_count(const uint8_t *buffer,
					    struct sensor_chan_spec chan_spec,
					    uint16_t *frame_count)
{
	const struct max30210_fifo_data *fifo_data = (const struct max30210_fifo_data *)buffer;

	if (chan_spec.chan_type != SENSOR_CHAN_AMBIENT_TEMP || chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	*frame_count = fifo_data->sample_count;
	return 0;
}

static int max30210_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					  size_t *frame_size)
{
	if (chan_spec.chan_type != SENSOR_CHAN_AMBIENT_TEMP || chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}

	*base_size = sizeof(struct sensor_q31_data);
	*frame_size = sizeof(struct sensor_q31_sample_data);
	return 0;
}

static int max30210_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct max30210_fifo_data *fifo_data = (const struct max30210_fifo_data *)buffer;
	const uint8_t *fifo = (const uint8_t *)(fifo_data + 1);
	const uint8_t *sample;
	struct sensor_q31_data *out = (struct sensor_q31_data *)data_out;
	uint16_t count;
	uint64_t timestamp_offset;

	if (chan_spec.chan_type != SENSOR_CHAN_AMBIENT_TEMP || chan_spec.chan_idx != 0) {
		return -ENOTSUP;
	}
	if (max_count == 0) {
		return -EINVAL;
	}
	if (*fit >= fifo_data->sample_count) {
		return -ENODATA;
	}

	timestamp_offset = (uint64_t)(fifo_data->sample_count - 1 - *fit) *
			   fifo_data->sample_period_ns;
	if (timestamp_offset > fifo_data->timestamp_ns) {
		LOG_ERR("FIFO samples predate the timestamp origin");
		return -ERANGE;
	}

	count = MIN(max_count, fifo_data->sample_count - *fit);
	out->header.base_timestamp_ns = fifo_data->timestamp_ns - timestamp_offset;
	out->shift = MAX30210_TEMP_Q31_SHIFT;

	sample = fifo + *fit * MAX30210_BYTES_PER_SAMPLE;
	for (uint16_t i = 0; i < count; i++, sample += MAX30210_BYTES_PER_SAMPLE) {
		int32_t raw_temp = sign_extend(sys_get_be16(sample + 1), 15);
		uint64_t timestamp_delta = (uint64_t)i * fifo_data->sample_period_ns;

		/* Continue long batches on the next decode call with a new timestamp base. */
		if (timestamp_delta > UINT32_MAX) {
			count = i;
			break;
		}

		out->readings[i].timestamp_delta = (uint32_t)timestamp_delta;
		out->readings[i].temperature =
			(q31_t)(((int64_t)raw_temp * (INT64_C(1) <<
						       (31 - MAX30210_TEMP_Q31_SHIFT))) /
				MAX30210_TEMP_COUNTS_PER_C);
	}

	out->header.reading_count = count;
	*fit += count;
	return count;
}

static bool max30210_decoder_has_trigger(const uint8_t *buffer,
					enum sensor_trigger_type trigger)
{
	const struct max30210_fifo_data *fifo_data = (const struct max30210_fifo_data *)buffer;

	switch (trigger) {
	case SENSOR_TRIG_FIFO_FULL:
		return (fifo_data->status & FIFO_FULL_MASK) != 0 &&
		       fifo_data->fifo_count == MAX30210_FIFO_DEPTH;
	case SENSOR_TRIG_FIFO_WATERMARK:
		return (fifo_data->status & FIFO_FULL_MASK) != 0;
	default:
		return false;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = max30210_decoder_get_frame_count,
	.get_size_info = max30210_decoder_get_size_info,
	.decode = max30210_decoder_decode,
	.has_trigger = max30210_decoder_has_trigger,
};

int max30210_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);

	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
