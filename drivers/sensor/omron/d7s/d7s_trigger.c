/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "d7s.h"

LOG_MODULE_DECLARE(D7S, CONFIG_SENSOR_LOG_LEVEL);

static void d7s_submit(struct d7s_data *data, atomic_val_t flag)
{
	atomic_or(&data->pending, flag);

#if defined(CONFIG_D7S_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#elif defined(CONFIG_D7S_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void d7s_int1_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	d7s_submit(CONTAINER_OF(cb, struct d7s_data, int1_cb), D7S_PENDING_INT1);
}

static void d7s_int2_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	d7s_submit(CONTAINER_OF(cb, struct d7s_data, int2_cb), D7S_PENDING_INT2);
}

static void d7s_process(const struct device *dev)
{
	struct d7s_data *data = dev->data;
	atomic_val_t pending = atomic_clear(&data->pending);

	if ((pending & D7S_PENDING_INT1) != 0 && data->threshold_handler != NULL) {
		data->threshold_handler(dev, data->threshold_trigger);
	}

	if ((pending & D7S_PENDING_INT2) != 0 && data->drdy_handler != NULL) {
		data->drdy_handler(dev, data->drdy_trigger);
	}
}

#if defined(CONFIG_D7S_TRIGGER_OWN_THREAD)
static void d7s_thread(void *p1, void *p2, void *p3)
{
	struct d7s_data *data = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);
		d7s_process(data->dev);
	}
}
#elif defined(CONFIG_D7S_TRIGGER_GLOBAL_THREAD)
static void d7s_work_handler(struct k_work *work)
{
	struct d7s_data *data = CONTAINER_OF(work, struct d7s_data, work);

	d7s_process(data->dev);
}
#endif

int d7s_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		    sensor_trigger_handler_t handler)
{
	const struct d7s_config *cfg = dev->config;
	struct d7s_data *data = dev->data;
	const struct gpio_dt_spec *gpio;
	gpio_flags_t mode;

	if (trig->chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		gpio = &cfg->int1;
		mode = GPIO_INT_EDGE_TO_ACTIVE;
		break;
	case SENSOR_TRIG_DATA_READY:
		/*
		 * INT2 is asserted while the sensor calculates, so the result
		 * becomes available on the trailing edge.
		 */
		gpio = &cfg->int2;
		mode = GPIO_INT_EDGE_TO_INACTIVE;
		break;
	default:
		return -ENOTSUP;
	}

	if (gpio->port == NULL) {
		return -ENOTSUP;
	}

	if (handler == NULL) {
		mode = GPIO_INT_DISABLE;
	}

	if (trig->type == SENSOR_TRIG_THRESHOLD) {
		data->threshold_trigger = trig;
		data->threshold_handler = handler;
		atomic_and(&data->pending, ~D7S_PENDING_INT1);
	} else {
		data->drdy_trigger = trig;
		data->drdy_handler = handler;
		atomic_and(&data->pending, ~D7S_PENDING_INT2);
	}

	return gpio_pin_interrupt_configure_dt(gpio, mode);
}

static int d7s_init_int(const struct gpio_dt_spec *gpio, struct gpio_callback *cb,
			gpio_callback_handler_t handler)
{
	int ret;

	if (gpio->port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(gpio)) {
		LOG_ERR_DEVICE_NOT_READY(gpio->port);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(cb, handler, BIT(gpio->pin));

	return gpio_add_callback_dt(gpio, cb);
}

int d7s_trigger_init(const struct device *dev)
{
	const struct d7s_config *cfg = dev->config;
	struct d7s_data *data = dev->data;
	int ret;

	data->dev = dev;

#if defined(CONFIG_D7S_TRIGGER_OWN_THREAD)
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_D7S_THREAD_STACK_SIZE, d7s_thread,
			data, NULL, NULL, K_PRIO_COOP(CONFIG_D7S_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, dev->name);
#elif defined(CONFIG_D7S_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->work, d7s_work_handler);
#endif

	ret = d7s_init_int(&cfg->int1, &data->int1_cb, d7s_int1_handler);
	if (ret < 0) {
		return ret;
	}

	return d7s_init_int(&cfg->int2, &data->int2_cb, d7s_int2_handler);
}
