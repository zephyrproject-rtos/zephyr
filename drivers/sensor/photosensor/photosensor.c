/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define DT_DRV_COMPAT photosensor

#define DRIVER_R_SQE_COUNT 3
#define DRIVER_R_CQE_COUNT 1
#define DRIVER_ADC_READ_BUF_SIZE 2
#define DRIVER_ADC_READ_CALIBRATE CONFIG_SENSOR_PHOTOSENSOR_ADC_READ_CALIBRATE
#define DRIVER_FIXED_POINT_SHIFT 17
#define DRIVER_NA_PER_UA 1000
#define DRIVER_PHOTOCURRENT_REF_LUX 100

struct driver_buffer_data {
	uint64_t base_timestamp_ns;
	int8_t shift;
	q31_t value;
};

struct driver_data {
	const struct device *dev;
	struct k_spinlock spinlock;
	struct mpsc iodev_sqe_q;
	struct k_timer timer;
	k_timepoint_t settled_timepoint;
	struct rtio_iodev_sqe *iodev_sqe;
	uint8_t buffer[sizeof(uint16_t)];
};

struct driver_config {
	struct gpio_dt_spec enable_pin;
	bool no_disconnect;
	struct adc_dt_spec adc_chan;
	k_timeout_t settle_timeout;
	uint32_t photocurrent_na;
	uint32_t load_resistance_ohm;
	struct adc_sequence_options sequence_options;
	struct adc_sequence sequence;
};

static void request_read(const struct device *dev);

static bool enable_pin_exists(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;

	return dev_config->enable_pin.port != NULL;
}

static void iodev_sqe_complete(const struct device *dev, int result)
{
	struct driver_data *dev_data = dev->data;
	struct mpsc_node *next;
	bool pending;

	if (result) {
		rtio_iodev_sqe_err(dev_data->iodev_sqe, result);
	} else {
		rtio_iodev_sqe_ok(dev_data->iodev_sqe, result);
	}

	K_SPINLOCK(&dev_data->spinlock) {
		next = mpsc_pop(&dev_data->iodev_sqe_q);

		pending = next != NULL;

		dev_data->iodev_sqe = pending
				    ? CONTAINER_OF(next, struct rtio_iodev_sqe, q)
				    : NULL;

		if (!pending) {
			pm_device_runtime_put(dev);
		}
	}

	if (pending) {
		request_read(dev);
	}
}

static enum adc_action read_done_callback(const struct device *adc,
					  const struct adc_sequence *sequence,
					  uint16_t sampling_index)
{
	const struct device *dev = sequence->options->user_data;
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	const struct adc_dt_spec *adc_chan = &dev_config->adc_chan;
	uint64_t base_timestamp_ns = k_ticks_to_ns_floor64(k_uptime_ticks());
	int ret;
	uint8_t *rx_buf;
	uint32_t rx_buf_len;
	struct driver_buffer_data *buffer_data;
	int32_t uv;
	int64_t lux;

	ARG_UNUSED(adc);
	ARG_UNUSED(sampling_index);

	ret = rtio_sqe_rx_buf(dev_data->iodev_sqe,
			      sizeof(struct driver_buffer_data),
			      sizeof(struct driver_buffer_data),
			      &rx_buf,
			      &rx_buf_len);
	if (ret) {
		iodev_sqe_complete(dev, -ENOMEM);
		return ADC_ACTION_CONTINUE;
	}

	uv = *((uint16_t *)dev_data->buffer);
	adc_raw_to_microvolts_dt(adc_chan, &uv);

	lux = uv;
	lux *= DRIVER_NA_PER_UA;
	lux *= DRIVER_PHOTOCURRENT_REF_LUX;
	lux <<= DRIVER_FIXED_POINT_SHIFT;
	lux /= dev_config->load_resistance_ohm;
	lux /= dev_config->photocurrent_na;

	buffer_data = (struct driver_buffer_data *)rx_buf;
	buffer_data->base_timestamp_ns = base_timestamp_ns;
	buffer_data->shift = 31 - DRIVER_FIXED_POINT_SHIFT;
	buffer_data->value = lux;

	iodev_sqe_complete(dev, 0);
	return ADC_ACTION_CONTINUE;
}

static void request_read(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	int ret;

	ret = adc_read_async_dt(&dev_config->adc_chan, &dev_config->sequence, NULL);
	if (ret) {
		iodev_sqe_complete(dev, ret);
	}
}

static void timer_expired_handler(struct k_timer *timer)
{
	struct driver_data *dev_data = CONTAINER_OF(timer, struct driver_data, timer);
	const struct device *dev = dev_data->dev;

	request_read(dev);
}

static int driver_decoder_get_frame_count(const uint8_t *buffer,
					  struct sensor_chan_spec channel,
					  uint16_t *frame_count)
{
	ARG_UNUSED(buffer);

	if (channel.chan_type != SENSOR_CHAN_AMBIENT_LIGHT || channel.chan_idx != 0) {
		*frame_count = 0;
		return -ENOENT;
	}

	*frame_count = 1;

	return 0;
}

static int driver_decoder_get_size_info(struct sensor_chan_spec channel,
					size_t *base_size,
					size_t *frame_size)
{
	ARG_UNUSED(channel);

	*base_size = sizeof(struct driver_buffer_data);
	*frame_size = sizeof(struct driver_buffer_data);

	return 0;
}

static int driver_decoder_decode(const uint8_t *buffer,
				 struct sensor_chan_spec channel,
				 uint32_t *fit,
				 uint16_t max_count,
				 void *data_out)
{
	const struct driver_buffer_data *buffer_data = (const struct driver_buffer_data *)buffer;
	struct sensor_q31_data *data = data_out;

	if (channel.chan_type != SENSOR_CHAN_AMBIENT_LIGHT || channel.chan_idx != 0) {
		return -ENOENT;
	}

	ARG_UNUSED(max_count);

	if (*fit) {
		return 0;
	}

	data->header.base_timestamp_ns = buffer_data->base_timestamp_ns;
	data->header.reading_count = 1;
	data->shift = buffer_data->shift;
	data->readings[0].light = buffer_data->value;

	*fit = 1;

	return 1;
}

static bool driver_decoder_has_trigger(const uint8_t *buffer, enum sensor_trigger_type trigger)
{
	ARG_UNUSED(buffer);
	ARG_UNUSED(trigger);

	return false;
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

static int validate_iodev_sqe(struct rtio_iodev_sqe *iodev_sqe)
{
	struct sensor_read_config *read_config = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *channel;

	if (read_config->is_streaming) {
		return -ENOTSUP;
	}

	for (size_t i = 0; i < read_config->count; i++) {
		channel = &read_config->channels[i];

		if (channel->chan_type == SENSOR_CHAN_AMBIENT_LIGHT && channel->chan_idx == 0) {
			return 0;
		}
	}

	return -ENOTSUP;
}

static void driver_api_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct driver_data *dev_data = dev->data;
	int ret;
	struct mpsc_node *next;
	bool running;

	ret = validate_iodev_sqe(iodev_sqe);
	if (ret) {
		rtio_iodev_sqe_err(iodev_sqe, ret);
		return;
	}

	mpsc_push(&dev_data->iodev_sqe_q, &iodev_sqe->q);

	K_SPINLOCK(&dev_data->spinlock) {
		running = dev_data->iodev_sqe != NULL;

		if (!running) {
			next = mpsc_pop(&dev_data->iodev_sqe_q);
			dev_data->iodev_sqe = CONTAINER_OF(next, struct rtio_iodev_sqe, q);
		}
	}

	if (!running) {
		pm_device_runtime_get(dev);
		k_timer_start(&dev_data->timer,
			      sys_timepoint_timeout(dev_data->settled_timepoint),
			      K_FOREVER);
	}
}

static DEVICE_API(sensor, driver_api) = {
	.submit = driver_api_submit,
	.get_decoder = driver_api_get_decoder,
};

static int resume(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;

	if (enable_pin_exists(dev) == false) {
		return 0;
	}

	dev_data->settled_timepoint = sys_timepoint_calc(dev_config->settle_timeout);

	return gpio_pin_configure_dt(&dev_config->enable_pin, GPIO_OUTPUT_ACTIVE);
}

static int suspend(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	uint32_t flags;

	if (enable_pin_exists(dev) == false) {
		return 0;
	}

	flags = dev_config->no_disconnect
	      ? GPIO_INPUT
	      : GPIO_INPUT | GPIO_DISCONNECTED;

	return gpio_pin_configure_dt(&dev_config->enable_pin, flags);
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
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	int ret;

	mpsc_init(&dev_data->iodev_sqe_q);
	k_timer_init(&dev_data->timer, timer_expired_handler, NULL);

	if (enable_pin_exists(dev)) {
		ret = gpio_is_ready_dt(&dev_config->enable_pin) ? 0 : -ENODEV;
		if (ret) {
			return ret;
		}
	}

	ret = adc_is_ready_dt(&dev_config->adc_chan) ? 0 : -ENODEV;
	if (ret) {
		return ret;
	}

	ret = adc_channel_setup_dt(&dev_config->adc_chan);
	if (ret) {
		return ret;
	}

	return pm_device_driver_init(dev, driver_pm_action);
}

#if CONFIG_DEVICE_DEINIT_SUPPORT
static int driver_deinit(const struct device *dev)
{
	return pm_device_driver_deinit(dev, driver_pm_action);
}
#endif

#define DRIVER_ADC_CHANNEL_HAS_PROP(inst, _prop)						\
	DT_NODE_HAS_PROP(									\
		ADC_CHANNEL_DT_NODE(								\
			DT_INST_IO_CHANNELS_CTLR(inst),						\
			DT_INST_IO_CHANNELS_INPUT(inst)						\
		),										\
		_prop										\
	)

#define DRIVER_ADC_CHANNEL_PROP(inst, _prop)							\
	DT_PROP(										\
		ADC_CHANNEL_DT_NODE(								\
			DT_INST_IO_CHANNELS_CTLR(inst),						\
			DT_INST_IO_CHANNELS_INPUT(inst)						\
		),										\
		_prop										\
	)

#define DRIVER_ADC_RESOLUTION(inst) \
	DRIVER_ADC_CHANNEL_PROP(inst, zephyr_resolution)

#define DRIVER_ADC_VREF_MV(inst) \
	DRIVER_ADC_CHANNEL_PROP(inst, zephyr_vref_mv)

#define DRIVER_ADC_OVERSAMPLING(inst) \
	DRIVER_ADC_CHANNEL_PROP(inst, zephyr_oversampling)

#define DRIVER_DEFINE(inst)									\
	BUILD_ASSERT(DRIVER_ADC_CHANNEL_HAS_PROP(inst, zephyr_resolution));			\
	BUILD_ASSERT(DRIVER_ADC_CHANNEL_PROP(inst, zephyr_resolution) <= 16);			\
	BUILD_ASSERT(DRIVER_ADC_CHANNEL_HAS_PROP(inst, zephyr_oversampling));			\
	BUILD_ASSERT(DT_INST_PROP(inst, photocurrent_na) > 1);					\
	BUILD_ASSERT(DT_INST_PROP(inst, load_resistance_ohm) > 1);				\
												\
	static struct driver_data CONCAT(data, inst) = {					\
		.dev = DEVICE_DT_INST_GET(inst),						\
	};											\
												\
	static const struct driver_config CONCAT(config, inst) = {				\
		.enable_pin = GPIO_DT_SPEC_INST_GET_OR(inst, enable_gpios, {0}),		\
		.no_disconnect = DT_INST_PROP(inst, no_disconnect),				\
		.adc_chan = ADC_DT_SPEC_INST_GET(inst),						\
		.settle_timeout = K_NSEC(DT_INST_PROP_OR(inst, settle_time_ns, 0)),		\
		.photocurrent_na = DT_INST_PROP(inst, photocurrent_na),				\
		.load_resistance_ohm = DT_INST_PROP(inst, load_resistance_ohm),			\
		.sequence_options = {								\
			.callback = read_done_callback,						\
			.user_data = (void *)DEVICE_DT_INST_GET(inst),				\
		},										\
		.sequence = {									\
			.options = &CONCAT(config, inst).sequence_options,			\
			.channels = BIT(DT_INST_IO_CHANNELS_INPUT(inst)),			\
			.buffer = CONCAT(data, inst).buffer,					\
			.buffer_size = sizeof(CONCAT(data, inst).buffer),			\
			.resolution = DRIVER_ADC_RESOLUTION(inst),				\
			.oversampling = DRIVER_ADC_OVERSAMPLING(inst),				\
			.calibrate = DRIVER_ADC_READ_CALIBRATE,					\
		},										\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, driver_pm_action);					\
												\
	SENSOR_DEVICE_DT_INST_DEINIT_DEFINE(							\
		inst,										\
		driver_init,									\
		driver_deinit,									\
		PM_DEVICE_DT_INST_GET(inst),							\
		&CONCAT(data, inst),								\
		&CONCAT(config, inst),								\
		POST_KERNEL,									\
		CONFIG_SENSOR_INIT_PRIORITY,							\
		&driver_api									\
	);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_DEFINE)
