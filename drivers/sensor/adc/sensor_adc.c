/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/ringq.h>

#define DT_DRV_COMPAT sensor_adc

#define DRIVER_SAMPLE_SIZE sizeof(uint16_t)
#define DRIVER_TIMESTAMP_TICKS_SIZE sizeof(int64_t)

/* Max supported voltage is +-32V */
#define DRIVER_VOLTAGE_SHIFT 5

#define DRIVER_TRIGGER_DATA_READY BIT(0)
#define DRIVER_TRIGGER_FIFO_WATERMARK BIT(1)
#define DRIVER_TRIGGER_FIFO_FULL BIT(2)

/* 1us interval = 1MHz = 1000000000000uHz */
#define DRIVER_FREQUENCY_MAX_UHZ 1000000000000

/* 4000000000 interval = 0.00025Hz = 250uHz */
#define DRIVER_FREQUENCY_MIN_UHZ 250

LOG_MODULE_REGISTER(sensor_adc, CONFIG_SENSOR_LOG_LEVEL);

/*
 * The driver defines a software FIFO with ADC samplings. This FIFO is a sys_ringq containing
 * items of struct driver_buffer_sampling.
 *
 * The sensor buffer layout follows:
 *
 *   +-------------------------------+
 *   | struct driver_buffer_header   |
 *   +-------------------------------+
 *   | struct driver_buffer_channel  |
 *   | ...                           |
 *   +-------------------------------+
 *   | struct driver_buffer_sampling |
 *   | ...                           |
 *   +-------------------------------+
 *
 * For trigger only buffers, which occur if streaming and the data option is either NOP or DROP,
 * only the header is included in the buffer.
 */

struct driver_buffer_header {
	uint8_t channels_count;
	uint8_t trigger_mask;
	uint8_t resolution;
	uint8_t reserved;
	uint16_t samplings_count;
} __packed;

struct driver_buffer_channel {
	uint16_t vref_mv;
	uint8_t gain;
	uint8_t differential;
} __packed;

struct driver_buffer_sampling {
	int64_t timestamp_ticks;
	uint16_t samples[];
};

enum driver_response {
	DRIVER_RESPONSE_NONE = 0,
	DRIVER_RESPONSE_DATA,
	DRIVER_RESPONSE_DROP,
	DRIVER_RESPONSE_TRIGGER,
};

struct driver_data {
	const struct device *dev;
	struct k_spinlock spinlock;
	struct mpsc iodev_sqe_q;
	struct rtio_iodev_sqe *iodev_sqe;
	struct sys_ringq fifo_ringq;
	atomic_t run_atom;
	struct k_sem stopped_sem;
	struct k_sem lock_sem;
	struct adc_sequence sequence;
	struct adc_sequence_options sequence_options;
};

struct driver_config {
	uint8_t *sampling_data;
	uint8_t *fifo_ringq_data;
	const struct adc_dt_spec *channels;
	uint8_t channels_count;
	uint8_t calibrate;
	uint16_t fifo_depth;
	uint16_t fifo_watermark;
};

static size_t get_buffer_header_size(void)
{
	return sizeof(struct driver_buffer_header);
}

static size_t get_buffer_channel_size(void)
{
	return sizeof(struct driver_buffer_channel);
}

static size_t get_driver_buffer_channels_size(size_t channels_count)
{
	return get_buffer_channel_size() * channels_count;
}

static size_t get_driver_buffer_sampling_size(size_t channels_count)
{
	return DRIVER_TIMESTAMP_TICKS_SIZE + DRIVER_SAMPLE_SIZE * channels_count;
}

static size_t get_driver_buffer_samplings_size(size_t channels_count, size_t samplings_count)
{
	return get_driver_buffer_sampling_size(channels_count) * samplings_count;
}

static size_t get_driver_buffer_size(size_t channels_count, size_t samplings_count)
{
	return get_buffer_header_size() +
	       get_driver_buffer_channels_size(channels_count) +
	       get_driver_buffer_samplings_size(channels_count, samplings_count);
}

static size_t get_driver_buffer_max_samplings_size(size_t channels_count, size_t rx_buf_size)
{
	size_t remaining;

	remaining = rx_buf_size;
	remaining -= get_buffer_header_size();
	remaining -= get_driver_buffer_channels_size(channels_count);

	return remaining / get_driver_buffer_sampling_size(channels_count);
}

static struct driver_buffer_header *get_driver_buffer_header(uint8_t *rx_buf)
{
	return (struct driver_buffer_header *)rx_buf;
}

static const struct driver_buffer_header *get_driver_buffer_header_const(const uint8_t *rx_buf)
{
	return (const struct driver_buffer_header *)rx_buf;
}

static struct driver_buffer_channel *get_buffer_channel(uint8_t *rx_buf, size_t idx)
{
	uint8_t *ptr;

	ptr = rx_buf +
	      get_buffer_header_size() +
	      get_buffer_channel_size() *
	      idx;

	return (struct driver_buffer_channel *)ptr;
}

static const struct driver_buffer_channel *get_buffer_channel_const(const uint8_t *buffer,
								    size_t idx)
{
	const uint8_t *ptr;

	ptr = buffer +
	      get_buffer_header_size() +
	      get_buffer_channel_size() *
	      idx;

	return (const struct driver_buffer_channel *)ptr;
}

static struct driver_buffer_sampling *get_buffer_sampling(uint8_t *rx_buf,
							  size_t channels_count,
							  size_t idx)
{
	uint8_t *ptr;

	ptr = rx_buf +
	      get_buffer_header_size() +
	      get_driver_buffer_channels_size(channels_count) +
	      get_driver_buffer_sampling_size(channels_count) *
	      idx;

	return (struct driver_buffer_sampling *)ptr;
}

static const struct driver_buffer_sampling *get_buffer_sampling_const(const uint8_t *buffer,
								      size_t channels_count,
								      size_t idx)
{
	const uint8_t *ptr;

	ptr = buffer +
	      get_buffer_header_size() +
	      get_driver_buffer_channels_size(channels_count) +
	      get_driver_buffer_sampling_size(channels_count) *
	      idx;

	return (const struct driver_buffer_sampling *)ptr;
}

static size_t get_samples_size(const struct device *dev)
{
	const struct driver_config *dev_config;

	dev_config = dev->config;

	return dev_config->channels_count * DRIVER_SAMPLE_SIZE;
}

static size_t get_sampling_size(const struct device *dev)
{
	return DRIVER_TIMESTAMP_TICKS_SIZE + get_samples_size(dev);
}

static struct driver_buffer_sampling *get_sampling(uint8_t *data)
{
	return (struct driver_buffer_sampling *)data;
}

static size_t get_fifo_ringq_data_size(const struct device *dev)
{
	const struct driver_config *dev_config;

	dev_config = dev->config;

	return get_sampling_size(dev) * dev_config->fifo_depth;
}

static const struct device *get_adc_dev(const struct device *dev)
{
	const struct driver_config *dev_config;
	const struct adc_dt_spec *chan;

	dev_config = dev->config;
	chan = &dev_config->channels[0];

	return chan->dev;
}

static int frequency_uhz_to_interval_us(int64_t frequency_uhz, uint32_t *interval_us)
{
	if (frequency_uhz < 0) {
		return -EINVAL;
	}

	if (frequency_uhz == 0) {
		/* Disable */
		*interval_us = 0;
		return 0;
	}

	if (frequency_uhz > DRIVER_FREQUENCY_MAX_UHZ) {
		return -EINVAL;
	}

	if (frequency_uhz < DRIVER_FREQUENCY_MIN_UHZ) {
		return -EINVAL;
	}

	*interval_us = DRIVER_FREQUENCY_MAX_UHZ / frequency_uhz;

	return 0;
}

static int64_t interval_us_to_frequency_uhz(uint32_t interval_us)
{
	return DRIVER_FREQUENCY_MAX_UHZ / interval_us;
}

static void stop_adc_read(const struct device *dev)
{
	struct driver_data *dev_data;

	dev_data = dev->data;

	if (atomic_cas(&dev_data->run_atom, 1, 0)) {
		k_sem_take(&dev_data->stopped_sem, K_FOREVER);
	}

	pm_device_runtime_put(dev);
}

static int start_adc_read(const struct device *dev)
{
	struct driver_data *dev_data;
	const struct device *adc_dev;
	struct adc_sequence *sequence;
	int ret;

	dev_data = dev->data;
	adc_dev = get_adc_dev(dev);
	sequence = &dev_data->sequence;

	ret = pm_device_runtime_get(dev);
	if (ret) {
		LOG_ERR("failed to %s %s %s", "pm", "get", "sensor");
		return ret;
	}

	atomic_set(&dev_data->run_atom, 1);
	sys_ringq_reset(&dev_data->fifo_ringq);

	ret = adc_read_async(adc_dev, sequence, NULL);
	if (ret) {
		atomic_set(&dev_data->run_atom, 0);
		pm_device_runtime_put(dev);
		LOG_ERR("failed to %s %s %s", "start", "adc", "read");
		return ret;
	}

	return 0;
}

static int generate_trigger_response(const struct device *dev, uint8_t trigger_mask)
{
	struct driver_data *dev_data;
	struct rtio_iodev_sqe *iodev_sqe;
	size_t rx_buf_size;
	int ret;
	uint8_t *rx_buf;
	struct driver_buffer_header *buffer_header;

	dev_data = dev->data;
	iodev_sqe = dev_data->iodev_sqe;
	rx_buf_size = get_buffer_header_size();

	ret = rtio_sqe_rx_buf(iodev_sqe,
			      rx_buf_size,
			      rx_buf_size,
			      &rx_buf,
			      &rx_buf_size);
	if (ret) {
		LOG_ERR("rx buffer too small");
		return ret;
	}

	buffer_header = get_driver_buffer_header(rx_buf);

	/* Trigger only response only populates trigger mask and samplings count of header */
	buffer_header->trigger_mask = trigger_mask;
	buffer_header->samplings_count = 0;

	return 0;
}

static int generate_data_response(const struct device *dev, uint8_t trigger_mask)
{
	struct driver_data *dev_data;
	const struct driver_config *dev_config;
	struct sys_ringq *fifo_ringq;
	struct rtio_iodev_sqe *iodev_sqe;
	struct adc_sequence *sequence;
	size_t min_rx_buf_size;
	size_t fifo_ringq_size;
	size_t max_rx_buf_size;
	int ret;
	uint8_t *rx_buf;
	size_t rx_buf_size;
	uint16_t samplings_count;
	struct driver_buffer_header *buffer_header;
	size_t i;
	struct driver_buffer_channel *buffer_channel;
	const struct adc_dt_spec *channel;
	const struct adc_channel_cfg *channel_config;
	struct driver_buffer_sampling *sampling;

	dev_data = dev->data;
	dev_config = dev->config;
	fifo_ringq = &dev_data->fifo_ringq;
	iodev_sqe = dev_data->iodev_sqe;
	sequence = &dev_data->sequence;

	min_rx_buf_size = get_driver_buffer_size(dev_config->channels_count, 1);
	fifo_ringq_size = sys_ringq_size(fifo_ringq);
	max_rx_buf_size = get_driver_buffer_size(dev_config->channels_count, fifo_ringq_size);

	ret = rtio_sqe_rx_buf(iodev_sqe,
			      min_rx_buf_size,
			      max_rx_buf_size,
			      &rx_buf,
			      &rx_buf_size);
	if (ret) {
		LOG_ERR("rx buffer too small");
		return ret;
	}

	samplings_count = get_driver_buffer_max_samplings_size(dev_config->channels_count,
							       rx_buf_size);

	/*
	 * We may get allocated more memory than we have samplings by rtio_sqe_rx_buf so limit
	 * samplings_count to the actual number of samplings in the FIFO.
	 */
	samplings_count = MIN(samplings_count, fifo_ringq_size);

	buffer_header = get_driver_buffer_header(rx_buf);

	buffer_header->channels_count = dev_config->channels_count;
	buffer_header->trigger_mask = trigger_mask;
	buffer_header->resolution = sequence->resolution;
	buffer_header->samplings_count = samplings_count;

	for (i = 0; i < dev_config->channels_count; i++) {
		buffer_channel = get_buffer_channel(rx_buf, i);
		channel = &dev_config->channels[i];
		channel_config = &channel->channel_cfg;

		if (channel->vref_mv == 0) {
			buffer_channel->vref_mv = adc_ref_internal(channel->dev);
		} else {
			buffer_channel->vref_mv = channel->vref_mv;
		}

		buffer_channel->gain = channel_config->gain;
		buffer_channel->differential = channel_config->differential;
	}

	for (i = 0; i < samplings_count; i++) {
		sampling = get_buffer_sampling(rx_buf, dev_config->channels_count, i);
		sys_ringq_get(fifo_ringq, sampling);
	}

	return 0;
}

static void iodev_sqe_queue_get(const struct device *dev)
{
	struct driver_data *dev_data;
	struct mpsc_node *next;

	dev_data = dev->data;
	next = mpsc_pop(&dev_data->iodev_sqe_q);

	if (next == NULL) {
		dev_data->iodev_sqe = NULL;
	} else {
		dev_data->iodev_sqe = CONTAINER_OF(next, struct rtio_iodev_sqe, q);
	}
}

static void iodev_sqe_complete(const struct device *dev, int result)
{
	struct driver_data *dev_data;

	dev_data = dev->data;

	if (result) {
		rtio_iodev_sqe_err(dev_data->iodev_sqe, result);
	} else {
		rtio_iodev_sqe_ok(dev_data->iodev_sqe, result);
	}

	iodev_sqe_queue_get(dev);
}

static uint8_t get_pending_trigger_mask(const struct device *dev)
{
	const struct driver_config *dev_config;
	struct driver_data *dev_data;
	struct sys_ringq *fifo_ringq;
	uint8_t trigger_mask;
	size_t fifo_ringq_size;

	dev_config = dev->config;
	dev_data = dev->data;
	fifo_ringq = &dev_data->fifo_ringq;
	trigger_mask = DRIVER_TRIGGER_DATA_READY;
	fifo_ringq_size = sys_ringq_size(fifo_ringq);

	if (dev_config->fifo_watermark && fifo_ringq_size >= dev_config->fifo_watermark) {
		trigger_mask |= DRIVER_TRIGGER_FIFO_WATERMARK;
	}

	if (fifo_ringq_size == dev_config->fifo_depth) {
		trigger_mask |= DRIVER_TRIGGER_FIFO_FULL;
	}

	return trigger_mask;
}

static bool trigger_is_pending(const struct sensor_stream_trigger *trigger,
			       uint8_t pending_trigger_mask)
{
	switch (trigger->trigger) {
	case SENSOR_TRIG_DATA_READY:
		return pending_trigger_mask & DRIVER_TRIGGER_DATA_READY;

	case SENSOR_TRIG_FIFO_WATERMARK:
		return pending_trigger_mask & DRIVER_TRIGGER_FIFO_WATERMARK;

	case SENSOR_TRIG_FIFO_FULL:
		return pending_trigger_mask & DRIVER_TRIGGER_FIFO_FULL;

	default:
		return false;
	}
}

static enum driver_response get_stream_response(const struct device *dev,
						uint8_t pending_trigger_mask)
{
	struct driver_data *dev_data;
	struct rtio_iodev_sqe *iodev_sqe;
	struct rtio_sqe *sqe;
	const struct rtio_iodev *iodev;
	struct sensor_read_config *read_config;
	bool data_include;
	bool data_nop;
	bool data_drop;
	size_t i;
	const struct sensor_stream_trigger *trigger;

	dev_data = dev->data;
	iodev_sqe = dev_data->iodev_sqe;
	sqe = &iodev_sqe->sqe;
	iodev = sqe->iodev;
	read_config = iodev->data;

	data_include = false;
	data_nop = false;
	data_drop = false;

	for (i = 0; i < read_config->count; i++) {
		trigger = &read_config->triggers[i];

		if (!trigger_is_pending(trigger, pending_trigger_mask)) {
			continue;
		}

		switch (trigger->opt) {
		case SENSOR_STREAM_DATA_INCLUDE:
			data_include = true;
			break;

		case SENSOR_STREAM_DATA_NOP:
			data_nop = true;
			break;

		case SENSOR_STREAM_DATA_DROP:
			data_drop = true;
			break;

		default:
			break;
		}
	}

	/*
	 * Some trigger data options may conflict with each other. Resolve conflicts by ranking
	 * the trigger options. If any trigger wants to keep data, keep data. If no trigger wants
	 * to keep data, but some want to drop it, drop data. If any trigger was triggered,
	 * respond with the triggers. Lastly, if no trigger was triggered, do not respond.
	 */
	if (data_include) {
		return DRIVER_RESPONSE_DATA;
	}

	if (data_drop) {
		return DRIVER_RESPONSE_DROP;
	}

	if (data_nop) {
		return DRIVER_RESPONSE_TRIGGER;
	}

	return DRIVER_RESPONSE_NONE;
}

static enum adc_action read_done_callback(const struct device *adc,
					  const struct adc_sequence *sequence,
					  uint16_t sampling_index)
{
	const struct adc_sequence_options *seq_options;
	const struct device *dev;
	const struct driver_config *dev_config;
	struct driver_data *dev_data;
	struct sys_ringq *fifo_ringq;
	struct driver_buffer_sampling *sampling;
	struct rtio_iodev_sqe *iodev_sqe;
	struct rtio_sqe *sqe;
	const struct rtio_iodev *iodev;
	struct sensor_read_config *read_config;
	uint8_t pending_trigger_mask;
	enum driver_response response;
	int ret;

	seq_options = sequence->options;

	dev = seq_options->user_data;
	dev_config = dev->config;
	dev_data = dev->data;
	fifo_ringq = &dev_data->fifo_ringq;

	ARG_UNUSED(adc);
	ARG_UNUSED(sampling_index);

	if (sys_ringq_full(fifo_ringq)) {
		/* Discard oldest sampling */
		sys_ringq_get(fifo_ringq, NULL);
	}

	sampling = (struct driver_buffer_sampling *)dev_config->sampling_data;
	sampling->timestamp_ticks = k_uptime_ticks();
	sys_ringq_put(fifo_ringq, sampling);

	if (dev_data->iodev_sqe == NULL) {
		iodev_sqe_queue_get(dev);
	}

	if (dev_data->iodev_sqe != NULL) {
		pending_trigger_mask = get_pending_trigger_mask(dev);
		iodev_sqe = dev_data->iodev_sqe;
		sqe = &iodev_sqe->sqe;
		iodev = sqe->iodev;
		read_config = iodev->data;

		if (read_config->is_streaming) {
			response = get_stream_response(dev, pending_trigger_mask);
		} else {
			response = DRIVER_RESPONSE_DATA;
		}

		switch (response) {
		case DRIVER_RESPONSE_NONE:
			break;

		case DRIVER_RESPONSE_DATA:
			ret = generate_data_response(dev, pending_trigger_mask);
			iodev_sqe_complete(dev, ret);
			break;

		case DRIVER_RESPONSE_DROP:
			sys_ringq_reset(fifo_ringq);
			ret = generate_trigger_response(dev, pending_trigger_mask);
			iodev_sqe_complete(dev, ret);
			break;

		case DRIVER_RESPONSE_TRIGGER:
			ret = generate_trigger_response(dev, pending_trigger_mask);
			iodev_sqe_complete(dev, ret);
			break;

		default:
			__ASSERT_NO_MSG(0);
			break;
		}
	}

	if (atomic_get(&dev_data->run_atom) == 1) {
		return ADC_ACTION_REPEAT;
	}

	k_sem_give(&dev_data->stopped_sem);
	return ADC_ACTION_FINISH;
}

static int driver_decoder_get_frame_count(const uint8_t *buffer,
					  struct sensor_chan_spec channel,
					  uint16_t *frame_count)
{
	const struct driver_buffer_header *buffer_header;

	if (channel.chan_type != SENSOR_CHAN_VOLTAGE) {
		return -EINVAL;
	}

	buffer_header = get_driver_buffer_header_const(buffer);

	if (channel.chan_idx >= buffer_header->channels_count) {
		return -EINVAL;
	}

	if (buffer_header->samplings_count == 0) {
		return -EINVAL;
	}

	*frame_count = buffer_header->samplings_count;
	return 0;
}

static int driver_decoder_get_size_info(struct sensor_chan_spec channel,
					size_t *base_size,
					size_t *frame_size)
{
	if (channel.chan_type != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	*base_size = sizeof(struct sensor_q31_data);
	*frame_size = sizeof(struct sensor_q31_sample_data);
	return 0;
}

static int driver_decoder_decode(const uint8_t *buffer,
				 struct sensor_chan_spec channel,
				 uint32_t *fit,
				 uint16_t max_count,
				 void *data_out)
{
	const struct driver_buffer_header *buffer_header;
	struct sensor_q31_data *data;
	const struct driver_buffer_channel *buffer_channel;
	const struct driver_buffer_sampling *buffer_sampling;
	int64_t base_timestamp_ticks;
	uint32_t count;
	int64_t timestamp_delta;
	struct sensor_q31_sample_data *sample_data;
	int64_t value;
	struct sensor_data_header *data_header;

	buffer_header = get_driver_buffer_header_const(buffer);

	if (*fit >= buffer_header->samplings_count) {
		return -EINVAL;
	}

	data = data_out;
	data->shift = DRIVER_VOLTAGE_SHIFT;

	buffer_channel = get_buffer_channel_const(buffer, channel.chan_idx);

	buffer_sampling = get_buffer_sampling_const(buffer,
						    buffer_header->channels_count,
						    *fit);

	base_timestamp_ticks = buffer_sampling->timestamp_ticks;
	count = 0;

	while (count < max_count) {
		buffer_sampling = get_buffer_sampling_const(buffer,
							    buffer_header->channels_count,
							    *fit);

		timestamp_delta = buffer_sampling->timestamp_ticks;
		timestamp_delta -= base_timestamp_ticks;
		timestamp_delta = k_ticks_to_ns_floor64(timestamp_delta);

		if (timestamp_delta > UINT32_MAX) {
			/* Timestamp delta overrun. Can only happen if count > 0 */
			break;
		}

		value = buffer_sampling->samples[channel.chan_idx];
		value *= buffer_channel->vref_mv;
		value <<= 31 - DRIVER_VOLTAGE_SHIFT - buffer_header->resolution;
		adc_gain_invert_64(buffer_channel->gain, &value);
		value /= 1000;

		sample_data = &data->readings[count];
		sample_data->timestamp_delta = timestamp_delta;
		sample_data->voltage = (q31_t)value;

		*fit += 1;
		count += 1;

		if (*fit == buffer_header->samplings_count) {
			break;
		}
	}

	data_header = &data->header;
	data_header->base_timestamp_ns = k_ticks_to_ns_floor64(base_timestamp_ticks);
	data_header->reading_count = count;

	return count;
}

static bool driver_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	const struct driver_buffer_header *buffer_header;

	buffer_header = get_driver_buffer_header_const(buffer);

	switch (trigger) {
	case SENSOR_TRIG_DATA_READY:
		return buffer_header->trigger_mask & DRIVER_TRIGGER_DATA_READY;

	case SENSOR_TRIG_FIFO_WATERMARK:
		return buffer_header->trigger_mask & DRIVER_TRIGGER_FIFO_WATERMARK;

	case SENSOR_TRIG_FIFO_FULL:
		return buffer_header->trigger_mask & DRIVER_TRIGGER_FIFO_FULL;

	default:
		return false;
	}
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = driver_decoder_get_frame_count,
	.get_size_info = driver_decoder_get_size_info,
	.decode = driver_decoder_decode,
	.has_trigger = driver_decoder_has_trigger,
};

static int driver_api_get_decoder(const struct device *dev,
				  const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);

	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}

static void driver_api_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct driver_data *dev_data;

	dev_data = dev->data;
	mpsc_push(&dev_data->iodev_sqe_q, &iodev_sqe->q);
}

static int attr_set_sampling_frequency(const struct device *dev, const struct sensor_value *val)
{
	struct driver_data *dev_data;
	int64_t frequency_uhz;
	uint32_t interval_us;
	bool running;
	int ret;

	dev_data = dev->data;
	frequency_uhz = sensor_value_to_micro(val);

	ret = frequency_uhz_to_interval_us(frequency_uhz, &interval_us);
	if (ret != 0) {
		return ret;
	}

	k_sem_take(&dev_data->lock_sem, K_FOREVER);

	running = atomic_get(&dev_data->run_atom);

	if (interval_us == 0) {
		dev_data->sequence_options.interval_us = 0;

		if (running) {
			stop_adc_read(dev);
		}

		k_sem_give(&dev_data->lock_sem);

		return 0;
	}

	if (running) {
		stop_adc_read(dev);
	}

	dev_data->sequence_options.interval_us = interval_us;

	ret = start_adc_read(dev);

	k_sem_give(&dev_data->lock_sem);

	return ret;
}

static int attr_get_sampling_frequency(const struct device *dev, struct sensor_value *val)
{
	struct driver_data *dev_data;
	uint32_t interval_us;
	int64_t frequency_uhz;

	dev_data = dev->data;

	k_sem_take(&dev_data->lock_sem, K_FOREVER);

	interval_us = dev_data->sequence_options.interval_us;

	k_sem_give(&dev_data->lock_sem);

	if (interval_us) {
		frequency_uhz = interval_us_to_frequency_uhz(interval_us);
	} else {
		frequency_uhz = 0;
	}

	return sensor_value_from_micro(val, frequency_uhz);
}

static int driver_api_attr_set(const struct device *dev,
			       enum sensor_channel chan,
			       enum sensor_attribute attr,
			       const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return attr_set_sampling_frequency(dev, val);

	default:
		return -ENOTSUP;
	}
}

static int driver_api_attr_get(const struct device *dev,
			       enum sensor_channel chan,
			       enum sensor_attribute attr,
			       struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return attr_get_sampling_frequency(dev, val);

	default:
		return -ENOTSUP;
	}
}

static int driver_api_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);

	return -ENOTSUP;
}

static int driver_api_channel_get(const struct device *dev,
				  enum sensor_channel chan,
				  struct sensor_value *val)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);
	ARG_UNUSED(val);

	return -ENOTSUP;
}

static DEVICE_API(sensor, driver_api) = {
	.attr_set = driver_api_attr_set,
	.attr_get = driver_api_attr_get,
	.sample_fetch = driver_api_sample_fetch,
	.channel_get = driver_api_channel_get,
	.get_decoder = driver_api_get_decoder,
	.submit = driver_api_submit,
};

static int resume(const struct device *dev)
{
	const struct device *adc_dev;
	int ret;

	adc_dev = get_adc_dev(dev);

	ret = pm_device_runtime_get(adc_dev);
	if (ret) {
		LOG_ERR("failed to %s %s %s", "pm", "get", "adc");
		return ret;
	}

	return 0;
}

static int suspend(const struct device *dev)
{
	const struct device *adc_dev;
	int ret;

	adc_dev = get_adc_dev(dev);

	ret = pm_device_runtime_put(adc_dev);
	if (ret) {
		LOG_ERR("failed to %s %s %s", "pm", "put", "adc");
		return ret;
	}

	return 0;
}

static int driver_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = resume(dev);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		ret = suspend(dev);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int driver_init(const struct device *dev)
{
	struct driver_data *dev_data;
	const struct driver_config *dev_config;
	struct adc_sequence_options *sequence_options;
	struct adc_sequence *sequence;
	const struct adc_dt_spec *chan;
	struct driver_buffer_sampling *sampling;
	const struct device *adc_dev;
	int ret;
	size_t i;

	dev_data = dev->data;
	dev_config = dev->config;
	sequence_options = &dev_data->sequence_options;
	sequence = &dev_data->sequence;

	dev_data->dev = dev;
	mpsc_init(&dev_data->iodev_sqe_q);
	sys_ringq_init(&dev_data->fifo_ringq,
		       dev_config->fifo_ringq_data,
		       get_fifo_ringq_data_size(dev),
		       get_sampling_size(dev));

	atomic_set(&dev_data->run_atom, 0);
	k_sem_init(&dev_data->stopped_sem, 0, 1);
	k_sem_init(&dev_data->lock_sem, 1, 1);

	chan = &dev_config->channels[0];

	sequence_options->interval_us = 0;
	sequence_options->callback = read_done_callback;
	sequence_options->user_data = (void *)dev;
	sequence_options->extra_samplings = 0;

	sampling = get_sampling(dev_config->sampling_data);

	sequence->options = sequence_options;
	sequence->channels = 0;
	sequence->buffer = &sampling->samples;
	sequence->buffer_size = get_samples_size(dev);
	sequence->resolution = chan->resolution;
	sequence->oversampling = chan->oversampling;
	sequence->calibrate = dev_config->calibrate;

	adc_dev = chan->dev;

	ret = device_is_ready(adc_dev) ? 0 : -ENODEV;
	if (ret) {
		LOG_ERR("%s is not ready", adc_dev->name);
		return ret;
	}

	for (i = 0; i < dev_config->channels_count; i++) {
		chan = &dev_config->channels[i];

		ret = adc_channel_setup_dt(chan);
		if (ret) {
			LOG_ERR("failed to %s %s %s", "set", "up", "io-channel");
			return ret;
		}

		sequence->channels |= BIT(chan->channel_id);
	}

	return pm_device_driver_init(dev, driver_pm_action);
}

#if CONFIG_DEVICE_DEINIT_SUPPORT
static int driver_deinit(const struct device *dev)
{
	return pm_device_driver_deinit(dev, driver_pm_action);
}
#endif

#define DRIVER_ADC_CHAN_HAS_PROP(node_id, prop, idx)						\
	DT_NODE_HAS_PROP(									\
		ADC_CHANNEL_DT_NODE(								\
			DT_IO_CHANNELS_CTLR_BY_IDX(node_id, idx),				\
			DT_IO_CHANNELS_INPUT_BY_IDX(node_id, idx)				\
		),										\
		prop										\
	)

#define DRIVER_ADC_CHAN_PROP(node_id, prop, idx)						\
	DT_PROP(										\
		ADC_CHANNEL_DT_NODE(								\
			DT_IO_CHANNELS_CTLR_BY_IDX(node_id, idx),				\
			DT_IO_CHANNELS_INPUT_BY_IDX(node_id, idx)				\
		),										\
		prop										\
	)

#define DRIVER_ADC_CHAN_ENUM_IDX(node_id, prop, idx)						\
	DT_ENUM_IDX(										\
		ADC_CHANNEL_DT_NODE(								\
			DT_IO_CHANNELS_CTLR_BY_IDX(node_id, idx),				\
			DT_IO_CHANNELS_INPUT_BY_IDX(node_id, idx)				\
		),										\
		prop										\
	)

#define DRIVER_ADC_CHAN_INIT(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx)

#define DRIVER_ADC_CHAN_MSG(node_id, idx, msg) \
	DT_NODE_FULL_NAME(node_id) ": io-channel " STRINGIFY(idx) ": " msg

#define DRIVER_ADC_CHAN_ASSERT_GAIN_IS_SET(node_id, idx)					\
	BUILD_ASSERT(										\
		DRIVER_ADC_CHAN_HAS_PROP(node_id, zephyr_gain, idx),				\
		DRIVER_ADC_CHAN_MSG(node_id, idx, "zephyr,gain is required")			\
	);

#define DRIVER_ADC_CHAN_ASSERT_VREF_MV_IS_VALID(node_id, idx)					\
	BUILD_ASSERT(										\
		DRIVER_ADC_CHAN_PROP(node_id, zephyr_vref_mv, idx) > 0,				\
		DRIVER_ADC_CHAN_MSG(node_id, idx, "zephyr,vref_mv is invalid")			\
	);											\
												\
	BUILD_ASSERT(										\
		DRIVER_ADC_CHAN_PROP(node_id, zephyr_vref_mv, idx) <= UINT16_MAX,		\
		DRIVER_ADC_CHAN_MSG(node_id, idx, "zephyr,vref_mv is invalid")			\
	);

#define DRIVER_ADC_CHAN_ASSERT_REFERENCE_IS_SET(node_id, idx)					\
	BUILD_ASSERT(										\
		DRIVER_ADC_CHAN_HAS_PROP(node_id, zephyr_reference, idx),			\
		DRIVER_ADC_CHAN_MSG(								\
			node_id,								\
			idx,									\
			"zephyr,vref_mv or zephyr,reference must be specified"			\
		)										\
	);

#define DRIVER_ADC_CHAN_ASSERT_REFERENCE_IS_INTERNAL(node_id, idx)				\
	BUILD_ASSERT(										\
		DRIVER_ADC_CHAN_ENUM_IDX(node_id, zephyr_reference, idx) == 4,			\
		DRIVER_ADC_CHAN_MSG(								\
			node_id,								\
			idx,									\
			"zephyr,reference must be ADC_REF_INTERNAL, or zephyr,vref "		\
			"must be specified"							\
		)										\
	);											\

#define DRIVER_ADC_CHAN_ASSERT_REFERENCE_IS_VALID(node_id, idx)					\
	DRIVER_ADC_CHAN_ASSERT_REFERENCE_IS_SET(node_id, idx)					\
												\
	IF_ENABLED(										\
		DRIVER_ADC_CHAN_HAS_PROP(node_id, zephyr_reference, idx),			\
		(DRIVER_ADC_CHAN_ASSERT_REFERENCE_IS_INTERNAL(node_id, idx))			\
	)

#define DRIVER_VALIDATE_ADC_CHAN_IDX(node_id, idx)						\
	DRIVER_ADC_CHAN_ASSERT_GAIN_IS_SET(node_id, idx)					\
												\
	COND_CODE_1(										\
		DRIVER_ADC_CHAN_HAS_PROP(node_id, zephyr_vref_mv, idx),				\
		(DRIVER_ADC_CHAN_ASSERT_VREF_MV_IS_VALID(node_id, idx)),			\
		(DRIVER_ADC_CHAN_ASSERT_REFERENCE_IS_VALID(node_id, idx))			\
	)

#define DRIVER_ADC_CHAN_ASSERT_RESOLUTION_IS_SET(node_id)					\
	BUILD_ASSERT(										\
		DRIVER_ADC_CHAN_HAS_PROP(node_id, zephyr_resolution, 0),			\
		DRIVER_ADC_CHAN_MSG(								\
			node_id,								\
			0,									\
			"zephyr,resolution is required"						\
		)										\
	);

#define DRIVER_ADC_CHAN_ASSERT_RESOLUTION_IS_VALID(node_id)					\
	IF_ENABLED(										\
		DRIVER_ADC_CHAN_HAS_PROP(node_id, zephyr_resolution, 0),			\
		(										\
			BUILD_ASSERT(								\
				DRIVER_ADC_CHAN_PROP(node_id, zephyr_resolution, 0) <= 16,	\
				DRIVER_ADC_CHAN_MSG(						\
					node_id,						\
					0,							\
					"zephyr,resolution must be <= 16"			\
				)								\
			);									\
												\
			BUILD_ASSERT(								\
				DRIVER_ADC_CHAN_PROP(node_id, zephyr_resolution, 0) > 0,	\
				DRIVER_ADC_CHAN_MSG(node_id,					\
					0,							\
					"zephyr,resolution must be > 0"				\
				)								\
			);									\
		)										\
	)

#define DRIVER_ADC_CHAN_ASSERT_OVERSAMPLING_IS_VALID(node_id)					\
	IF_ENABLED(										\
		DRIVER_ADC_CHAN_HAS_PROP(node_id, zephyr_oversampling, 0),			\
		(										\
			BUILD_ASSERT(								\
				DRIVER_ADC_CHAN_PROP(node_id, zephyr_oversampling, 0) <= 255,	\
				DRIVER_ADC_CHAN_MSG(						\
					node_id,						\
					0,							\
					"zephyr,resolution must be <= 16"			\
				)								\
			);									\
		)										\
	)

#define DRIVER_VALIDATE_ADC_CHAN_0(node_id)							\
	DRIVER_ADC_CHAN_ASSERT_RESOLUTION_IS_SET(node_id)					\
	DRIVER_ADC_CHAN_ASSERT_RESOLUTION_IS_VALID(node_id)					\
	DRIVER_ADC_CHAN_ASSERT_OVERSAMPLING_IS_VALID(node_id)					\
	DRIVER_VALIDATE_ADC_CHAN_IDX(node_id, 0)

#define DRIVER_VALIDATE_ADC_CHAN(node_id, prop, idx)						\
	COND_CODE_1(										\
		IS_EQ(idx, 0),									\
		(DRIVER_VALIDATE_ADC_CHAN_0(node_id)),						\
		(DRIVER_VALIDATE_ADC_CHAN_IDX(node_id, idx))					\
	)

#define DRIVER_SAMPLES_SIZE(inst) \
	(DRIVER_SAMPLE_SIZE * DT_INST_PROP_LEN(inst, io_channels))

#define DRIVER_SAMPLING_SIZE(inst) \
	(DRIVER_TIMESTAMP_TICKS_SIZE + DRIVER_SAMPLES_SIZE(inst))

#define DRIVER_FIFO_RINGQ_DATA_SIZE(inst) \
	(DRIVER_SAMPLING_SIZE(inst) * DT_INST_PROP(inst, fifo_depth))

#define DRIVER_DEFINE(inst)									\
	DT_INST_FOREACH_PROP_ELEM(inst, io_channels, DRIVER_VALIDATE_ADC_CHAN)			\
												\
	static struct driver_data CONCAT(data, inst);						\
												\
	static const struct adc_dt_spec CONCAT(channels, inst)[] = {				\
		DT_INST_FOREACH_PROP_ELEM_SEP(							\
			inst,									\
			io_channels,								\
			DRIVER_ADC_CHAN_INIT,							\
			(,)									\
		)										\
	};											\
												\
	static uint8_t __noinit __aligned(4) CONCAT(sampling_data, inst)			\
		[DRIVER_SAMPLING_SIZE(inst)];							\
												\
	static uint8_t __noinit __aligned(4) CONCAT(fifo_ringq_data, inst)			\
		[DRIVER_FIFO_RINGQ_DATA_SIZE(inst)];						\
												\
	static const struct driver_config CONCAT(config, inst) = {				\
		.sampling_data = CONCAT(sampling_data, inst),					\
		.fifo_ringq_data = CONCAT(fifo_ringq_data, inst),				\
		.channels = CONCAT(channels, inst),						\
		.channels_count = ARRAY_SIZE(CONCAT(channels, inst)),				\
		.calibrate = DT_INST_PROP(inst, calibrate),					\
		.fifo_depth = DT_INST_PROP(inst, fifo_depth),					\
		.fifo_watermark = DT_INST_PROP(inst, fifo_watermark),				\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, driver_pm_action);					\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(								\
		inst,										\
		driver_init,									\
		PM_DEVICE_DT_INST_GET(inst),							\
		&CONCAT(data, inst),								\
		&CONCAT(config, inst),								\
		POST_KERNEL,									\
		CONFIG_SENSOR_INIT_PRIORITY,							\
		&driver_api									\
	);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_DEFINE)
