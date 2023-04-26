/*
 * Copyright (c) 2023 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/dsp/types.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rtio_sensor, CONFIG_SENSOR_LOG_LEVEL);

static void sensor_submit_fallback(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

static void sensor_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe->iodev->data;
	const struct device *dev = cfg->sensor;
	const struct sensor_driver_api *api = dev->api;

	if (api->submit != NULL) {
		api->submit(dev, iodev_sqe);
	} else {
		sensor_submit_fallback(dev, iodev_sqe);
	}
}

struct rtio_iodev_api __sensor_iodev_api = {
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
static inline uint32_t compute_min_buf_len(size_t num_channels, int num_output_samples)
{
	return sizeof(struct sensor_data_generic_header) + (num_output_samples * sizeof(q31_t)) +
	       (num_channels * sizeof(struct sensor_data_generic_channel));
}

/**
 * @brief Fallback function for retrofiting old drivers to rtio
 *
 * @param[in] dev The sensor device to read
 * @param[in] iodev_sqe The read submission queue event
 */
static void sensor_submit_fallback(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe->iodev->data;
	const enum sensor_channel *const channels = cfg->channels;
	const size_t num_channels = cfg->num_channels;
	int num_output_samples = compute_num_samples(channels, num_channels);
	uint32_t min_buf_len = compute_min_buf_len(num_channels, num_output_samples);
	uint64_t timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());
	int rc = sensor_sample_fetch(dev);
	uint8_t *buf;
	uint32_t buf_len;

	/* Check that the fetch succeeded */
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	/* Get the buffer for the frame, it may be allocated dynamically by the rtio context */
	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	/* Set the timestamp and num_channels */
	struct sensor_data_generic_header *header = (struct sensor_data_generic_header *)buf;

	header->timestamp_ns = timestamp_ns;
	header->num_channels = num_channels;

	uint8_t current_channel_index = 0;
	q31_t *q = (q31_t *)(buf + sizeof(struct sensor_data_generic_header) +
			     num_channels * sizeof(struct sensor_data_generic_channel));

	for (size_t i = 0; i < num_channels; ++i) {
		struct sensor_value value[3];
		int num_samples = SENSOR_CHANNEL_3_AXIS(channels[i]) ? 3 : 1;

		/* Get the current channel requested by the user */
		rc = sensor_channel_get(dev, channels[i], value);
		if (rc != 0) {
			LOG_ERR("Failed to get channel %d", channels[i]);
			rtio_iodev_sqe_err(iodev_sqe, rc);
			return;
		}

		/* Get the largest absolute value reading to set the scale for the channel */
		uint32_t header_scale = 0;

		for (int sample = 0; sample < num_samples; ++sample) {
			uint32_t scale = (uint32_t)llabs((int64_t)value[sample].val1 + 1);

			header_scale = MAX(header_scale, scale);
		}
		header->channel_info[i].channel = channels[i];
		header->channel_info[i].shift = ilog2(header_scale - 1) + 1;

		/*
		 * Spread the q31 values. This is needed because some channels are 3D. If
		 * the user specified one of those then num_samples will be 3; and we need to
		 * produce 3 separate readings.
		 */
		for (int sample = 0; sample < num_samples; ++sample) {
			/* Convert the value to micro-units */
			int64_t value_u =
				value[sample].val1 * INT64_C(1000000) + value[sample].val2;

			/* Convert to q31 using the shift */
			q[current_channel_index + sample] =
				((value_u * ((INT64_C(1) << 31) - 1)) / 1000000) >>
				header->channel_info[i].shift;

			LOG_DBG("value[%d]=%s%d.%06d, q[%d]=%d", sample, value_u < 0 ? "-" : "",
				abs((int)(value_u / 1000000)), abs((int)(value_u % 1000000)),
				current_channel_index + sample, q[current_channel_index + sample]);
		}
		current_channel_index += num_samples;
	}
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void z_sensor_processing_loop(struct rtio *ctx, sensor_processing_callback_t cb)
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
	rtio_cqe_release(ctx);

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
 * @brief Check if @p needle is a subchannel of @p haystack
 *
 * Returns true when:
 * - @p haystack and @p needle are the same
 * - @p haystack is a 3D channel and @p needle is a subchannel. For example: @p haystack is
 *   :c:enum:`SENSOR_CHAN_ACCEL_XYZ` and @p needle is :c:enum:`SENSOR_CHAN_ACCEL_X`.
 *
 * @param needle The channel to search
 * @param haystack The potential superset of channels.
 * @return true When @p needle can be found in @p haystack
 * @return false Otherwise
 */
static inline bool is_subchannel(enum sensor_channel needle, enum sensor_channel haystack)
{
	if (needle == haystack) {
		return true;
	}
	if (!SENSOR_CHANNEL_3_AXIS(haystack)) {
		return false;
	}
	return (haystack == SENSOR_CHAN_ACCEL_XYZ &&
		(needle == SENSOR_CHAN_ACCEL_X || needle == SENSOR_CHAN_ACCEL_Y ||
		 needle == SENSOR_CHAN_ACCEL_Z)) ||
	       (haystack == SENSOR_CHAN_GYRO_XYZ &&
		(needle == SENSOR_CHAN_GYRO_X || needle == SENSOR_CHAN_GYRO_Y ||
		 needle == SENSOR_CHAN_GYRO_Z)) ||
	       (haystack == SENSOR_CHAN_MAGN_XYZ &&
		(needle == SENSOR_CHAN_MAGN_X || needle == SENSOR_CHAN_MAGN_Y ||
		 needle == SENSOR_CHAN_MAGN_Z));
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

	for (uint8_t i = 0; i < header->num_channels; ++i) {
		if (is_subchannel(channel_type, header->channel_info[i].channel)) {
			*shift = header->channel_info[i].shift;
			return 0;
		}
	}
	return -EINVAL;
}

/**
 * @brief Compute the channel_info index and offset in the header
 *
 * This is a helper function for decoding which allows mapping the current channel iterator @p cit
 * to the current channel_info object in the header along with a potential offset if the channel is
 * a 3D channel.
 *
 * @param[in]  header The data header to parse
 * @param[in]  cit The channel iterator where decoding is starting
 * @param[out] index The index of the @p header's channel_info matching the @p cit
 * @param[out] offset the offset of the channel value if the channel value is a 3D channel,
 *             0 otherwise.
 * @return 0 on success
 * @return -EINVAL if the index couldn't be computed
 */
static int compute_channel_info_index(const struct sensor_data_generic_header *header,
				      sensor_channel_iterator_t cit, int *index, int *offset)
{
	sensor_channel_iterator_t i = { 0 };

	*index = 0;
	do {
		sensor_channel_iterator_t last_i = i;

		i += SENSOR_CHANNEL_3_AXIS(header->channel_info[*index].channel) ? 3 : 1;

		if (i > cit) {
			*offset = cit - last_i;
			return 0;
		}
		*index += 1;
	} while (*index < header->num_channels);

	return -EINVAL;
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
	int num_samples = 0;
	const q31_t *q =
		(const q31_t *)(buffer + sizeof(struct sensor_data_generic_header) +
				header->num_channels * sizeof(struct sensor_data_generic_channel));

	if (*fit != 0) {
		return -EINVAL;
	}

	for (int i = 0; i < header->num_channels; ++i) {
		num_samples += SENSOR_CHANNEL_3_AXIS(header->channel_info[i].channel) ? 3 : 1;
	}

	int count = 0;
	int channel_info_index;
	int offset;

	if (compute_channel_info_index(header, *cit, &channel_info_index, &offset) != 0) {
		LOG_ERR("Could not compute the channel index");
		return -EINVAL;
	}

	for (; *cit < num_samples && count < max_count; ++count) {
		enum sensor_channel channel = header->channel_info[channel_info_index].channel;

		channels[count] = channel;
		if (SENSOR_CHANNEL_3_AXIS(channel)) {
			channels[count] = channel - 3 + offset;
			offset++;
			if (offset == 3) {
				channel_info_index++;
				offset = 0;
			}
		} else {
			channel_info_index++;
		}
		values[count] = q[*cit];
		*cit += 1;
	}

	if (*cit >= num_samples) {
		*fit += 1;
		*cit = 0;
	}
	return count;
}

struct sensor_decoder_api __sensor_default_decoder = {
	.get_frame_count = get_frame_count,
	.get_timestamp = get_timestamp,
	.get_shift = get_shift,
	.decode = decode,
};
