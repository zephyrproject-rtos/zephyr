/*
 * Copyright (c) 2026 Zephyr Project Developers
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT avia_hx711_spi

#include <zephyr/drivers/sensor/hx711.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "hx711_spi.h"

LOG_MODULE_REGISTER(HX711, CONFIG_SENSOR_LOG_LEVEL);

/* Max sample rate of HX711 is 145 Hz
 * (assuming max external clock speed 20 MHz)
 */
#define HX711_SLEEP_BETWEEN_POLLING_MS 6

static int hx711_spi_read_sample(const struct device *dev, int32_t *sample)
{
	const struct hx711_config *config = dev->config;
	struct hx711_data *data = dev->data;

	/*
	 * Send out a clock pulse sequence through MOSI to HX711.
	 * The first 48 bits are 101010... repeating 24 times, for reading
	 * out the last 24-bit sample.
	 * The last 8 bits are trailing clock pulses that set the gain of
	 * the next sample.
	 */

	uint8_t tx_buffer[7] = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x00};
	uint8_t rx_buffer[6] = {0};
	int ret;

	switch (data->gain) {
	case 64:
		tx_buffer[6] = HX711_CHA_GAIN_64;
		break;
	case 128:
		tx_buffer[6] = HX711_CHA_GAIN_128;
		break;
	default:
		LOG_ERR("Unsupported gain.");
		return -ENOTSUP;
	}

	const struct spi_buf tx_buf = {.buf = tx_buffer, .len = sizeof(tx_buffer)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	struct spi_buf rx_buf[2] = {{.buf = rx_buffer, .len = sizeof(rx_buffer)},
				    {.buf = NULL, .len = 1}};
	const struct spi_buf_set rx = {.buffers = rx_buf, .count = 2};

	ret = spi_transceive_dt(&config->spi, &tx, &rx);

	if (ret < 0) {
		return ret;
	}

	/*
	 * The received data in the buffer has each bit doubled, i.e.
	 * 11110011... should be 1101...
	 * Need to "squash" it to recover the actual value.
	 */

	if (!sample) {
		return 0;
	}

	*sample = 0;

	for (int i = 0; i < sizeof(rx_buffer); ++i) {
		uint8_t b = rx_buffer[i];

		*sample <<= 4;
		*sample |= (b & 0x1) | (((b >> 3) & 0x1) << 1) | (((b >> 5) & 0x1) << 2) |
			   (((b >> 7) & 0x1) << 3);
	}

	/* Discard unfilled bits and recover the sign */
	*sample = (*sample << 8) >> 8;

	return 0;
}

static int hx711_sample_fetch_impl(const struct device *dev)
{
	const struct hx711_config *config = dev->config;
	struct hx711_data *data = dev->data;
	int32_t sample;
	int ret;

	ret = hx711_spi_read_sample(dev, &sample);

	if (ret < 0) {
		LOG_ERR("Failed to fetch sample.");
		return ret;
	}

	data->sample_uv = ((int64_t)sample * config->avdd_uv / HX711_SAMPLE_MAX) / 2 / data->gain;

	return 0;
}

#if defined(CONFIG_HX711_SPI_TRIGGER)
static int hx711_configure_gpio_interrupt(const struct device *dev, bool enable)
{
	const struct hx711_config *config = dev->config;
	struct hx711_data *data = dev->data;
	int ret;

	if (!config->trig_cfg) {
		LOG_ERR("trigger not configured for this device");
		return -ENOTSUP;
	}

	if (enable) {
		ret = pinctrl_apply_state(config->trig_cfg->pcfg, PINCTRL_STATE_TRIGGER);
		if (ret < 0) {
			LOG_ERR("could not apply pin config, return code %d", ret);
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->trig_cfg->int_gpio,
						      GPIO_INT_EDGE_FALLING);
		if (ret < 0) {
			LOG_ERR("could not set up gpio interrupt, return code %d", ret);
			return ret;
		}

		data->trig_data->trigger_armed = true;
	} else {
		ret = gpio_pin_interrupt_configure_dt(&config->trig_cfg->int_gpio,
						      GPIO_INT_DISABLE);
		if (ret < 0) {
			LOG_ERR("could not set up gpio interrupt, return code %d", ret);
			return ret;
		}

		ret = pinctrl_apply_state(config->trig_cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("could not apply pin config, return code %d", ret);
			return ret;
		}

		data->trig_data->trigger_armed = false;
	}

	return 0;
}

static void hx711_gpio_irq_handler(const struct device *port, struct gpio_callback *cb,
				   gpio_port_pins_t pins)
{
	struct hx711_trigger_data *trig_data = CONTAINER_OF(cb, struct hx711_trigger_data, gpio_cb);

	hx711_configure_gpio_interrupt(trig_data->dev, false);

#if defined(CONFIG_HX711_SPI_TRIGGER_OWN_THREAD)
	k_work_submit_to_queue(&trig_data->work_q, &trig_data->work);
#else
	k_work_submit(&trig_data->work);
#endif
}

static void hx711_gpio_irq_work_handler(struct k_work *item)
{
	struct hx711_trigger_data *trig_data = CONTAINER_OF(item, struct hx711_trigger_data, work);
	const struct device *dev = trig_data->dev;

	if (trig_data->data_ready_handler != NULL) {
		trig_data->data_ready_handler(dev, trig_data->data_ready_trigger);
	}

	hx711_configure_gpio_interrupt(dev, true);
}

static int hx711_gpio_irq_init(const struct device *dev)
{
	const struct hx711_config *config = dev->config;
	struct hx711_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&config->trig_cfg->int_gpio)) {
		LOG_ERR("gpio %s is not ready", config->trig_cfg->int_gpio.port->name);
		return -EIO;
	}

	gpio_init_callback(&data->trig_data->gpio_cb, hx711_gpio_irq_handler,
			   BIT(config->trig_cfg->int_gpio.pin));

	ret = gpio_add_callback(config->trig_cfg->int_gpio.port, &data->trig_data->gpio_cb);
	if (ret != 0) {
		LOG_ERR("Failed at gpio_add_callback_dt for int_gpio, return code %d", ret);
		return ret;
	}

	data->trig_data->dev = dev;

#if defined(CONFIG_HX711_SPI_TRIGGER_OWN_THREAD)
	k_work_queue_init(&data->trig_data->work_q);
	k_work_queue_start(&data->trig_data->work_q, data->trig_data->thread_stack,
			   CONFIG_HX711_SPI_THREAD_STACK_SIZE,
			   K_PRIO_COOP(CONFIG_HX711_SPI_THREAD_PRIORITY), NULL);
#endif

	data->trig_data->work.handler = hx711_gpio_irq_work_handler;

	return 0;
}

static int hx711_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			     sensor_trigger_handler_t handler)
{
	const struct hx711_config *config = dev->config;
	struct hx711_data *data = dev->data;

	if (!config->trig_cfg->int_gpio.port || trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	hx711_configure_gpio_interrupt(dev, false);

	data->trig_data->data_ready_handler = handler;
	data->trig_data->data_ready_trigger = trig;

	if (handler) {
		hx711_configure_gpio_interrupt(dev, true);
	}

	return 0;
}
#endif

static int hx711_sample_wait(const struct device *dev)
{
	const struct hx711_config *config = dev->config;

	uint8_t tx_buffer = 0;
	uint8_t rx_buffer = 0;
	int ret;

	const struct spi_buf tx_buf = {.buf = &tx_buffer, .len = 1};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	struct spi_buf rx_buf = {.buf = &rx_buffer, .len = 1};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

	/* Don't wait forever */
	k_timepoint_t end = sys_timepoint_calc(K_SECONDS(1));

	while (true) {
		ret = spi_transceive_dt(&config->spi, &tx, &rx);

		if (ret < 0) {
			return ret;
		}

		if (rx_buffer == 0) {
			/* MISO low => data ready */
			break;
		}

		if (sys_timepoint_expired(end)) {
			return -EAGAIN;
		}

		k_msleep(HX711_SLEEP_BETWEEN_POLLING_MS);
	}

	return 0;
}

static int hx711_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_VOLTAGE ||
			chan == (enum sensor_channel)SENSOR_CHAN_HX711_MASS);

#if defined(CONFIG_HX711_SPI_TRIGGER)
	struct hx711_data *data = dev->data;

	if (data->trig_data->trigger_armed) {
		LOG_ERR("cannot fetch when waiting for trigger");
		return -EAGAIN;
	}
#endif
	ret = hx711_sample_wait(dev);

	if (ret < 0) {
		return ret;
	}

	ret = hx711_sample_fetch_impl(dev);

#if defined(CONFIG_HX711_SPI_TRIGGER)
	if (data->trig_data->data_ready_handler) {
		hx711_configure_gpio_interrupt(dev, true);
	}
#endif

	return ret;
}

static int hx711_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct hx711_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		val->val1 = 0;
		val->val2 = data->sample_uv;
		break;
	case SENSOR_CHAN_HX711_MASS:
		val->val1 = (data->sample_uv - data->offset) * data->conv_factor_uv_to_g;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int hx711_attr_set(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, const struct sensor_value *val)
{
	struct hx711_data *data = dev->data;
	int ret = 0;

	switch ((uint32_t)attr) {
	case SENSOR_ATTR_GAIN:
		if (val->val1 == HX711_GAIN_128) {
			data->gain = 128;
		} else if (val->val1 == HX711_GAIN_64) {
			data->gain = 64;
		} else {
			return -ENOTSUP;
		}
		break;
	case SENSOR_ATTR_HX711_CONV_FACTOR:
		data->conv_factor_uv_to_g = val->val1;
		break;
	case SENSOR_ATTR_HX711_OFFSET:
		data->offset = val->val1;
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

static DEVICE_API(sensor, hx711_driver_api) = {
	.sample_fetch = hx711_sample_fetch,
	.channel_get = hx711_channel_get,
	.attr_set = hx711_attr_set,
#if defined(CONFIG_HX711_SPI_TRIGGER)
	.trigger_set = hx711_trigger_set,
#endif
};

static int hx711_init(const struct device *dev)
{
	const struct hx711_config *config = dev->config;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_DBG("SPI bus not ready");
		return -ENODEV;
	}

	/* Perform the first read to set the gain */

	if (hx711_sample_wait(dev) < 0) {
		return -ENODEV;
	}

	if (hx711_spi_read_sample(dev, NULL) < 0) {
		return -ENODEV;
	}

#if defined(CONFIG_HX711_SPI_TRIGGER)
	if (config->trig_cfg && hx711_gpio_irq_init(dev) < 0) {
		return -ENODEV;
	}
#endif

	return 0;
}

#define HX711_SPI_OPERATION                                                                        \
	(SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_TRANSFER_MSB)

#define HX711_TRGGER_ENABLED(node)                                                                 \
	COND_CODE_1(CONFIG_HX711_SPI_TRIGGER, (DT_INST_PINCTRL_HAS_NAME(node, trigger)), (false))

#define HX711_TRIGGER_CONFIG_NAME(node) hx711_##node##_trig_config

#define HX711_TRIGGER_DATA_NAME(node) hx711_##node##_trig_data

#define HX711_TRIGGER_DECL(node)                                                                   \
	PINCTRL_DT_INST_DEFINE(node);                                                              \
	static struct hx711_trigger_config HX711_TRIGGER_CONFIG_NAME(node) = {                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(node),                                      \
		.int_gpio = GPIO_DT_SPEC_INST_GET(node, dout_trig_gpios),                          \
	};                                                                                         \
	static struct hx711_trigger_data HX711_TRIGGER_DATA_NAME(node);

#define HX711_DEFINE(inst)                                                                         \
	IF_ENABLED(CONFIG_HX711_SPI_TRIGGER, (HX711_TRIGGER_DECL(inst)));                          \
	static struct hx711_data hx711_data_##inst = {                                             \
		.gain = DT_INST_PROP(inst, gain),                                                  \
		IF_ENABLED(HX711_TRGGER_ENABLED(inst), (                                           \
					.trig_data = &(HX711_TRIGGER_DATA_NAME(inst)),             \
					)) };                                                      \
	static const struct hx711_config hx711_config_##inst = {                                   \
		.spi = SPI_DT_SPEC_INST_GET(inst, HX711_SPI_OPERATION),                            \
		.avdd_uv = DT_INST_PROP(inst, avdd) * 1000,                                        \
		IF_ENABLED(HX711_TRGGER_ENABLED(inst), (                                           \
					.trig_cfg = &(HX711_TRIGGER_CONFIG_NAME(inst)),            \
					)) };                                                      \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, hx711_init, NULL,                                       \
				     &hx711_data_##inst, &hx711_config_##inst, POST_KERNEL,        \
				     CONFIG_SENSOR_INIT_PRIORITY, &hx711_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HX711_DEFINE)
