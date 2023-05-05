/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT asahi_kasei_akm09918c

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "akm09918c.h"
#include "akm09918c_decoder.h"
#include "akm09918c_internal.h"
#include "akm09918c_reg.h"
#include "akm09918c_rtio.h"

LOG_MODULE_REGISTER(AKM09918C, CONFIG_SENSOR_LOG_LEVEL);

static int akm09918c_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	return akm09918c_sample_fetch_helper(dev, chan, &data->x_sample, &data->y_sample,
					     &data->z_sample);
}

/**
 * @brief Perform the bus transaction to fetch samples
 *
 * @param dev Sensor device to operate on
 * @param chan Channel ID to fetch
 * @param x Location to write X channel sample or null.
 * @param y Location to write Y channel sample or null.
 * @param z Location to write Z channel sample or null.
 * @return int 0 if successful or error code
 */
static int akm09918c_sample_fetch_helper(const struct device *dev, enum sensor_channel chan,
					 int16_t *x, int16_t *y, int16_t *z)
{
	struct akm09918c_data *data = dev->data;
	const struct akm09918c_config *cfg = dev->config;
	uint8_t buf[9] = {0};

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_MAGN_X && chan != SENSOR_CHAN_MAGN_Y &&
	    chan != SENSOR_CHAN_MAGN_Z && chan != SENSOR_CHAN_MAGN_XYZ) {
		LOG_WRN("Invalid channel %d", chan);
		return -EINVAL;
	}

	if (data->mode == AKM09918C_CNTL2_PWR_DOWN) {
		if (i2c_reg_write_byte_dt(&cfg->i2c, AKM09918C_REG_CNTL2,
					  AKM09918C_CNTL2_SINGLE_MEASURE) != 0) {
			LOG_ERR("Failed to start measurement.");
			return -EIO;
		}

		/* Wait for sample */
		LOG_DBG("Waiting for sample...");
		k_usleep(AKM09918C_MEASURE_TIME_US);
	}

	/* We have to read through the TMPS register or the data_ready bit won't clear */
	if (i2c_burst_read_dt(&cfg->i2c, AKM09918C_REG_ST1, buf, ARRAY_SIZE(buf)) != 0) {
		LOG_ERR("Failed to read sample data.");
		return -EIO;
	}

	if (FIELD_GET(AKM09918C_ST1_DRDY, buf[0]) == 0) {
		LOG_ERR("Data not ready, st1=0x%02x", buf[0]);
		return -EBUSY;
	}

	if (x)
		*x = sys_le16_to_cpu(buf[1] | (buf[2] << 8));
	if (y)
		*y = sys_le16_to_cpu(buf[3] | (buf[4] << 8));
	if (z)
		*z = sys_le16_to_cpu(buf[5] | (buf[6] << 8));

	return 0;
}

static void akm09918c_convert(struct sensor_value *val, int16_t sample)
{
	int64_t conv_val = sample * AKM09918C_MICRO_GAUSS_PER_BIT;

	val->val1 = conv_val / 1000000;
	val->val2 = conv_val - (val->val1 * 1000000);
}

static int akm09918c_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct akm09918c_data *data = dev->data;

	if (chan == SENSOR_CHAN_MAGN_XYZ) {
		akm09918c_convert(val, data->x_sample);
		akm09918c_convert(val + 1, data->y_sample);
		akm09918c_convert(val + 2, data->z_sample);
	} else if (chan == SENSOR_CHAN_MAGN_X) {
		akm09918c_convert(val, data->x_sample);
	} else if (chan == SENSOR_CHAN_MAGN_Y) {
		akm09918c_convert(val, data->y_sample);
	} else if (chan == SENSOR_CHAN_MAGN_Z) {
		akm09918c_convert(val, data->z_sample);
	} else {
		LOG_WRN("Invalid channel %d", chan);
		return -EINVAL;
	}

	return 0;
}

static int akm09918c_attr_get(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, struct sensor_value *val)
{
	struct akm09918c_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		if (attr != SENSOR_ATTR_SAMPLING_FREQUENCY) {
			LOG_WRN("Invalid attribute %d", attr);
			return -EINVAL;
		}
		akm09918c_reg_to_hz(data->mode, val);
		break;
	default:
		LOG_WRN("Invalid channel %d", chan);
		return -EINVAL;
	}
	return 0;
}

static int akm09918c_attr_set(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct akm09918c_config *cfg = dev->config;
	struct akm09918c_data *data = dev->data;
	int res;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		if (attr != SENSOR_ATTR_SAMPLING_FREQUENCY) {
			LOG_WRN("Invalid attribute %d", attr);
			return -EINVAL;
		}

		uint8_t mode = akm09918c_hz_to_reg(val);

		res = i2c_reg_write_byte_dt(&cfg->i2c, AKM09918C_REG_CNTL2, mode);
		if (res != 0) {
			LOG_ERR("Failed to set sample frequency");
			return -EIO;
		}
		data->mode = mode;
		break;
	default:
		LOG_WRN("Invalid channel %d", chan);
		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api akm09918c_driver_api = {
	.sample_fetch = akm09918c_sample_fetch,
	.channel_get = akm09918c_channel_get,
	.attr_get = akm09918c_attr_get,
	.attr_set = akm09918c_attr_set,
};

static inline int akm09918c_check_who_am_i(const struct i2c_dt_spec *i2c)
{
	uint8_t buffer[2];
	int rc;

	rc = i2c_burst_read_dt(i2c, AKM09918C_REG_WIA1, buffer, ARRAY_SIZE(buffer));
	if (rc != 0) {
		LOG_ERR("Failed to read who-am-i register (rc=%d)", rc);
		return -EIO;
	}

	if (buffer[0] != AKM09918C_WIA1 || buffer[1] != AKM09918C_WIA2) {
		LOG_ERR("Wrong who-am-i value");
		return -EINVAL;
	}

	return 0;
}

static int akm09918c_init(const struct device *dev)
{
	const struct akm09918c_config *cfg = dev->config;
	struct akm09918c_data *data = dev->data;
	int rc;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* Soft reset the chip */
	rc = i2c_reg_write_byte_dt(&cfg->i2c, AKM09918C_REG_CNTL3,
				   FIELD_PREP(AKM09918C_CNTL3_SRST, 1));
	if (rc != 0) {
		LOG_ERR("Failed to soft reset");
		return -EIO;
	}

	/* check chip ID */
	rc = akm09918c_check_who_am_i(&cfg->i2c);
	if (rc != 0) {
		return rc;
	}
	data->mode = AKM09918C_CNTL2_PWR_DOWN;

	return 0;
}

int akm09981c_convert_raw_to_q31(enum sensor_channel chan, int16_t reading, q31_t *out)
{
	/* Convert and store reading with separate whole and fractional parts */
	struct sensor_value val;

	switch (chan) {
	case SENSOR_CHAN_MAGN_XYZ:
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
		akm09918c_convert(&val, reading);
		*out = (val.val1 << AKM09918C_RTIO_SHIFT | val.val2);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Count number of samples needed to fulfill requested list of channels (a single
 *        channel like SENSOR_CHAN_MAGN_XYZ may count as three).
 *
 * @param channels Pointer to channel list
 * @param num_channels Length of channel list
 * @return int Number of samples needed
 */
static int compute_num_samples(const enum sensor_channel *channels, size_t num_channels)
{
	int num_samples = 0;

	for (size_t i = 0; i < num_channels; ++i) {
		num_samples += SENSOR_CHANNEL_3_AXIS(channels[i]) ? 3 : 1;
	}

	return num_samples;
}

/**
 * @brief Calculate the minimum size RTIO buffer needed to fulfill the request.
 *
 * @param num_channels Count of requested channels (i.e. number of channel headers)
 * @param num_output_samples Count of requested samples (i.e. payload)
 * @return uint32_t Size of buffer needed to store the above
 */
static uint32_t compute_min_buf_len(size_t num_channels, int num_output_samples)
{
	return sizeof(struct sensor_data_generic_header) +
	       (num_channels * sizeof(struct sensor_data_generic_channel)) +
	       (num_output_samples * sizeof(int16_t));
}

/**
 * @brief Process an RTIO submission queue event.
 *
 * @param sensor Reference to the AKM09918C sensor being targetted
 * @param iodev_sqe Reference to RTIO submission queue
 * @return int 0 if successful or error code
 */
int akm09918c_submit(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe)
{

	const struct sensor_read_config *cfg = iodev_sqe->sqe->iodev->data;
	const enum sensor_channel *const channels = cfg->channels;

	const size_t num_channels = cfg->count;
	int num_output_samples = compute_num_samples(channels, num_channels);
	uint32_t min_buf_len = compute_min_buf_len(num_channels, num_output_samples);

	uint8_t *buf;
	uint32_t buf_len;

	int ret;

	/* Get RTIO memory buffer */
	int ret = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (ret) {
		LOG_ERR("rtio_sqe_rx_buf failed with code %d", ret);

		rtio_iodev_sqe_err(iodev_sqe, ret);
		return ret;
	}

	/* Insert header */
	struct sensor_data_generic_header *header = (struct sensor_data_generic_header *)buf;

	header->timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());
	header->num_channels = num_channels;

	/* Loop through and add channel header(s) */
	for (int i = 0; i < num_channels; i++) {
		header->channel_info[i].channel = channels[i];
		header->channel_info[i].shift = AKM09918C_RTIO_SHIFT;
	}

	/* Fetch and insert data for each channel */
	uint16_t *sample_data = buf + sizeof(struct sensor_data_generic_header) +
				(num_channels * sizeof(struct sensor_data_generic_header));

	for (i = 0; i < num_channels; i++) {
		switch (channels[i]) {
		case SENSOR_CHAN_MAGN_X:
			ret = akm09918c_sample_fetch_helper(sensor, channels[i], &sample_data, NULL,
							    NULL);
			sample_data++;
			break;
		case SENSOR_CHAN_MAGN_Y:
			ret = akm09918c_sample_fetch_helper(sensor, channels[i], NULL, &sample_data,
							    NULL);
			sample_data++;
			break;
		case SENSOR_CHAN_MAGN_Z:
			ret = akm09918c_sample_fetch_helper(sensor, channels[i], NULL, NULL,
							    &sample_data);
			sample_data++;
			break;
		case SENSOR_CHAN_MAGN_XYZ:
			ret = akm09918c_sample_fetch_helper(sensor, channels[i], &sample_data[0],
							    &sample_data[1], &sample_data[2]);
			sample_data += 3;
			break;
		}

		if (ret) {
			LOG_ERR("Failed to fetch channel %d (%d)", channels[i], ret);

			rtio_iodev_sqe_err(iodev_sqe, ret);
			return ret;
		}

		sample_data += chan_samples;
	}

	rtio_iodev_sqe_ok(iodev_sqe, 0);

	return 0;
}

static void akm09918c_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe->iodev->data;
	const struct device *dev = cfg->sensor;
	const struct sensor_driver_api *api = dev->api;

	if (api->submit != NULL) {
		// TODO: Implement Sensor v2
		api->submit(dev, iodev_sqe);
	} else {
		akm09918c_submit(dev, iodev_sqe);
	}
}

struct rtio_iodev_api __akm09918c_iodev_api = {
	.submit = akm09918c_iodev_submit,
};

/**
 * @brief Decode an RTIO sensor data buffer and extract samples in Q31 format.
 *
 * @param buffer Buffer received through RTIO context
 * @param fit Frame iterator
 * @param cit Channel iterator
 * @param channels Pointer to list of channels
 * @param values Pointer to memory for writing out decoded samples
 * @param max_count Maximum number of Q31 samples to write to `values`.
 * @return int 0 if successful or error code.
 */
static int decode(const uint8_t *buffer, sensor_frame_iterator_t *fit,
		  sensor_channel_iterator_t *cit, enum sensor_channel *channels, q31_t *values,
		  uint8_t max_count)
{
	const struct sensor_data_generic_header *header =
		(const struct sensor_data_generic_header *)buffer;
	const int16_t *samples =
		(const int16_t *)(buffer + sizeof(struct sensor_data_generic_header) +
				  header->num_channels *
					  sizeof(struct sensor_data_generic_channel));

	int total_samples = 0;

	if (!fit || !*fit) {
		/* TODO: support reading multiple frames in one buffer. For now, consider it an
		 * error if `fit > 0`.
		 */
		LOG_ERR("frame iterator NULL or non-zero");
		return -EINVAL;
	}

	/* Track sample offset in the buffer */
	int sample_offset = 0;

	/* Calculate how many samples exist in buffer. */
	for (int i = 0; i < header->num_channels; ++i) {
		int num_samples_in_channel =
			SENSOR_CHANNEL_3_AXIS(header->channel_info[i].channel) ? 3 : 1;

		total_samples += num_samples_in_channel;

		if (i < *cit) {
			/* While looping, calculate sample offset for the current value of the
			 * channel iterator. */
			sample_offset += num_samples_in_channel;
		}
	}

	int offset_in_triplet = 0;

	/* Loop through channels, ensuring the channel iterator does not overrun the end of the
	 * frame and that no more than `max_count` samples get written out.
	 */
	for (int i = 0; *cit < total_samples && i < max_count; i++) {
		enum sensor_channel channel = header->channel_info[*cit].channel;

		if (SENSOR_CHANNEL_3_AXIS(channel)) {
			channels[i] = channel - 3 + offset_in_triplet;
			values[i] = akm09981c_convert_raw_to_q31(
				samples[sample_offset + offset_in_triplet]);
			offset_in_triplet++;
			if (offset_in_triplet == 3) {
				*cit++;
				sample_offset += 3;
				offset_in_triplet = 0;
			}
		} else {
			channels[i] = channel;
			values[i] = akm09981c_convert_raw_to_q31(samples[sample_offset]);
			sample_offset++;
			*cit++;
		}
	}

	if (*cit >= num_samples) {
		/* All samples in frame have been read. Reset channel iterator and advance frame
		 * iterator. */
		*fit++;
		*cit = 0;
	}
	return 0;
}

/**
 * @brief Get the number of frames from the RTIO sensor data buffer. We only support one-shot mode
 * for now, so this will always be 1.
 *
 * @param buffer Data buffer.
 * @param frame_count Output param for frame count.
 * @return int 0 if successful or error code.
 */
static int get_frame_count(const uint8_t *buffer, uint16_t *frame_count)
{
	*frame_count = 1;
	return 0;
}

/**
 * @brief Get the timestamp value from an RTIO sensor data buffer
 *
 * @param buffer Data buffer.
 * @param timestamp_ns Output param for timestamp.
 * @return int 0 if successful or error code.
 */
static int get_timestamp(const uint8_t *buffer, uint64_t *timestamp_ns)
{
	*timestamp_ns = ((struct sensor_data_generic_header *)buffer)->timestamp_ns;
	return 0;
}

/**
 * @brief Get the shift offset used for encoding Q31 ints. All channels use a fixed shift of
 * AKM09918C, so just verify the channel is supported
 *
 * @param buffer Unused
 * @param channel_type Channel ID enum
 * @param shift Output param to write shift to
 * @return int 0 if successful, -EINVAL if unsupported channel.
 */
static int get_shift(const uint8_t *buffer, enum sensor_channel channel_type, int8_t *shift)
{
	ARG_UNUSED(buffer);

	switch (channel_type) {
	case SENSOR_CHAN_MAGN_XYZ:
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
		*shift = AKM09918C_RTIO_SHIFT;
		return 0;
	}

	/* Unsupported channel */
	return -EINVAL;
}

struct sensor_decoder_api akm09981c_decoder = {
	.get_frame_count = get_frame_count,
	.get_timestamp = get_timestamp,
	.get_shift = get_shift,
	.decode = decode,
};

int akm09981c_get_decoder(const struct device *dev, struct sensor_decoder_api *decoder)
{
	*decoder = akm09981c_decoder;

	return 0;
}

#define AKM09918C_DEFINE(inst)                                                                     \
	static struct akm09918c_data akm09918c_data_##inst;                                        \
                                                                                                   \
	static const struct akm09918c_config akm09918c_config_##inst = {                           \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, akm09918c_init, NULL, &akm09918c_data_##inst,           \
				     &akm09918c_config_##inst, POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY, &akm09918c_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AKM09918C_DEFINE)
