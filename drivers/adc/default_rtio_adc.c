/*
 * Copyright (c) 2023 Google LLC.
 * Copyright (c) 2024 Croxel Inc.
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/work.h>

LOG_MODULE_REGISTER(adc_compat, CONFIG_ADC_LOG_LEVEL);

#if CONFIG_RTIO_WORKQ
static void adc_submit_fallback(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
#endif /* CONFIG_RTIO_WORKQ */

static void adc_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct adc_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->adc;
	const struct adc_driver_api *api = dev->api;

	if (api->submit != NULL) {
		api->submit(dev, iodev_sqe);
#if CONFIG_RTIO_WORKQ
	} else if (!cfg->is_streaming) {
		adc_submit_fallback(dev, iodev_sqe);
#endif /* CONFIG_RTIO_WORKQ */
	} else {
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

const struct rtio_iodev_api __adc_iodev_api = {
	.submit = adc_iodev_submit,
};

/**
 * @brief Compute the required header size
 *
 * This function takes into account alignment of the q31 values that will follow the header.
 *
 * @param[in] num_output_samples The number of samples to represent
 * @return The number of bytes needed for this sample frame's header
 */
static inline uint32_t compute_read_buf_size(const struct adc_dt_spec *adc_spec, int num_channels)
{
	uint32_t size = 0;

	for (int i = 0; i < num_channels; ++i) {
		size += adc_spec[i].resolution / 8;
		if (adc_spec[i].resolution % 8) {
			size++;
		}
	}

	/* Align to 4 bytes */
	if (size % 4) {
		size += 4 - (size % 4);
	}

	return size;
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
	uint32_t size = sizeof(struct adc_data_generic_header) +
			(num_output_samples * sizeof(struct adc_chan_spec));
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
 * @brief Convert sample to q31_t format
 *
 * @param[in] out Pointer to the output q31_t value
 * @param[in] data_in The input data to convert
 * @param[in] channel The ADC channel specification
 * @param[in] adc_shift The shift value for the ADC
 */
static inline void adc_convert_q31(q31_t *out, uint64_t data_in,
			const struct adc_dt_spec *adc_spec, uint8_t adc_shift)
{
	uint32_t scale = BIT(adc_spec->resolution);
	uint8_t data_size = adc_spec->resolution / 8;

	if (adc_spec->resolution % 8) {
		data_size++;
	}

	/* In Differential mode, 1 bit is used for sign */
	if (adc_spec->channel_cfg.differential) {
		scale = BIT(adc_spec->resolution - 1);
	}

	uint32_t sensitivity = (adc_spec->vref_mv * (scale - 1)) / scale
				* 1000 / scale; /* uV / LSB */

	*out = BIT(31 - adc_shift)/* scaling to q_31*/ * sensitivity / 1000000/*uV to V*/ * data_in;
}

/**
 * @brief Compute the number of bits needed to represent the vref_mv
 *
 * @param[in] vref_mv The reference voltage in mV
 * @return The number of bits needed to represent the vref_mv
 */
uint8_t adc_convert_vref_to_shift(uint16_t vref_mv)
{
	uint8_t count = 1;

	while (1) {
		vref_mv /= 2;
		if (vref_mv) {
			count++;
		} else {
			break;
		}
	}
	return count;
}

#if CONFIG_RTIO_WORKQ
/**
 * @brief Fallback function for retrofiting old drivers to rtio (sync)
 *
 * @param[in] iodev_sqe The read submission queue event
 */
static void adc_submit_fallback_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct adc_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->adc;
	const struct adc_dt_spec *adc_spec = cfg->adc_spec;
	const int num_output_samples = cfg->adc_spec_cnt;
	uint32_t min_buf_len = compute_min_buf_len(num_output_samples);
	uint64_t timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());
	uint8_t read_buf_size = compute_read_buf_size(adc_spec, num_output_samples);
	uint8_t sample_buffer[read_buf_size];
	struct adc_sequence sequence = {
		.buffer = sample_buffer,
		.buffer_size = read_buf_size,
	};
	int rc = adc_read(dev, &sequence);

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
	struct adc_data_generic_header *header = (struct adc_data_generic_header *)buf;

	header->timestamp_ns = timestamp_ns;
	header->num_channels = num_output_samples;
	header->shift = 0;

	q31_t *q = (q31_t *)(buf + compute_header_size(num_output_samples));
	uint8_t *sample_pointer = sample_buffer;

	/* Populate values, update shift, and set channels */
	for (size_t i = 0; i < num_output_samples; ++i) {
		uint8_t sample_size = adc_spec[i].resolution / 8;

		if (adc_spec[i].resolution % 8) {
			sample_size++;
		}

		uint64_t sample = 0;

		memcpy(&sample, sample_pointer, sample_size);
		sample_pointer += sample_size;
		if ((adc_spec[i].channel_cfg.differential) &&
			(sample & (BIT(adc_spec[i].resolution - 1)))) {
			sample |= ~BIT_MASK(adc_spec[i].resolution);
		}

		header->channels[i].chan_idx = adc_spec[i].channel_id;
		header->channels[i].chan_resolution = adc_spec[i].resolution;

		int8_t new_shift = adc_convert_vref_to_shift(adc_spec[i].vref_mv);

		if (header->shift < new_shift) {
			/*
			 * Shift was updated, need to convert all the existing q values. This could
			 * be optimized by calling zdsp_scale_q31() but that would force a
			 * dependency between sensors and the zDSP subsystem.
			 */
			for (int q_idx = 0; q_idx < i; ++q_idx) {
				q[q_idx] = q[q_idx] >> (new_shift - header->shift);
			}
			header->shift = new_shift;
		}

		adc_convert_q31(&q[i], sample, &adc_spec[i], header->shift);
	}
	LOG_DBG("Total channels in header: %" PRIu32, header->num_channels);
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

/**
 * @brief Fallback function for retrofiting old drivers to rtio
 *
 * @param[in] dev The ADC device to read
 * @param[in] iodev_sqe The read submission queue event
 */
static void adc_submit_fallback(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed. Consider to increase "
			"CONFIG_RTIO_WORKQ_POOL_ITEMS.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, adc_submit_fallback_sync);
}
#endif /* CONFIG_RTIO_WORKQ */

/**
 * @brief Default decoder get frame count
 *
 * Default reader can only ever service a single frame at a time.
 *
 * @param[in]  buffer The data buffer to parse
 * @param[in]  channel The channel to get the count for
 * @param[out] frame_count The number of frames in the buffer (always 1)
 * @return 0 in all cases
 */
static int get_frame_count(const uint8_t *buffer, uint32_t channel, uint16_t *frame_count)
{
	*frame_count = 1;
	return 0;
}

int adc_natively_supported_channel_size_info(struct adc_dt_spec adc_spec, uint32_t channel,
					size_t *base_size, size_t *frame_size)
{
	__ASSERT_NO_MSG(base_size != NULL);
	__ASSERT_NO_MSG(frame_size != NULL);

	*base_size = sizeof(struct adc_data);
	*frame_size = sizeof(struct adc_sample_data);
	return 0;
}

static int get_q31_value(const struct adc_data_generic_header *header, const q31_t *values,
			 uint32_t channel, q31_t *out)
{
	for (size_t i = 0; i < header->num_channels; ++i) {
		if (channel == header->channels[i].chan_idx) {
			*out = values[i];
			return 0;
		}
	}

	return -EINVAL;
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
 * @param[in,out] fit The current frame iterator
 * @param[in]     max_count The maximum number of channels to decode.
 * @param[out]    data_out The decoded data
 * @return 0 no more samples to decode
 * @return >0 the number of decoded frames
 * @return <0 on error
 */
static int decode(const uint8_t *buffer, uint32_t channel, uint32_t *fit,
		uint16_t max_count, void *data_out)
{
	const struct adc_data_generic_header *header =
		(const struct adc_data_generic_header *)buffer;
	const q31_t *q = (const q31_t *)(buffer + compute_header_size(header->num_channels));
	struct adc_data *data_out_q31 = (struct adc_data *)data_out;

	if (*fit != 0 || max_count < 1) {
		return -EINVAL;
	}

	data_out_q31->header.base_timestamp_ns = header->timestamp_ns;
	data_out_q31->header.reading_count = 1;
	data_out_q31->shift = header->shift;
	data_out_q31->readings[0].timestamp_delta = 0;

	*fit = 1;

	return get_q31_value(header, q, channel, &data_out_q31->readings[0].value);
}

const struct adc_decoder_api __adc_default_decoder = {
	.get_frame_count = get_frame_count,
	.get_size_info = adc_natively_supported_channel_size_info,
	.decode = decode,
};
