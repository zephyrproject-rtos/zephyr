/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Decoder for the Sensirion SGP41 sensor driver.
 *
 * Implements sensor_decoder_api to decode raw RTIO buffers into
 * sensor_q31_data frames for VOC and NOx channels.
 */

#include <zephyr/drivers/sensor.h>
#include "sgp41.h"

/**
 * @brief Return the number of available frames for a given channel.
 *
 * @param buf       Encoded data buffer from the RTIO context.
 * @param chan      Channel specification to query.
 * @param count     Output: number of frames available (0 or 1).
 * @retval 0        Success.
 * @retval -ENOTSUP Channel index is non-zero or channel type is unsupported.
 */
static int sgp41_dec_frame_count(const uint8_t *buf, struct sensor_chan_spec chan, uint16_t *count)
{
	const struct sgp41_encoded_data *e = (const void *)buf;

	if (chan.chan_idx != 0) {
		return -ENOTSUP;
	}
	switch (chan.chan_type) {
	case SENSOR_CHAN_GAS_RES:
		*count = e->has_voc;
		return 0;
	case SGP41_CHAN_NOX:
		*count = e->has_nox;
		return 0;
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Return the buffer sizes needed to decode a channel.
 *
 * For a single frame: allocate @p base_size bytes.
 * For N frames: allocate base_size + (N - 1) * frame_size bytes.
 *
 * @param chan       Channel specification to query.
 * @param base_size  Output: size in bytes for the first decoded frame.
 * @param frame_size Output: additional size in bytes for each extra frame.
 * @retval 0        Success.
 * @retval -ENOTSUP Channel type is unsupported.
 */
static int sgp41_dec_size_info(struct sensor_chan_spec chan, size_t *base_size, size_t *frame_size)
{
	switch (chan.chan_type) {
	case SENSOR_CHAN_GAS_RES:
	case SGP41_CHAN_NOX:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Decode up to @p max_count frames for the specified channel.
 *
 * The SGP41 produces at most one frame per measurement, so this function
 * decodes exactly one frame when *fit == 0 and the requested channel is
 * present in the buffer.
 *
 * @param buf       Encoded data buffer from the RTIO context.
 * @param chan      Channel specification to decode.
 * @param fit       Frame iterator; must be 0 on first call, set to 1 on return.
 * @param max_count Maximum number of frames to decode (unused; always <= 1).
 * @param data_out  Output buffer; must be at least sizeof(struct sensor_q31_data).
 * @retval 1        One frame was decoded successfully.
 * @retval 0        No more frames available (*fit was already non-zero).
 * @retval -ENOTSUP Channel index is non-zero or channel type is unsupported.
 * @retval -ENODATA Requested channel has no valid reading in this buffer.
 */
static int sgp41_dec_decode(const uint8_t *buf, struct sensor_chan_spec chan, uint32_t *fit,
			    uint16_t max_count, void *data_out)
{
	const struct sgp41_encoded_data *e = (const void *)buf;
	struct sensor_q31_data *out = data_out;
	uint16_t raw;

	ARG_UNUSED(max_count);

	if (*fit != 0) {
		return 0;
	}
	if (chan.chan_idx != 0) {
		return -ENOTSUP;
	}
	if (chan.chan_type == SENSOR_CHAN_GAS_RES && e->has_voc) {
		raw = e->sraw_voc;
	} else if (chan.chan_type == SGP41_CHAN_NOX && e->has_nox) {
		raw = e->sraw_nox;
	} else {
		return -ENODATA;
	}

	out->header.base_timestamp_ns = e->timestamp;
	out->header.reading_count = 1;
	out->shift = SGP41_RAW_SHIFT;
	out->readings[0].timestamp_delta = 0;
	out->readings[0].resistance = (int32_t)((int64_t)raw << (31 - SGP41_RAW_SHIFT));

	*fit = 1;
	return 1;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = sgp41_dec_frame_count,
	.get_size_info = sgp41_dec_size_info,
	.decode = sgp41_dec_decode,
	.has_trigger = NULL, /* SGP41 does not support triggers */
};

int sgp41_get_decoder(const struct device *dev, const struct sensor_decoder_api **api)
{
	ARG_UNUSED(dev);
	*api = &SENSOR_DECODER_NAME();
	return 0;
}
