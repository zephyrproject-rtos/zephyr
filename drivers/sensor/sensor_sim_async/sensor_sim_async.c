/*
 * Copyright (c) 2025 COGNID Telematik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Simulated sensor with async API.
 */

#define DT_DRV_COMPAT zephyr_sensor_sim_async

#include <math.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/sensor/sensor_sim_async.h>
#include <zephyr/drivers/sensor.h>

#define RING_BUF_ITEM_SIZE                                                                         \
	ROUND_UP(sizeof(struct sensor_sim_async_sensor_fifo_sample), sizeof(uint32_t))
#define RING_BUF_ITEM_SIZE_WITH_HEADER                                                             \
	ROUND_UP(sizeof(struct sensor_sim_async_sensor_fifo_sample) +                              \
			 sizeof(uint32_t) /*ringbuf header*/,                                      \
		 sizeof(uint32_t))

LOG_MODULE_REGISTER(sensor_sim_async, CONFIG_SENSOR_LOG_LEVEL);

struct sensor_sim_async_data {
	struct rtio_iodev_sqe *streaming_sqe;
	struct ring_buf sensor_fifo;
	uint32_t odr_period_us;
	sensor_trigger_handler_t trigger_callbacks[SENSOR_TRIG_COMMON_COUNT];
	uint64_t current_fifo_timestamp;
	uint16_t trigger_happened_bitfield;
	uint16_t stream_triggers;
	int16_t fallback_value;
	enum sensor_channel fifo_channel;
	int16_t channel_values[SENSOR_CHAN_COMMON_COUNT];
};

struct sensor_sim_async_config {
	uint32_t fifo_wm;
	uint32_t fifo_capacity;
	uint32_t *fifo_buffer;
} __aligned(4);

struct sensor_sim_async_encoded_data {
	uint64_t timestamp_ns;
	uint32_t period_us;
	uint16_t count;
	uint16_t trigger_happened_bitfield;
	int16_t fallback_value; /* A different channel from <channel> will return this value */
	enum sensor_channel channel;
	uint8_t _padding[2];
	struct sensor_sim_async_sensor_fifo_sample readings[0];
} __packed __aligned(sizeof(void *));

BUILD_ASSERT(offsetof(struct sensor_sim_async_encoded_data, readings) % sizeof(uint32_t) == 0,
	     "Encoded data not aligned to uint32_t");

BUILD_ASSERT(SENSOR_TRIG_COMMON_COUNT <=
		     SIZEOF_FIELD(struct sensor_sim_async_encoded_data, trigger_happened_bitfield) *
			     8,
	     "Bifield array too small");

static int sensor_sim_async_init(const struct device *dev)
{
	const struct sensor_sim_async_config *config = dev->config;
	struct sensor_sim_async_data *data = dev->data;

	uint32_t ring_buf_size = config->fifo_capacity *
				 DIV_ROUND_UP(RING_BUF_ITEM_SIZE_WITH_HEADER, sizeof(uint32_t));

	ring_buf_item_init(&data->sensor_fifo, ring_buf_size, config->fifo_buffer);

	return 0;
}

int sensor_sim_async_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				 sensor_trigger_handler_t handler)
{
	struct sensor_sim_async_data *data = dev->data;

	if (trig->type < SENSOR_TRIG_COMMON_COUNT) {
		data->trigger_callbacks[trig->type] = handler;
	}

	return 0;
}

int sensor_sim_async_attr_set(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	struct sensor_sim_async_data *data = dev->data;

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY: {
		data->odr_period_us = 1000000ULL * 1000000ULL / sensor_value_to_micro(val);
		break;
	}
	default:
		return -ENOTSUP;
	}
	return 0;
}

int sensor_sim_async_attr_get(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, struct sensor_value *val)
{
	struct sensor_sim_async_data *data = dev->data;

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY: {
		return sensor_value_from_micro(val, 1000000ULL * 1000000ULL / data->odr_period_us);
	}
	default:
		return -ENOTSUP;
	}
}

static void submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *sqe)
{
	struct sensor_sim_async_data *data = dev->data;
	struct sensor_read_config *read_config = sqe->sqe.iodev->data;
	const uint32_t min_buf_len =
		sizeof(struct sensor_sim_async_encoded_data) +
		SIZEOF_FIELD(struct sensor_sim_async_encoded_data, readings[0]);
	uint8_t *buf;
	uint32_t buf_len;

	int rc = rtio_sqe_rx_buf(sqe, min_buf_len, min_buf_len, &buf, &buf_len);

	if (rc != 0) {
		LOG_ERR("Failed to get buffer for sensor data");
		return;
	}

	LOG_DBG("Encode one sample");

	struct sensor_sim_async_encoded_data *edata = (struct sensor_sim_async_encoded_data *)buf;

	edata->period_us = data->odr_period_us;
	edata->timestamp_ns = data->current_fifo_timestamp;
	edata->fallback_value = data->fallback_value;
	edata->count = 1;
	edata->trigger_happened_bitfield = data->trigger_happened_bitfield;
	data->trigger_happened_bitfield = 0;
	edata->channel = read_config->channels[0].chan_type;
	edata->readings[0].x = data->channel_values[read_config->channels[0].chan_type - 3];
	edata->readings[0].y = data->channel_values[read_config->channels[0].chan_type - 2];
	edata->readings[0].z = data->channel_values[read_config->channels[0].chan_type - 1];

	rtio_iodev_sqe_ok(sqe, 0);
}

/**
 * On a real sensor, this function would be triggered by the fifo watermark/full
 * interrupt.
 */
static void fifo_callback(const struct device *dev)
{
	struct sensor_sim_async_data *data = dev->data;
	struct rtio_iodev_sqe *sqe = data->streaming_sqe;

	data->streaming_sqe = NULL;
	if (sqe == NULL) {
		LOG_DBG("No pending SQE");
		return;
	}

	struct sensor_read_config *read_config = sqe->sqe.iodev->data;

	if (!read_config->is_streaming) {
		LOG_DBG("Not a streaming SQE");
		return;
	}

	uint8_t *buf;
	uint32_t buf_len;

	const uint32_t min_read_size =
		sizeof(struct sensor_sim_async_encoded_data) +
		SIZEOF_FIELD(struct sensor_sim_async_encoded_data, readings[0]);
	const uint32_t ideal_read_size = sizeof(struct sensor_sim_async_encoded_data) +
					 ring_buf_size_get(&data->sensor_fifo);

	if (rtio_sqe_rx_buf(sqe, min_read_size, ideal_read_size, &buf, &buf_len) != 0) {
		LOG_ERR("Failed to get buffer [%d/%d]", min_read_size, ideal_read_size);
		rtio_iodev_sqe_err(sqe, -ENOMEM);
		return;
	}
	if (buf_len < min_read_size) {
		LOG_ERR("Buffer too small [%d < %d/%d]", buf_len, min_read_size, ideal_read_size);
		rtio_release_buffer(sqe->r, buf, buf_len);
		rtio_iodev_sqe_err(sqe, -ENOMEM);
		return;
	}
	LOG_DBG("Requested buffer of size [%u, %u] got %u", (unsigned int)min_read_size,
		(unsigned int)ideal_read_size, buf_len);

	/* Read sensor data, best with RTIO enabled bus (which we do not have)
	 * which would result in an RTIO SQE for the bus driver and a callback
	 * after finished
	 */
	enum sensor_stream_data_opt preceding_opt = SENSOR_STREAM_DATA_DROP;

	for (int i = 0; i < read_config->count; i++) {
		if (read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_FULL ||
		    read_config->triggers[i].trigger == SENSOR_TRIG_FIFO_WATERMARK) {
			preceding_opt = MIN(preceding_opt, read_config->triggers[i].opt);
		}
	}

	if (preceding_opt == SENSOR_STREAM_DATA_DROP || preceding_opt == SENSOR_STREAM_DATA_NOP) {
		/* Not interested in the actual data, just drop it */
		uint8_t *dummy;
		uint32_t claimed = ring_buf_get_claim(&data->sensor_fifo, &dummy, UINT32_MAX);

		ring_buf_get_finish(&data->sensor_fifo, claimed);
		LOG_DBG("Dropped %d bytes from FIFO", claimed);
		return;
	}

	struct sensor_sim_async_encoded_data *edata = (struct sensor_sim_async_encoded_data *)buf;
	uint16_t type;
	uint8_t value;
	uint32_t avail_buf_len = buf_len;
	int num_samples = 0;

	while (avail_buf_len >= sizeof(edata->readings[0])) {
		uint8_t size = CLAMP(avail_buf_len, 0, UINT8_MAX);
		int rc = ring_buf_item_get(&data->sensor_fifo, &type, &value,
					   (uint32_t *)&edata->readings[num_samples], &size);
		if (rc < 0) {
			break;
		}
		num_samples++;
		avail_buf_len -= size * 4;
	}
	/* If there are samples left in the FIFO, the current_fifo_timestamp has
	 * to be adjusted as we are not reading the most recent samples
	 */
	int avail_samples_ringbuf =
		ring_buf_size_get(&data->sensor_fifo) / RING_BUF_ITEM_SIZE_WITH_HEADER;

	edata->channel = data->fifo_channel;
	edata->fallback_value = data->fallback_value;
	edata->period_us = data->odr_period_us;
	edata->timestamp_ns = data->current_fifo_timestamp -
			      edata->period_us * (num_samples - 1 - avail_samples_ringbuf) * 1000;
	edata->count = num_samples;
	edata->trigger_happened_bitfield = data->trigger_happened_bitfield;
	data->trigger_happened_bitfield = 0;

	LOG_DBG("Encoded %u values to buffer %p with triggers %x", edata->count, edata,
		edata->trigger_happened_bitfield);

	/* Callback of completed bus transaction: */
	rtio_iodev_sqe_ok(sqe, 0);
}

void sensor_sim_async_trigger(const struct device *dev, enum sensor_trigger_type trigger)
{
	struct sensor_sim_async_data *data = dev->data;

	if (trigger < SENSOR_TRIG_COMMON_COUNT && data->trigger_callbacks[trigger]) {
		struct sensor_trigger t = {trigger, SENSOR_CHAN_ALL};

		data->trigger_callbacks[trigger](dev, &t);
	}
	sys_bitfield_set_bit((mem_addr_t)&data->trigger_happened_bitfield, trigger);

	if (sys_bitfield_test_bit((mem_addr_t)&data->stream_triggers, trigger)) {
		fifo_callback(dev);
	}
}

void sensor_sim_async_flush_fifo(const struct device *dev)
{
	struct sensor_sim_async_data *data = dev->data;

	ring_buf_reset(&data->sensor_fifo);
	data->fifo_channel = 0;
}

void sensor_sim_async_feed_data(const struct device *dev,
				const struct sensor_sim_async_sensor_fifo_sample test_data[],
				uint32_t count, int64_t start_ns, enum sensor_channel chan)
{
	struct sensor_sim_async_data *data = dev->data;
	const struct sensor_sim_async_config *cfg = dev->config;

	if (start_ns >= 0) {
		data->current_fifo_timestamp = start_ns;
	}

	if (data->fifo_channel && data->fifo_channel != chan) {
		LOG_WRN("FIFO was fed with different channel before!");
	}
	data->fifo_channel = chan;

	if (!data->streaming_sqe && count > 1) {
		LOG_WRN("Sensor is not streaming while more than one sample is "
			"fed, this is probably not intended");
	}

	for (int i = 0; i < count; ++i) {
		/* Calculate next timestamp if start_ns is not provided or for
		 * subsequent iterations
		 */
		if (start_ns < 0 || i != 0) {
			data->current_fifo_timestamp += data->odr_period_us * 1000;
		}
		int rc;

		while ((rc = ring_buf_item_put(&data->sensor_fifo, 0, 0, (uint32_t *)&test_data[i],
					       RING_BUF_ITEM_SIZE / sizeof(uint32_t))) < 0) {
			LOG_WRN("Sensor FIFO overflow, failed to put buffer "
				"item %d (size = %d bytes, space %d items)",
				rc, ring_buf_size_get(&data->sensor_fifo),
				ring_buf_item_space_get(&data->sensor_fifo));
			uint16_t type;
			uint8_t value;
			uint32_t buf[RING_BUF_ITEM_SIZE / sizeof(uint32_t)];
			uint8_t size = ARRAY_SIZE(buf);

			rc = ring_buf_item_get(&data->sensor_fifo, &type, &value, buf, &size);
			__ASSERT(rc == 0, "Last item could not be removed from ringbuffer");
		}

		/* save last sample for fetch + get API / one-shot read */
		if (SENSOR_CHANNEL_3_AXIS(chan)) {
			data->channel_values[chan - 3] = test_data[i].x;
			data->channel_values[chan - 2] = test_data[i].y;
			data->channel_values[chan - 1] = test_data[i].z;
		} else {
			data->channel_values[chan] = test_data[i].val[0];
		}

		/* Trigger watermark/full triggers if needed */
		if (ring_buf_size_get(&data->sensor_fifo) / RING_BUF_ITEM_SIZE_WITH_HEADER ==
		    cfg->fifo_wm) {
			sensor_sim_async_trigger(dev, SENSOR_TRIG_FIFO_WATERMARK);
		}
		if (ring_buf_item_space_get(&data->sensor_fifo) == 0) {
			sensor_sim_async_trigger(dev, SENSOR_TRIG_FIFO_FULL);
		}

		/* Wait until it's time for the next sample
		 * (but always sleep to let other threads run)
		 */
		__ASSERT(data->odr_period_us != 0, "ODR is 0");
		int64_t sleep_time_ns =
			data->current_fifo_timestamp - k_ticks_to_ns_floor64(k_uptime_ticks());
		if (sleep_time_ns > 0) {
			k_sleep(K_NSEC(MAX(1, sleep_time_ns)));
		}
	}
}

int sensor_sim_async_set_channel(const struct device *dev, enum sensor_channel chan, float value)
{
	struct sensor_sim_async_data *data = dev->data;

	if (chan >= SENSOR_CHAN_COMMON_COUNT || SENSOR_CHANNEL_3_AXIS(chan)) {
		return -EINVAL;
	}
	__ASSERT(fabs(value) <= INT16_MAX / CONFIG_SENSOR_SIM_ASYNC_SCALE,
		 "Value too big for sensor scale");
	data->channel_values[chan] = round(value * CONFIG_SENSOR_SIM_ASYNC_SCALE);
	return 0;
}

void sensor_sim_async_set_fallback_value(const struct device *dev, float value)
{
	struct sensor_sim_async_data *data = dev->data;

	data->fallback_value = round(value * CONFIG_SENSOR_SIM_ASYNC_SCALE);
}

static void submit_stream(const struct device *sensor, struct rtio_iodev_sqe *sqe)
{
	struct sensor_sim_async_data *data = sensor->data;
	const struct sensor_read_config *cfg = sqe->sqe.iodev->data;

	volatile uint16_t new_config = 0;
	/* without volatile the compiler thinks new_config is not modified in
	 * sys_bitfield_set_bit() and just assigns 0 to data->stream_triggers
	 */
	for (int i = 0; i < cfg->count; ++i) {
		sys_bitfield_set_bit((mem_addr_t)&new_config, cfg->triggers[i].trigger);
	}
	data->stream_triggers = new_config;

	data->streaming_sqe = sqe;
}

void sensor_sim_async_submit(const struct device *sensor, struct rtio_iodev_sqe *sqe)
{
	const struct sensor_read_config *cfg = sqe->sqe.iodev->data;

	if (cfg->is_streaming) {
		submit_stream(sensor, sqe);
	} else {
		submit_one_shot(sensor, sqe);
	}
}

static int sensor_sim_async_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);

	/* Nothing to do */
	return 0;
}

static void sensor_value_from_scaled(struct sensor_value *val, int16_t scaled)
{
	val->val1 = scaled / CONFIG_SENSOR_SIM_ASYNC_SCALE;
	val->val2 =
		scaled % CONFIG_SENSOR_SIM_ASYNC_SCALE * 1000000 / CONFIG_SENSOR_SIM_ASYNC_SCALE;
}

static int sensor_sim_async_channel_get(const struct device *dev, enum sensor_channel chan,
					struct sensor_value *val)
{
	struct sensor_sim_async_data *data = dev->data;

	if (chan >= SENSOR_CHAN_COMMON_COUNT) {
		return -ENOTSUP;
	}

	if (SENSOR_CHANNEL_3_AXIS(chan)) {
		sensor_value_from_scaled(&val[0], data->channel_values[chan - 3]);
		sensor_value_from_scaled(&val[1], data->channel_values[chan - 2]);
		sensor_value_from_scaled(&val[2], data->channel_values[chan - 1]);
	} else {
		sensor_value_from_scaled(val, data->channel_values[chan]);
	}

	return 0;
}

static int sensor_sim_async_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
					   uint32_t *fit, uint16_t max_count, void *data_out)
{
	struct sensor_sim_async_encoded_data *edata =
		(struct sensor_sim_async_encoded_data *)buffer;

	if (SENSOR_CHANNEL_3_AXIS(chan_spec.chan_type)) {
		struct sensor_three_axis_data *out = data_out;
		int output_count = CLAMP((int)max_count, 0, (int)(edata->count - *fit));

		out->header.base_timestamp_ns = edata->timestamp_ns;
		out->header.reading_count = 0;
		out->shift = 31 - LOG2(CONFIG_SENSOR_SIM_ASYNC_SCALE);
		for (int i = 0; i < output_count; ++i, ++*fit) {
			out->header.reading_count++;
			out->readings[i].timestamp_delta = edata->period_us * *fit * 1000;
			if (chan_spec.chan_type == edata->channel) {
				out->readings[i].x = edata->readings[*fit].x;
				out->readings[i].y = edata->readings[*fit].y;
				out->readings[i].z = edata->readings[*fit].z;
			} else {
				out->readings[i].x = edata->fallback_value;
				out->readings[i].y = edata->fallback_value;
				out->readings[i].z = edata->fallback_value;
			}
		}
	} else {
		struct sensor_q31_data *out = data_out;

		out->header.base_timestamp_ns = edata->timestamp_ns;
		out->header.reading_count = 0;
		out->shift = 31 - LOG2(CONFIG_SENSOR_SIM_ASYNC_SCALE);
		for (int i = 0; i < MIN(max_count, edata->count - *fit); ++i, ++*fit) {
			out->header.reading_count++;
			out->readings[i].timestamp_delta = edata->period_us * *fit * 1000;
			if (chan_spec.chan_type == edata->channel) {
				out->readings[i].value = edata->readings[*fit].val[0];
			} else {
				out->readings[i].value = edata->fallback_value;
			}
		}
	}

	return 0;
}

static int sensor_sim_async_decoder_get_frame_count(const uint8_t *buffer,
						    struct sensor_chan_spec chan_spec,
						    uint16_t *frame_count)
{
	struct sensor_sim_async_encoded_data *edata =
		(struct sensor_sim_async_encoded_data *)buffer;

	*frame_count = edata->count;
	return 0;
}

static bool sensor_sim_async_decoder_has_trigger(const uint8_t *buffer,
						 enum sensor_trigger_type trigger)
{
	struct sensor_sim_async_encoded_data *edata =
		(struct sensor_sim_async_encoded_data *)buffer;

	return sys_bitfield_test_bit((mem_addr_t)&edata->trigger_happened_bitfield, trigger);
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = sensor_sim_async_decoder_get_frame_count,
	.get_size_info = sensor_natively_supported_channel_size_info,
	.decode = sensor_sim_async_decoder_decode,
	.has_trigger = sensor_sim_async_decoder_has_trigger,
};

int sensor_sim_async_get_decoder(const struct device *dev,
				 const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}

static const struct sensor_driver_api sensor_sim_async_api_funcs = {
	.attr_get = sensor_sim_async_attr_get,
	.attr_set = sensor_sim_async_attr_set,
	.trigger_set = sensor_sim_async_trigger_set,
	.sample_fetch = sensor_sim_async_sample_fetch,
	.channel_get = sensor_sim_async_channel_get,
	.submit = sensor_sim_async_submit,
	.get_decoder = sensor_sim_async_get_decoder};

#define SENSOR_SIM_ASYNC_DEFINE(_idx)                                                              \
	static uint32_t ring_buffer_##_idx[DT_INST_PROP(_idx, fifo_capacity) *                     \
					     DIV_ROUND_UP(RING_BUF_ITEM_SIZE_WITH_HEADER,          \
							  sizeof(uint32_t))];                      \
	static struct sensor_sim_async_data data_##_idx = {.odr_period_us = 10 * 1000};            \
	const struct sensor_sim_async_config config_##_idx = {                                     \
		.fifo_wm = DT_INST_PROP(_idx, fifo_wm),                                            \
		.fifo_capacity = DT_INST_PROP(_idx, fifo_capacity),                                \
		.fifo_buffer = ring_buffer_##_idx,                                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(_idx, sensor_sim_async_init, NULL, &data_##_idx, &config_##_idx,     \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                            \
			      &sensor_sim_async_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_SIM_ASYNC_DEFINE)
