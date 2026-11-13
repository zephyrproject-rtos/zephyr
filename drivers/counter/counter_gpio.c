/*
 * SPDX-FileCopyrightText: Copyright RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_counter_gpio

#include <errno.h>

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(counter_gpio, CONFIG_COUNTER_LOG_LEVEL);

struct counter_gpio_chan_data {
	struct gpio_callback cb;
	const struct device *dev;
	counter_capture_cb_t user_cb;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	counter_capture_cb_64_t user_cb_64;
#endif
	void *user_data;
	counter_capture_flags_t flags;
	uint8_t chan_id;
};

struct counter_gpio_data {
	struct counter_gpio_chan_data *chan;
};

struct counter_gpio_config {
	struct counter_config_info info;
	const struct gpio_dt_spec *gpios;
};

/* The kernel clock is already running and cannot be stopped, so starting is
 * nothing to do and stopping is nothing this device can offer. Captures are
 * turned on a channel at a time, through enable_capture.
 */
static int counter_gpio_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int counter_gpio_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static int counter_gpio_get_value(const struct device *dev, uint32_t *ticks)
{
	ARG_UNUSED(dev);

	*ticks = (uint32_t)k_uptime_ticks();

	return 0;
}

static uint32_t counter_gpio_get_top_value(const struct device *dev)
{
	ARG_UNUSED(dev);

	return UINT32_MAX;
}

#ifdef CONFIG_COUNTER_CAPTURE
static gpio_flags_t counter_gpio_int_flags(counter_capture_flags_t flags)
{
	switch (flags & COUNTER_CAPTURE_BOTH_EDGES) {
	case COUNTER_CAPTURE_RISING_EDGE:
		return GPIO_INT_EDGE_RISING;
	case COUNTER_CAPTURE_FALLING_EDGE:
		return GPIO_INT_EDGE_FALLING;
	case COUNTER_CAPTURE_BOTH_EDGES:
		return GPIO_INT_EDGE_BOTH;
	default:
		return 0;
	}
}

static int counter_gpio_capture_arm(const struct device *dev, uint8_t chan_id,
				    counter_capture_flags_t flags, void *user_data)
{
	const struct counter_gpio_config *config = dev->config;
	struct counter_gpio_data *data = dev->data;
	struct counter_gpio_chan_data *chan = &data->chan[chan_id];
	int ret;

	if (counter_gpio_int_flags(flags) == 0U) {
		return -EINVAL;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->gpios[chan_id], GPIO_INT_DISABLE);
	if (ret < 0) {
		return ret;
	}

	chan->user_cb = NULL;
#ifdef CONFIG_COUNTER_64BITS_TICKS
	chan->user_cb_64 = NULL;
#endif
	chan->user_data = user_data;
	chan->flags = flags;

	return 0;
}

static void counter_gpio_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct counter_gpio_chan_data *chan = CONTAINER_OF(cb, struct counter_gpio_chan_data, cb);
	const struct counter_gpio_config *config = chan->dev->config;
	uint64_t ticks = (uint64_t)k_uptime_ticks();

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	if ((chan->flags & COUNTER_CAPTURE_SINGLE_SHOT) != 0U) {
		(void)gpio_pin_interrupt_configure_dt(&config->gpios[chan->chan_id],
						      GPIO_INT_DISABLE);
	}

#ifdef CONFIG_COUNTER_64BITS_TICKS
	if (chan->user_cb_64 != NULL) {
		chan->user_cb_64(chan->dev, chan->chan_id, chan->flags, ticks, chan->user_data);
	}
#endif

	if (chan->user_cb != NULL) {
		chan->user_cb(chan->dev, chan->chan_id, chan->flags, (uint32_t)ticks,
			      chan->user_data);
	}
}

static int counter_gpio_capture_configure(const struct device *dev, uint8_t chan_id,
					  counter_capture_flags_t flags, counter_capture_cb_t cb,
					  void *user_data)
{
	struct counter_gpio_data *data = dev->data;
	int ret;

	ret = counter_gpio_capture_arm(dev, chan_id, flags, user_data);
	if (ret < 0) {
		return ret;
	}

	data->chan[chan_id].user_cb = cb;

	return 0;
}

#ifdef CONFIG_COUNTER_64BITS_TICKS
static int counter_gpio_capture_configure_64(const struct device *dev, uint8_t chan_id,
					     counter_capture_flags_t flags,
					     counter_capture_cb_64_t cb, void *user_data)
{
	struct counter_gpio_data *data = dev->data;
	int ret;

	ret = counter_gpio_capture_arm(dev, chan_id, flags, user_data);
	if (ret < 0) {
		return ret;
	}

	data->chan[chan_id].user_cb_64 = cb;

	return 0;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

static int counter_gpio_enable_capture(const struct device *dev, uint8_t chan_id)
{
	const struct counter_gpio_config *config = dev->config;
	struct counter_gpio_data *data = dev->data;
	struct counter_gpio_chan_data *chan = &data->chan[chan_id];
	bool armed = chan->user_cb != NULL;

#ifdef CONFIG_COUNTER_64BITS_TICKS
	armed = armed || (chan->user_cb_64 != NULL);
#endif

	if (!armed) {
		return -EINVAL;
	}

	return gpio_pin_interrupt_configure_dt(&config->gpios[chan_id],
					       counter_gpio_int_flags(chan->flags));
}

static int counter_gpio_disable_capture(const struct device *dev, uint8_t chan_id)
{
	const struct counter_gpio_config *config = dev->config;

	return gpio_pin_interrupt_configure_dt(&config->gpios[chan_id], GPIO_INT_DISABLE);
}
#endif /* CONFIG_COUNTER_CAPTURE */

static int counter_gpio_init(const struct device *dev)
{
	const struct counter_gpio_config *config = dev->config;
	struct counter_gpio_data *data = dev->data;

	for (uint8_t i = 0; i < config->info.channels; i++) {
		const struct gpio_dt_spec *gpio = &config->gpios[i];
		struct counter_gpio_chan_data *chan = &data->chan[i];
		int ret;

		if (!gpio_is_ready_dt(gpio)) {
			LOG_ERR_DEVICE_NOT_READY(gpio->port);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure channel %u: %d", i, ret);
			return ret;
		}

		chan->dev = dev;
		chan->chan_id = i;

		gpio_init_callback(&chan->cb, counter_gpio_isr, BIT(gpio->pin));

		ret = gpio_add_callback_dt(gpio, &chan->cb);
		if (ret < 0) {
			LOG_ERR("Could not add callback for channel %u: %d", i, ret);
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(counter, counter_gpio_api) = {
	.start = counter_gpio_start,
	.stop = counter_gpio_stop,
	.get_value = counter_gpio_get_value,
	.get_top_value = counter_gpio_get_top_value,
#ifdef CONFIG_COUNTER_CAPTURE
	.capture_configure = counter_gpio_capture_configure,
#ifdef CONFIG_COUNTER_64BITS_TICKS
	.capture_configure_64 = counter_gpio_capture_configure_64,
#endif
	.enable_capture = counter_gpio_enable_capture,
	.disable_capture = counter_gpio_disable_capture,
#endif
};

#define COUNTER_GPIO_SPEC(node_id, prop, idx) GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx),

#define COUNTER_GPIO_INIT(inst)                                                                    \
	static const struct gpio_dt_spec counter_gpio_pins_##inst[] = {                            \
		DT_INST_FOREACH_PROP_ELEM(inst, gpios, COUNTER_GPIO_SPEC)};                        \
                                                                                                   \
	static struct counter_gpio_chan_data                                                       \
		counter_gpio_chans_##inst[ARRAY_SIZE(counter_gpio_pins_##inst)];                   \
                                                                                                   \
	static struct counter_gpio_data counter_gpio_data_##inst = {                               \
		.chan = counter_gpio_chans_##inst,                                                 \
	};                                                                                         \
                                                                                                   \
	static const struct counter_gpio_config counter_gpio_config_##inst = {                     \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.freq = CONFIG_SYS_CLOCK_TICKS_PER_SEC,                            \
				.channels = ARRAY_SIZE(counter_gpio_pins_##inst),                  \
			},                                                                         \
		.gpios = counter_gpio_pins_##inst,                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, counter_gpio_init, NULL, &counter_gpio_data_##inst,            \
			      &counter_gpio_config_##inst, POST_KERNEL,                            \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_GPIO_INIT)
