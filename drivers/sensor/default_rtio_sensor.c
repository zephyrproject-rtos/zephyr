/*
 * Copyright (c) 2023 Google LLC.
 * Copyright (c) 2024 Croxel Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/dsp/types.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/work.h>

LOG_MODULE_REGISTER(sensor_compat, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Ensure that the size of the generic header aligns with the sensor channel specifier . If it
 * doesn't, then cores that require aligned memory access will fail to read channel[0].
 */
BUILD_ASSERT((sizeof(struct sensor_data_generic_header) % sizeof(struct sensor_chan_spec)) == 0);

static void sensor_submit_fallback(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);

static void sensor_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	const struct sensor_driver_api *api = dev->api;

	if (api->submit != NULL) {
		api->submit(dev, iodev_sqe);
	} else if (!cfg->is_streaming) {
		sensor_submit_fallback(dev, iodev_sqe);
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
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
static inline int compute_num_samples(const struct sensor_chan_spec *const channels,
				      size_t num_channels)
{
	int num_samples = 0;

	for (size_t i = 0; i < num_channels; ++i) {
		num_samples += SENSOR_CHANNEL_3_AXIS(channels[i].chan_type) ? 3 : 1;
	}

	return num_samples;
}

/**
 * @brief Compute the required header size
 *
 * This function takes into account alignment of the q31 values that will follow the header.
 *
 * @param[in] num_output_samples The number of samples to represent
 * @return The number of bytes needed for this sample frame's header
 */
static inline uint32_t compute_header_size(int num_output_samples)
{
	uint32_t size = sizeof(struct sensor_data_generic_header) +
			(num_output_samples * sizeof(struct sensor_chan_spec));
	return (size + 3) & ~0x3;
}

/**
 * @brief Compute the minimum number of bytes needed
 *
 * @param[in] num_output_samples The number of samples to represent
 * @return The number of bytes needed for this sample frame
 */
static inline uint32_t compute_min_buf_len(int num_output_samples)
{
	return compute_header_size(num_output_samples) + (num_output_samples * sizeof(q31_t));
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
						struct sensor_chan_spec chan_spec, int num_channels)
{
	__ASSERT_NO_MSG(!SENSOR_CHANNEL_3_AXIS(chan_spec.chan_type));

	for (int i = 0; i < num_channels; ++i) {
		if (sensor_chan_spec_eq(header->channels[i], chan_spec)) {
			return i;
		}
	}
	return -1;
}

/**
 * @brief Fallback function for retrofiting old drivers to rtio (sync)
 *
 * @param[in] iodev_sqe The read submission queue event
 */
static void sensor_submit_fallback_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	const struct sensor_chan_spec *const channels = cfg->channels;
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

	q31_t *q = (q31_t *)(buf + compute_header_size(num_output_samples));

	/* Populate values, update shift, and set channels */
	for (size_t i = 0, sample_idx = 0; i < cfg->count; ++i) {
		struct sensor_value value[3];
		const int num_samples = SENSOR_CHANNEL_3_AXIS(channels[i].chan_type) ? 3 : 1;

		/* Get the current channel requested by the user */
		rc = sensor_channel_get(dev, channels[i].chan_type, value);

		if (num_samples == 3) {
			header->channels[sample_idx++] = (struct sensor_chan_spec) {
				rc == 0 ? channels[i].chan_type - 3 : SENSOR_CHAN_MAX,
				0
			};
			header->channels[sample_idx++] = (struct sensor_chan_spec) {
				rc == 0 ? channels[i].chan_type - 2 : SENSOR_CHAN_MAX,
				0
			};
			header->channels[sample_idx++] = (struct sensor_chan_spec) {
				rc == 0 ? channels[i].chan_type - 1 : SENSOR_CHAN_MAX,
				0
			};
		} else {
			header->channels[sample_idx++] = (struct sensor_chan_spec) {
				rc == 0 ? channels[i].chan_type : SENSOR_CHAN_MAX,
				0
			};
		}

		if (rc != 0) {
			LOG_DBG("Failed to get channel (type: %d, index %d), skipping",
				channels[i].chan_type, channels[i].chan_idx);
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

			LOG_DBG("value[%d]=%s%d.%06d, q[%d]@%p=%d, shift: %d",
				sample, value_u < 0 ? "-" : "",
				abs((int)value[sample].val1), abs((int)value[sample].val2),
				(int)(sample_idx + sample), (void *)&q[sample_idx + sample],
				q[sample_idx + sample], header->shift);
		}
		sample_idx += num_samples;
	}
	LOG_DBG("Total channels in header: %" PRIu32, header->num_channels);
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

/**
 * @brief Fallback function for retrofiting old drivers to rtio
 *
 * @param[in] dev The sensor device to read
 * @param[in] iodev_sqe The read submission queue event
 */
static void sensor_submit_fallback(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed. Consider to increase "
			"CONFIG_RTIO_WORKQ_POOL_ITEMS.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, sensor_submit_fallback_sync);
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
 * @param[in]  channel The channel to get the count for
 * @param[in]  channel_idx The index of the channel
 * @param[out] frame_count The number of frames in the buffer (always 1)
 * @return 0 in all cases
 */
static int get_frame_count(const uint8_t *buffer, struct sensor_chan_spec channel,
			   uint16_t *frame_count)
{
	struct sensor_data_generic_header *header = (struct sensor_data_generic_header *)buffer;

	switch (channel.chan_type) {
	case SENSOR_CHAN_ACCEL_XYZ:
		channel.chan_type = SENSOR_CHAN_ACCEL_X;
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		channel.chan_type = SENSOR_CHAN_GYRO_X;
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		channel.chan_type = SENSOR_CHAN_MAGN_X;
		break;
	case SENSOR_CHAN_POS_DXYZ:
		channel.chan_type = SENSOR_CHAN_POS_DX;
		break;
	default:
		break;
	}
	for (size_t i = 0; i < header->num_channels; ++i) {
		if (sensor_chan_spec_eq(header->channels[i], channel)) {
			*frame_count = 1;
			return 0;
		}
	}

	return -ENOTSUP;
}

int sensor_natively_supported_channel_size_info(struct sensor_chan_spec channel, size_t *base_size,
						size_t *frame_size)
{
	__ASSERT_NO_MSG(base_size != NULL);
	__ASSERT_NO_MSG(frame_size != NULL);

	if (((int)channel.chan_type < 0) || channel.chan_type >= (SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	switch (channel.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
	case SENSOR_CHAN_POS_DX:
	case SENSOR_CHAN_POS_DY:
	case SENSOR_CHAN_POS_DZ:
	case SENSOR_CHAN_POS_DXYZ:
		*base_size = sizeof(struct sensor_three_axis_data);
		*frame_size = sizeof(struct sensor_three_axis_sample_data);
		return 0;
	case SENSOR_CHAN_PROX:
		*base_size = sizeof(struct sensor_byte_data);
		*frame_size = sizeof(struct sensor_byte_sample_data);
		return 0;
	case SENSOR_CHAN_GAUGE_CYCLE_COUNT:
		*base_size = sizeof(struct sensor_uint64_data);
		*frame_size = sizeof(struct sensor_uint64_sample_data);
		return 0;
	default:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	}
}

static int get_q31_value(const struct sensor_data_generic_header *header, const q31_t *values,
			 struct sensor_chan_spec chan_spec, q31_t *out)
{
	for (size_t i = 0; i < header->num_channels; ++i) {
		if (sensor_chan_spec_eq(chan_spec, header->channels[i])) {
			*out = values[i];
			return 0;
		}
	}
	return -EINVAL;
}

static int decode_three_axis(const struct sensor_data_generic_header *header, const q31_t *values,
			     struct sensor_three_axis_data *data_out, enum sensor_channel x,
			     enum sensor_channel y, enum sensor_channel z, size_t channel_idx)
{
	int rc;

	data_out->header.base_timestamp_ns = header->timestamp_ns;
	data_out->header.reading_count = 1;
	data_out->shift = header->shift;
	data_out->readings[0].timestamp_delta = 0;

	rc = get_q31_value(header, values, (struct sensor_chan_spec){x, channel_idx},
			   &data_out->readings[0].values[0]);
	if (rc < 0) {
		return rc;
	}
	rc = get_q31_value(header, values, (struct sensor_chan_spec){y, channel_idx},
			   &data_out->readings[0].values[1]);
	if (rc < 0) {
		return rc;
	}
	rc = get_q31_value(header, values, (struct sensor_chan_spec){z, channel_idx},
			   &data_out->readings[0].values[2]);
	if (rc < 0) {
		return rc;
	}
	return 1;
}

static int decode_q31(const struct sensor_data_generic_header *header, const q31_t *values,
		      struct sensor_q31_data *data_out, struct sensor_chan_spec chan_spec)
{
	int rc;

	data_out->header.base_timestamp_ns = header->timestamp_ns;
	data_out->header.reading_count = 1;
	data_out->shift = header->shift;
	data_out->readings[0].timestamp_delta = 0;

	rc = get_q31_value(header, values, chan_spec, &data_out->readings[0].value);
	if (rc < 0) {
		return rc;
	}
	return 1;
}

/**
 * @brief Decode up to N samples from the buffer
 *
 * This function will never wrap frames. If 1 channel is available in the current frame and
 * @p max_count is 2, only 1 channel will be decoded and the frame iterator will be modified
 * so that the next call to decode will begin at the next frame.
 *
 * @param[in]     buffer The buffer provided on the :c:struct:`rtio` context
 * @param[in]     channel The channel to decode
 * @param[in]     channel_idx The index of the channel
 * @param[in,out] fit The current frame iterator
 * @param[in]     max_count The maximum number of channels to decode.
 * @param[out]    data_out The decoded data
 * @return 0 no more samples to decode
 * @return >0 the number of decoded frames
 * @return <0 on error
 */
static int decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
		  uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct sensor_data_generic_header *header =
		(const struct sensor_data_generic_header *)buffer;
	const q31_t *q = (const q31_t *)(buffer + compute_header_size(header->num_channels));
	int count = 0;

	if (*fit != 0 || max_count < 1) {
		return -EINVAL;
	}

	if (((int)chan_spec.chan_type < 0) || chan_spec.chan_type >= (SENSOR_CHAN_ALL)) {
		return 0;
	}

	/* Check for 3d channel mappings */
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		count = decode_three_axis(header, q, data_out, SENSOR_CHAN_ACCEL_X,
					  SENSOR_CHAN_ACCEL_Y, SENSOR_CHAN_ACCEL_Z,
					  chan_spec.chan_idx);
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		count = decode_three_axis(header, q, data_out, SENSOR_CHAN_GYRO_X,
					  SENSOR_CHAN_GYRO_Y, SENSOR_CHAN_GYRO_Z,
					  chan_spec.chan_idx);
		break;
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		count = decode_three_axis(header, q, data_out, SENSOR_CHAN_MAGN_X,
					  SENSOR_CHAN_MAGN_Y, SENSOR_CHAN_MAGN_Z,
					  chan_spec.chan_idx);
		break;
	case SENSOR_CHAN_POS_DX:
	case SENSOR_CHAN_POS_DY:
	case SENSOR_CHAN_POS_DZ:
	case SENSOR_CHAN_POS_DXYZ:
		count = decode_three_axis(header, q, data_out, SENSOR_CHAN_POS_DX,
					  SENSOR_CHAN_POS_DY, SENSOR_CHAN_POS_DZ,
					  chan_spec.chan_idx);
		break;
	default:
		count = decode_q31(header, q, data_out, chan_spec);
		break;
	}
	if (count > 0) {
		*fit = 1;
	}
	return count;
}

const struct sensor_decoder_api __sensor_default_decoder = {
	.get_frame_count = get_frame_count,
	.get_size_info = sensor_natively_supported_channel_size_info,
	.decode = decode,
};
