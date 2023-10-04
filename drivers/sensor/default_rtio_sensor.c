/*
 * Copyright (c) 2023 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/dsp/types.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensor_compat, CONFIG_SENSOR_LOG_LEVEL);

static void sensor_submit_fallback(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

static void sensor_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	const struct sensor_driver_api *api = dev->api;

	if (api->submit != NULL) {
		api->submit(dev, iodev_sqe);
	} else {
		sensor_submit_fallback(dev, iodev_sqe);
	}
}

const struct rtio_iodev_api __sensor_iodev_api = {
	.submit = sensor_iodev_submit,
};

/**
 * @brief Compute the number of samples needed for the given channels
 *
 * @param[in] channels Array of channels requested
 * @param[in] num_channels Number of channels on the @p channels array
 * @return The number of samples required to read the given channels
 */
static inline int compute_num_samples(const enum sensor_channel *channels, size_t num_channels)
{
	int num_samples = 0;

	for (size_t i = 0; i < num_channels; ++i) {
		num_samples += SENSOR_CHANNEL_3_AXIS(channels[i]) ? 3 : 1;
	}

	return num_samples;
}

/**
 * @brief Compute the minimum number of bytes needed
 *
 * @param[in] num_output_samples The number of samples to represent
 * @return The number of bytes needed for this sample frame
 */
static inline uint32_t compute_min_buf_len(int num_output_samples)
{
	return sizeof(struct sensor_data_generic_header) + (num_output_samples * sizeof(q31_t)) +
	       (num_output_samples * sizeof(enum sensor_channel));
}

/**
 * @brief Checks if the header already contains a given channel
 *
 * @param[in] header The header to scan
 * @param[in] channel The channel to search for
 * @param[in] num_channels The number of valid channels in the header so far
 * @return Index of the @p channel if found or negative if not found
 */
static inline int check_header_contains_channel(const struct sensor_data_generic_header *header,
						enum sensor_channel channel, int num_channels)
{
	__ASSERT_NO_MSG(!SENSOR_CHANNEL_3_AXIS(channel));

	for (int i = 0; i < num_channels; ++i) {
		if (header->channels[i] == channel) {
			return i;
		}
	}
	return -1;
}

/**
 * @brief Fallback function for retrofiting old drivers to rtio
 *
 * @param[in] dev The sensor device to read
 * @param[in] iodev_sqe The read submission queue event
 */
static void sensor_submit_fallback(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const enum sensor_channel *const channels = cfg->channels;
	const int num_output_samples = compute_num_samples(channels, cfg->count);
	uint32_t min_buf_len = compute_min_buf_len(num_output_samples);
	uint64_t timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());
	int rc = sensor_sample_fetch(dev);
	uint8_t *buf;
	uint32_t buf_len;

	/* Check that the fetch succeeded */
	if (rc != 0) {
		LOG_WRN("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_WRN("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	/* Set the timestamp and num_channels */
	struct sensor_data_generic_header *header = (struct sensor_data_generic_header *)buf;

	header->timestamp_ns = timestamp_ns;
	header->num_channels = num_output_samples;
	header->shift = 0;

	q31_t *q = (q31_t *)(buf + sizeof(struct sensor_data_generic_header) +
			     num_output_samples * sizeof(enum sensor_channel));

	/* Populate values, update shift, and set channels */
	for (size_t i = 0, sample_idx = 0; i < cfg->count; ++i) {
		struct sensor_value value[3];
		const int num_samples = SENSOR_CHANNEL_3_AXIS(channels[i]) ? 3 : 1;

		/* Get the current channel requested by the user */
		rc = sensor_channel_get(dev, channels[i], value);

		if (num_samples == 3) {
			header->channels[sample_idx++] =
				rc == 0 ? channels[i] - 3 : SENSOR_CHAN_MAX;
			header->channels[sample_idx++] =
				rc == 0 ? channels[i] - 2 : SENSOR_CHAN_MAX;
			header->channels[sample_idx++] =
				rc == 0 ? channels[i] - 1 : SENSOR_CHAN_MAX;
		} else {
			header->channels[sample_idx++] = rc == 0 ? channels[i] : SENSOR_CHAN_MAX;
		}

		if (rc != 0) {
			LOG_DBG("Failed to get channel %d, skipping", channels[i]);
			continue;
		}

		/* Get the largest absolute value reading to set the scale for the channel */
		uint32_t header_scale = 0;

		for (int sample = 0; sample < num_samples; ++sample) {
			/*
			 * The scale is the ceil(abs(sample)).
			 * Since we are using fractional values, it's easier to assume that .val2
			 *   is non 0 and convert this to abs(sample.val1) + 1 (removing a branch).
			 * Since it's possible that val1 (int32_t) is saturated (INT32_MAX) we need
			 *   to upcast it to 64 bit int first, then take the abs() of that 64 bit
			 *   int before we '+ 1'. Once that's done, we can safely cast back down
			 *   to uint32_t because the min value is 0 and max is INT32_MAX + 1 which
			 *   is less than UINT32_MAX.
			 */
			uint32_t scale = (uint32_t)llabs((int64_t)value[sample].val1) + 1;

			header_scale = MAX(header_scale, scale);
		}

		int8_t new_shift = ilog2(header_scale - 1) + 1;

		/* Reset sample_idx */
		sample_idx -= num_samples;
		if (header->shift < new_shift) {
			/*
			 * Shift was updated, need to convert all the existing q values. This could
			 * be optimized by calling zdsp_scale_q31() but that would force a
			 * dependency between sensors and the zDSP subsystem.
			 */
			for (int q_idx = 0; q_idx < sample_idx; ++q_idx) {
				q[q_idx] = q[q_idx] >> (new_shift - header->shift);
			}
			header->shift = new_shift;
		}

		/*
		 * Spread the q31 values. This is needed because some channels are 3D. If
		 * the user specified one of those then num_samples will be 3; and we need to
		 * produce 3 separate readings.
		 */
		for (int sample = 0; sample < num_samples; ++sample) {
			/* Check if the channel is already in the buffer */
			int prev_computed_value_idx = check_header_contains_channel(
				header, header->channels[sample_idx + sample], sample_idx + sample);

			if (prev_computed_value_idx >= 0 &&
			    prev_computed_value_idx != sample_idx + sample) {
				LOG_DBG("value[%d] previously computed at q[%d]@%p", sample,
					prev_computed_value_idx,
					(void *)&q[prev_computed_value_idx]);
				q[sample_idx + sample] = q[prev_computed_value_idx];
				continue;
			}

			/* Convert the value to micro-units */
			int64_t value_u = sensor_value_to_micro(&value[sample]);

			/* Convert to q31 using the shift */
			q[sample_idx + sample] =
				((value_u * ((INT64_C(1) << 31) - 1)) / 1000000) >> header->shift;

			LOG_DBG("value[%d]=%s%d.%06d, q[%d]@%p=%d", sample, value_u < 0 ? "-" : "",
				abs((int)value[sample].val1), abs((int)value[sample].val2),
				(int)(sample_idx + sample), (void *)&q[sample_idx + sample],
				q[sample_idx + sample]);
		}
		sample_idx += num_samples;
	}
	LOG_DBG("Total channels in header: %zu", header->num_channels);
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void sensor_processing_with_callback(struct rtio *ctx, sensor_processing_callback_t cb)
{
	void *userdata = NULL;
	uint8_t *buf = NULL;
	uint32_t buf_len = 0;
	int rc;

	/* Wait for a CQE */
	struct rtio_cqe *cqe = rtio_cqe_consume_block(ctx);

	/* Cache the data from the CQE */
	rc = cqe->result;
	userdata = cqe->userdata;
	rtio_cqe_get_mempool_buffer(ctx, cqe, &buf, &buf_len);

	/* Release the CQE */
	rtio_cqe_release(ctx, cqe);

	/* Call the callback */
	cb(rc, buf, buf_len, userdata);

	/* Release the memory */
	rtio_release_buffer(ctx, buf, buf_len);
}

/**
 * @brief Default decoder get frame count
 *
 * Default reader can only ever service a single frame at a time.
 *
 * @param[in]  buffer The data buffer to parse
 * @param[out] frame_count The number of frames in the buffer (always 1)
 * @return 0 in all cases
 */
static int get_frame_count(const uint8_t *buffer, uint16_t *frame_count)
{
	ARG_UNUSED(buffer);
	*frame_count = 1;
	return 0;
}

/**
 * @brief Default decoder get the timestamp of the first frame
 *
 * @param[in]  buffer The data buffer to parse
 * @param[out] timestamp_ns The timestamp of the first frame
 * @return 0 in all cases
 */
static int get_timestamp(const uint8_t *buffer, uint64_t *timestamp_ns)
{
	*timestamp_ns = ((struct sensor_data_generic_header *)buffer)->timestamp_ns;
	return 0;
}

/**
 * @brief Default decoder get the bitshift of the given channel (if possible)
 *
 * @param[in]  buffer The data buffer to parse
 * @param[in]  channel_type The channel to query
 * @param[out] shift The bitshift for the q31 value
 * @return 0 on success
 * @return -EINVAL if the @p channel_type couldn't be found
 */
static int get_shift(const uint8_t *buffer, enum sensor_channel channel_type, int8_t *shift)
{
	struct sensor_data_generic_header *header = (struct sensor_data_generic_header *)buffer;

	ARG_UNUSED(channel_type);
	*shift = header->shift;
	return 0;
}

/**
 * @brief Default decoder decode N samples
 *
 * Decode up to N samples starting at the provided @p fit and @p cit. The appropriate channel types
 * and q31 values will be placed in @p values and @p channels respectively.
 *
 * @param[in]     buffer The data buffer to decode
 * @param[in,out] fit The starting frame iterator
 * @param[in,out] cit The starting channel iterator
 * @param[out]    channels The decoded channel types
 * @param[out]    values The decoded q31 values
 * @param[in]     max_count The maximum number of values to decode
 * @return > 0 The number of decoded values
 * @return 0 Nothing else to decode on this @p buffer
 * @return < 0 Error
 */
static int decode(const uint8_t *buffer, sensor_frame_iterator_t *fit,
		  sensor_channel_iterator_t *cit, enum sensor_channel *channels, q31_t *values,
		  uint8_t max_count)
{
	const struct sensor_data_generic_header *header =
		(const struct sensor_data_generic_header *)buffer;
	const q31_t *q =
		(const q31_t *)(buffer + sizeof(struct sensor_data_generic_header) +
				header->num_channels * sizeof(enum sensor_channel));
	int count = 0;

	if (*fit != 0 || *cit >= header->num_channels) {
		return -EINVAL;
	}

	/* Skip invalid channels */
	while (*cit < header->num_channels && header->channels[*cit] == SENSOR_CHAN_MAX) {
		*cit += 1;
	}

	for (; *cit < header->num_channels && count < max_count; ++count) {
		channels[count] = header->channels[*cit];
		values[count] = q[*cit];
		LOG_DBG("Decoding q[%u]@%p=%d", *cit, (void *)&q[*cit], q[*cit]);
		*cit += 1;
	}

	if (*cit >= header->num_channels) {
		*fit = 1;
		*cit = 0;
	}
	return count;
}

const struct sensor_decoder_api __sensor_default_decoder = {
	.get_frame_count = get_frame_count,
	.get_timestamp = get_timestamp,
	.get_shift = get_shift,
	.decode = decode,
};
