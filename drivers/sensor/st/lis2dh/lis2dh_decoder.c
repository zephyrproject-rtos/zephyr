/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2dh

#include <errno.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>

#include "lis2dh_decoder.h"

static int lis2dh_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
					  uint16_t *frame_count)
{
	const struct lis2dh_encoded_header *header = (const struct lis2dh_encoded_header *)buffer;

	if (buffer == NULL || chan_spec.chan_idx != 0U || frame_count == NULL) {
		return -EINVAL;
	}

	if (chan_spec.chan_type != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	if (header->sample_count > 32U) {
		return -EINVAL;
	}
	*frame_count = header->sample_count;
	return 0;
}

static q31_t lis2dh_decode_accel(int16_t raw, uint32_t scale, int8_t shift)
{
	int64_t micro_ms2 = (int64_t)(raw >> 4) * scale;

	return (q31_t)((micro_ms2 * (INT64_C(1) << 31)) /
		       (INT64_C(1000000) * (INT64_C(1) << shift)));
}

static int lis2dh_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				 uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct lis2dh_encoded_header *header = (const struct lis2dh_encoded_header *)buffer;
	const uint8_t *samples;
	struct sensor_three_axis_data *data = data_out;
	uint16_t count;
	uint16_t i;

	if (buffer == NULL || chan_spec.chan_idx != 0U || fit == NULL || data_out == NULL) {
		return -EINVAL;
	}

	if (chan_spec.chan_type != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	if (header->sample_count > 32U || header->shift < 0 || header->shift > 31 ||
	    header->scale > 1000000U) {
		return -EINVAL;
	}
	samples = buffer + sizeof(*header);
	if (*fit >= header->sample_count || max_count == 0U) {
		return 0;
	}

	count = MIN(max_count, header->sample_count - *fit);
	/* Zephyr deltas are uint32_t nanoseconds: split slow-ODR batches. */
	if (header->period_ns != 0U) {
		count = MIN(count, UINT32_MAX / header->period_ns + 1U);
	}
	memset(data, 0, sizeof(*data) + (count - 1U) * sizeof(data->readings[0]));
	data->header.base_timestamp_ns =
		header->timestamp_ns - (header->sample_count - 1U - *fit) * header->period_ns;
	data->header.reading_count = count;
	data->shift = header->shift;

	for (i = 0U; i < count; i++) {
		const uint8_t *sample = samples + (*fit + i) * LIS2DH_ENCODED_SAMPLE_SIZE;

		data->readings[i].timestamp_delta = i * header->period_ns;
		data->readings[i].x = lis2dh_decode_accel((int16_t)sys_get_le16(sample),
							  header->scale, header->shift);
		data->readings[i].y = lis2dh_decode_accel((int16_t)sys_get_le16(sample + 2U),
							  header->scale, header->shift);
		data->readings[i].z = lis2dh_decode_accel((int16_t)sys_get_le16(sample + 4U),
							  header->scale, header->shift);
	}

	*fit += count;
	return count;
}

static bool lis2dh_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct lis2dh_encoded_header *header = (const struct lis2dh_encoded_header *)buffer;

	return buffer != NULL && header->is_fifo != 0U && header->trigger == trigger;
}

static int lis2dh_decoder_get_size_info(struct sensor_chan_spec channel, size_t *base_size,
					size_t *frame_size)
{
	if (base_size == NULL || frame_size == NULL || channel.chan_idx != 0U) {
		return -EINVAL;
	}

	if (channel.chan_type != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	*base_size = sizeof(struct sensor_three_axis_data);
	*frame_size = sizeof(struct sensor_three_axis_sample_data);
	return 0;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = lis2dh_decoder_get_frame_count,
	.decode = lis2dh_decoder_decode,
	.has_trigger = lis2dh_decoder_has_trigger,
	.get_size_info = lis2dh_decoder_get_size_info,
};

int lis2dh_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	if (decoder == NULL) {
		return -EINVAL;
	}
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}
