/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_qdec

#include <stdint.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_gpio_qdec, CONFIG_INPUT_LOG_LEVEL);

#define GPIO_QDEC_GPIO_NUM 2

struct gpio_qdec_config {
	struct gpio_dt_spec ab_gpio[GPIO_QDEC_GPIO_NUM];
	const struct gpio_dt_spec *led_gpio;
	uint8_t led_gpio_count;
	uint32_t led_pre_us;
	uint32_t sample_time_us;
	uint32_t idle_poll_time_us;
	uint32_t idle_timeout_ms;
	uint16_t axis;
	uint8_t steps_per_period;
};

struct gpio_qdec_data {
	const struct device *dev;
	struct k_timer sample_timer;
	uint8_t prev_step;
	int32_t acc;
	struct k_work event_work;
	struct k_work_delayable idle_work;
	struct gpio_callback gpio_cb;
	atomic_t polling;
#ifdef CONFIG_PM_DEVICE
	atomic_t suspended;
#endif
};

/* Positive transitions */
#define QDEC_LL_LH 0x01
#define QDEC_LH_HH 0x13
#define QDEC_HH_HL 0x32
#define QDEC_HL_LL 0x20

/* Negative transitions */
#define QDEC_LL_HL 0x02
#define QDEC_LH_LL 0x10
#define QDEC_HH_LH 0x31
#define QDEC_HL_HH 0x23

static void gpio_qdec_irq_setup(const struct device *dev, bool enable)
{
	const struct gpio_qdec_config *cfg = dev->config;
	gpio_flags_t flags = enable ? GPIO_INT_EDGE_BOTH : GPIO_INT_DISABLE;
	int ret;

	for (int i = 0; i < GPIO_QDEC_GPIO_NUM; i++) {
		const struct gpio_dt_spec *gpio = &cfg->ab_gpio[i];

		ret = gpio_pin_interrupt_configure_dt(gpio, flags);
		if (ret != 0) {
			LOG_ERR("Pin %d interrupt configuration failed: %d", i, ret);
			return;
		}
	}
}

static bool gpio_qdec_idle_polling_mode(const struct device *dev)
{
	const struct gpio_qdec_config *cfg = dev->config;

	if (cfg->idle_poll_time_us > 0) {
		return true;
	}

	return false;
}

static void gpio_qdec_poll_mode(const struct device *dev)
{
	const struct gpio_qdec_config *cfg = dev->config;
	struct gpio_qdec_data *data = dev->data;

	if (!gpio_qdec_idle_polling_mode(dev)) {
		gpio_qdec_irq_setup(dev, false);
	}

	k_timer_start(&data->sample_timer, K_NO_WAIT,
		      K_USEC(cfg->sample_time_us));

	atomic_set(&data->polling, 1);

	LOG_DBG("polling start");
}

static void gpio_qdec_idle_mode(const struct device *dev)
{
	const struct gpio_qdec_config *cfg = dev->config;
	struct gpio_qdec_data *data = dev->data;

	if (gpio_qdec_idle_polling_mode(dev)) {
		k_timer_start(&data->sample_timer, K_NO_WAIT,
			      K_USEC(cfg->idle_poll_time_us));
	} else {
		k_timer_stop(&data->sample_timer);
		gpio_qdec_irq_setup(dev, true);
	}

	atomic_set(&data->polling, 0);

	LOG_DBG("polling stop");
}

static uint8_t gpio_qdec_get_step(const struct device *dev)
{
	const struct gpio_qdec_config *cfg = dev->config;
	uint8_t step = 0x00;

	if (gpio_qdec_idle_polling_mode(dev)) {
		for (int i = 0; i < cfg->led_gpio_count; i++) {
			gpio_pin_set_dt(&cfg->led_gpio[i], 1);
		}

		k_busy_wait(cfg->led_pre_us);
	}

	if (gpio_pin_get_dt(&cfg->ab_gpio[0])) {
		step |= 0x01;
	}
	if (gpio_pin_get_dt(&cfg->ab_gpio[1])) {
		step |= 0x02;
	}

	if (gpio_qdec_idle_polling_mode(dev)) {
		for (int i = 0; i < cfg->led_gpio_count; i++) {
			gpio_pin_set_dt(&cfg->led_gpio[i], 0);
		}
	}

	return step;
}

static void gpio_qdec_sample_timer_timeout(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);
	const struct gpio_qdec_config *cfg = dev->config;
	struct gpio_qdec_data *data = dev->data;
	int8_t delta = 0;
	unsigned int key;
	uint8_t step;

#ifdef CONFIG_PM_DEVICE
	if (atomic_get(&data->suspended) == 1) {
		return;
	}
#endif

	step = gpio_qdec_get_step(dev);

	if (data->prev_step == step) {
		return;
	}

	if (gpio_qdec_idle_polling_mode(dev) &&
	    atomic_get(&data->polling) == 0) {
		gpio_qdec_poll_mode(dev);
	}

	switch ((data->prev_step << 4U) | step) {
	case QDEC_LL_LH:
	case QDEC_LH_HH:
	case QDEC_HH_HL:
	case QDEC_HL_LL:
		delta = 1;
		break;
	case QDEC_LL_HL:
	case QDEC_LH_LL:
	case QDEC_HH_LH:
	case QDEC_HL_HH:
		delta = -1;
		break;
	default:
		LOG_WRN("%s: lost steps", dev->name);
	}

	data->prev_step = step;

	key = irq_lock();
	data->acc += delta;
	irq_unlock(key);

	if (abs(data->acc) >= cfg->steps_per_period) {
		k_work_submit(&data->event_work);
	}

	k_work_reschedule(&data->idle_work, K_MSEC(cfg->idle_timeout_ms));
}

static void gpio_qdec_event_worker(struct k_work *work)
{
	struct gpio_qdec_data *data = CONTAINER_OF(
			work, struct gpio_qdec_data, event_work);
	const struct device *dev = data->dev;
	const struct gpio_qdec_config *cfg = dev->config;
	unsigned int key;
	int32_t acc;

	key = irq_lock();
	acc = data->acc / cfg->steps_per_period;
	data->acc -= acc * cfg->steps_per_period;
	irq_unlock(key);

	if (acc != 0) {
		input_report_rel(data->dev, cfg->axis, acc, true, K_FOREVER);
	}
}

static void gpio_qdec_idle_worker(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct gpio_qdec_data *data = CONTAINER_OF(
			dwork, struct gpio_qdec_data, idle_work);
	const struct device *dev = data->dev;

	gpio_qdec_idle_mode(dev);
}

static void gpio_qdec_cb(const struct device *gpio_dev, struct gpio_callback *cb,
			 uint32_t pins)
{
	struct gpio_qdec_data *data = CONTAINER_OF(
			cb, struct gpio_qdec_data, gpio_cb);
	const struct device *dev = data->dev;

	gpio_qdec_poll_mode(dev);
}

static int gpio_qdec_init(const struct device *dev)
{
	const struct gpio_qdec_config *cfg = dev->config;
	struct gpio_qdec_data *data = dev->data;
	int ret;

	data->dev = dev;

	k_work_init(&data->event_work, gpio_qdec_event_worker);
	k_work_init_delayable(&data->idle_work, gpio_qdec_idle_worker);

	k_timer_init(&data->sample_timer, gpio_qdec_sample_timer_timeout, NULL);
	k_timer_user_data_set(&data->sample_timer, (void *)dev);

	gpio_init_callback(&data->gpio_cb, gpio_qdec_cb,
			   BIT(cfg->ab_gpio[0].pin) | BIT(cfg->ab_gpio[1].pin));
	for (int i = 0; i < GPIO_QDEC_GPIO_NUM; i++) {
		const struct gpio_dt_spec *gpio = &cfg->ab_gpio[i];

		if (!gpio_is_ready_dt(gpio)) {
			LOG_ERR("%s is not ready", gpio->port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
		if (ret != 0) {
			LOG_ERR("Pin %d configuration failed: %d", i, ret);
			return ret;
		}

		if (gpio_qdec_idle_polling_mode(dev)) {
			continue;
		}

		ret = gpio_add_callback_dt(gpio, &data->gpio_cb);
		if (ret < 0) {
			LOG_ERR("Could not set gpio callback");
			return ret;
		}
	}

	for (int i = 0; i < cfg->led_gpio_count; i++) {
		const struct gpio_dt_spec *gpio = &cfg->led_gpio[i];
		gpio_flags_t mode;

		if (!gpio_is_ready_dt(gpio)) {
			LOG_ERR("%s is not ready", gpio->port->name);
			return -ENODEV;
		}

		mode = gpio_qdec_idle_polling_mode(dev) ?
			GPIO_OUTPUT_INACTIVE : GPIO_OUTPUT_ACTIVE;

		ret = gpio_pin_configure_dt(gpio, mode);
		if (ret != 0) {
			LOG_ERR("Pin %d configuration failed: %d", i, ret);
			return ret;
		}
	}

	data->prev_step = gpio_qdec_get_step(dev);

	gpio_qdec_idle_mode(dev);

	ret = pm_device_runtime_enable(dev);
	if (ret < 0) {
		LOG_ERR("Failed to enable runtime power management");
		return ret;
	}

	LOG_DBG("Device %s initialized", dev->name);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static void gpio_qdec_pin_suspend(const struct device *dev, bool suspend)
{
	const struct gpio_qdec_config *cfg = dev->config;
	gpio_flags_t mode = suspend ? GPIO_DISCONNECTED : GPIO_INPUT;
	int ret;

	for (int i = 0; i < GPIO_QDEC_GPIO_NUM; i++) {
		const struct gpio_dt_spec *gpio = &cfg->ab_gpio[i];

		ret = gpio_pin_configure_dt(gpio, mode);
		if (ret != 0) {
			LOG_ERR("Pin %d configuration failed: %d", i, ret);
			return;
		}
	}

	for (int i = 0; i < cfg->led_gpio_count; i++) {
		if (suspend) {
			gpio_pin_set_dt(&cfg->led_gpio[i], 0);
		} else if (!gpio_qdec_idle_polling_mode(dev)) {
			gpio_pin_set_dt(&cfg->led_gpio[i], 1);
		}
	}
}

static int gpio_qdec_pm_action(const struct device *dev,
			       enum pm_device_action action)
{
	struct gpio_qdec_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		struct k_work_sync sync;

		atomic_set(&data->suspended, 1);

		k_work_cancel_delayable_sync(&data->idle_work, &sync);

		if (!gpio_qdec_idle_polling_mode(dev)) {
			gpio_qdec_irq_setup(dev, false);
		}

		k_timer_stop(&data->sample_timer);

		gpio_qdec_pin_suspend(dev, true);

		break;
	case PM_DEVICE_ACTION_RESUME:
		atomic_set(&data->suspended, 0);

		gpio_qdec_pin_suspend(dev, false);

		data->prev_step = gpio_qdec_get_step(dev);
		data->acc = 0;

		gpio_qdec_idle_mode(dev);

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#define QDEC_GPIO_INIT(n)							\
	BUILD_ASSERT(DT_INST_PROP_LEN(n, gpios) == GPIO_QDEC_GPIO_NUM,		\
		     "input_gpio_qdec: gpios must have exactly two entries");	\
										\
	BUILD_ASSERT(!(DT_INST_NODE_HAS_PROP(n, led_gpios) &&			\
		       DT_INST_NODE_HAS_PROP(n, idle_poll_time_us)) ||		\
		     DT_INST_NODE_HAS_PROP(n, led_pre_us),			\
		     "led-pre-us must be specified when setting led-gpios and "	\
		     "idle-poll-time-us");					\
										\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(n, led_gpios), (			\
	static const struct gpio_dt_spec gpio_qdec_led_gpio_##n[] = {		\
		DT_INST_FOREACH_PROP_ELEM_SEP(n, led_gpios,			\
					      GPIO_DT_SPEC_GET_BY_IDX, (,))	\
	};									\
	))									\
										\
	static const struct gpio_qdec_config gpio_qdec_cfg_##n = {		\
		.ab_gpio = {							\
			GPIO_DT_SPEC_INST_GET_BY_IDX(n, gpios, 0),		\
			GPIO_DT_SPEC_INST_GET_BY_IDX(n, gpios, 1),		\
		},								\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, led_gpios), (		\
		.led_gpio = gpio_qdec_led_gpio_##n,				\
		.led_gpio_count = ARRAY_SIZE(gpio_qdec_led_gpio_##n),		\
		.led_pre_us = DT_INST_PROP_OR(n, led_pre_us, 0),		\
		))								\
		.sample_time_us = DT_INST_PROP(n, sample_time_us),		\
		.idle_poll_time_us = DT_INST_PROP_OR(n, idle_poll_time_us, 0),	\
		.idle_timeout_ms = DT_INST_PROP(n, idle_timeout_ms),		\
		.steps_per_period = DT_INST_PROP(n, steps_per_period),		\
		.axis = DT_INST_PROP(n, zephyr_axis),				\
	};									\
										\
	static struct gpio_qdec_data gpio_qdec_data_##n;			\
										\
	PM_DEVICE_DT_INST_DEFINE(n, gpio_qdec_pm_action);			\
										\
	DEVICE_DT_INST_DEFINE(n, gpio_qdec_init, PM_DEVICE_DT_INST_GET(n),	\
			      &gpio_qdec_data_##n,				\
			      &gpio_qdec_cfg_##n,				\
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,		\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(QDEC_GPIO_INIT)
