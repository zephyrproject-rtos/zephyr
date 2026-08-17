/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "max30009.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(MAX30009);

/* FIFO sample: TAG[23:20] followed by 20-bit 2's complement DATA[19:0] (datasheet Table 4) */
#define MAX30009_FIFO_TAG_MASK      GENMASK(23, 20)
#define MAX30009_FIFO_DATA_FIELD    GENMASK(19, 0)
#define MAX30009_FIFO_DATA_SIGN_BIT 19
#define MAX30009_BIOZ_I_TAG         0x1
#define MAX30009_BIOZ_Q_TAG         0x2

static inline uint8_t get_channel_tag(struct sensor_chan_spec channel)
{
	return (channel.chan_type == SENSOR_CHAN_BIOZ_I) ? MAX30009_BIOZ_I_TAG
							 : MAX30009_BIOZ_Q_TAG;
}

static int max30009_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec channel,
					    uint16_t *frame_count)
{
#ifndef CONFIG_MAX30009_STREAM
	return -ENOTSUP;
#endif
	const struct max30009_fifo_data *data = (const struct max30009_fifo_data *)buffer;
	uint8_t channel_tag = get_channel_tag(channel);

	if (channel.chan_type != SENSOR_CHAN_BIOZ_I && channel.chan_type != SENSOR_CHAN_BIOZ_Q) {
		return -ENOTSUP;
	}

	buffer += sizeof(struct max30009_fifo_data);
	const uint8_t *buffer_end = buffer + data->fifo_byte_count;
	uint16_t count = 0;

	while (buffer < buffer_end) {
		uint32_t fifo_data = sys_get_be24(buffer);

		if (FIELD_GET(MAX30009_FIFO_TAG_MASK, fifo_data) == channel_tag) {
			count++;
		}
		buffer += MAX30009_FIFO_BYTES_PER_SAMPLE;
	}

	*frame_count = count;
	return 0;
}

static int max30009_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec channel,
				   uint32_t *fit, uint16_t max_count, void *data_out)
{
#ifndef CONFIG_MAX30009_STREAM
	return -ENOTSUP;
#endif
	const struct max30009_fifo_data *data = (const struct max30009_fifo_data *)buffer;
	struct sensor_q31_data *out = (struct sensor_q31_data *)data_out;
	uint8_t channel_tag = get_channel_tag(channel);

	if (channel.chan_type != SENSOR_CHAN_BIOZ_I && channel.chan_type != SENSOR_CHAN_BIOZ_Q) {
		LOG_ERR("Unsupported channel type %d", channel.chan_type);
		return -ENOTSUP;
	}

	uint16_t total_samples = 0;

	if (max30009_decoder_get_frame_count(buffer, channel, &total_samples) != 0) {
		return -ENOTSUP;
	}

	buffer += sizeof(struct max30009_fifo_data);
	const uint8_t *buffer_end = buffer + data->fifo_byte_count;
	int count = 0;
	uint32_t samples_seen = 0;
	uint32_t start_offset = *fit;

	while (count < max_count && buffer < buffer_end) {
		uint32_t fifo_data = sys_get_be24(buffer);

		if (FIELD_GET(MAX30009_FIFO_TAG_MASK, fifo_data) == channel_tag) {
			if (samples_seen >= start_offset) {
				int32_t sample = sign_extend(fifo_data & MAX30009_FIFO_DATA_FIELD,
							     MAX30009_FIFO_DATA_SIGN_BIT);

				/*
				 * timestamp marks the newest sample (FIFO interrupt time), so
				 * back-date each sample by its distance from the newest one.
				 */
				out[count].header.base_timestamp_ns =
					data->timestamp -
					(uint64_t)(total_samples - 1 - samples_seen) *
						data->sample_period_ns;
				out[count].header.reading_count = 1;
				out[count].shift = 0;
				out[count].readings[0].timestamp_delta = 0;
				out[count].readings[0].value = sample;
				count++;
			}
			samples_seen++;
		}
		buffer += MAX30009_FIFO_BYTES_PER_SAMPLE;
	}

	*fit += count;
	return count;
}

static int max30009_decoder_get_size_info(struct sensor_chan_spec channel, size_t *base_size,
					  size_t *frame_size)
{
	__ASSERT_NO_MSG(base_size != NULL);
	__ASSERT_NO_MSG(frame_size != NULL);

	switch ((int)channel.chan_type) {
	case SENSOR_CHAN_BIOZ_I:
	case SENSOR_CHAN_BIOZ_Q:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		LOG_ERR("Unsupported channel type %d", channel.chan_type);
		return -ENOTSUP;
	}
}

static bool max30009_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
#ifndef CONFIG_MAX30009_STREAM
	return false;
#endif

	const struct max30009_fifo_data *fifo_data = (const struct max30009_fifo_data *)buffer;

	switch (trigger) {
	case SENSOR_TRIG_FIFO_FULL:
	case SENSOR_TRIG_FIFO_WATERMARK:
		return FIELD_GET(MAX30009_STATUS1_A_FULL_MSK, fifo_data->status1);
	default:
		return false;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = max30009_decoder_get_frame_count,
	.get_size_info = max30009_decoder_get_size_info,
	.decode = max30009_decoder_decode,
	.has_trigger = max30009_decoder_has_trigger,
};

/**
 * @brief Get the sensor decoder API for MAX30009
 *
 * @param dev Device pointer
 * @param decoder Decoder API pointer
 * @return int 0 on success, negative error code otherwise
 */
int max30009_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}
