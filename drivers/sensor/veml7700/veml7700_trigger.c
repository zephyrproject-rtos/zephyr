/*
 * Copyright (c) 2022 Innblue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vishay_veml7700

#include "veml7700.h"
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <kernel.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(veml7700, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_VEML7700_TRIGGER_OWN_THREAD
static K_KERNEL_STACK_DEFINE(veml7700_thread_stack, CONFIG_VEML7700_THREAD_STACK_SIZE);
static struct k_thread veml7700_thread;
#endif

int veml7700_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct veml7700_data *data = (struct veml7700_data *)dev->data;

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		return -ENOTSUP;
	}
	if (NULL == handler) {
		return -EINVAL;
	}

	data->handler = handler;
	data->trigger = *trig;

	return 0;
}

static void veml7700_gpio_thread_cb(const struct device *dev)
{
	struct veml7700_data *data = (struct veml7700_data *)dev->data;

	if (data->handler_als_thrs != NULL) {
		data->handler_als_thrs(dev, &data->trig_als_thrs)
	}
}

#ifdef CONFIG_VEML7700_TRIGGER_OWN_THREAD

static void veml7700_gpio_cb(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct veml7700_data *data = CONTAINER_OF(cb, struct veml7700_data, gpio_cb);

	ARG_UNUSED(pins);

	k_sem_give(&data->sem);
}

static void veml7700_thread_main(struct veml7700_data *data)
{
	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		veml7700_gpio_thread_cb(data->dev);
	}
}

#else /* CONFIG_VEML7700_TRIGGER_GLOBAL_THREAD */

static void veml7700_gpio_cb(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct veml7700_data *data = CONTAINER_OF(cb, struct veml7700_data, gpio_cb);

	ARG_UNUSED(pins);

	k_work_submit(&data->work);
}

static void veml7700_work_cb(struct k_work *work)
{
	struct veml7700_data *data = CONTAINER_OF(work, struct veml7700_data, work);

	veml7700_gpio_thread_cb(data->dev);
}
#endif /* CONFIG_VEML7700_TRIGGER_GLOBAL_THREAD */

int veml7700_interrupt_init(const struct device *dev)
{
	struct veml7700_data *data = (struct veml7700_data *)dev->data;
	const struct veml7700_config *cfg = (struct veml7700_config *)dev->config;

#ifdef CONFIG_VEML7700_TRIGGER_OWN_THREAD
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);
#else
	data->work.handler = veml7700_work_cb;
#endif

	data->int_gpio = device_get_binding(cfg->int_gpio);
	if (NULL == data->int_gpio) {
		LOG_ERR("VEML >> GPIO device %s >> Not found", cfg->i2c_name);
		return -EINVAL;
	}

	LOG_DBG("VEML >> GPIO >> Device ready");
	data->int_gpio_pin = cfg->int_gpio_pin;
	gpio_flags_t gpio_cfg_flags = (GPIO_INPUT | cfg->int_gpio_flags);
	err = gpio_pin_configure(data->int_gpio, data->int_gpio_pin, gpio_cfg_flags);
	if (err < 0) {
		LOG_ERR("VEML >> Could not configure gpio %u", data->int_gpio_pin);
		return err;
	}

	LOG_DBG("VEML >> GPIO >> Pins configured");
	gpio_init_callback(&data->int_gpio_cb, veml7700_gpio_cb, BIT(data->int_gpio_pin));

	if (gpio_add_callback(data->int_gpio, &data->int_gpio_cb) < 0) {
		LOG_ERR("VEML >> Could not set gpio callback");
		return -EIO;
	}

	err = gpio_pin_interrupt_configure(data->int_gpio, data->int_gpio_pin,
					   GPIO_INT_EDGE_TO_ACTIVE);
	if (err < 0) {
		LOG_ERR("VEML >> Could not configure interrupt for gpio %u (%d)",
			data->int_gpio_pin, err);
		return err;
	}

#ifdef CONFIG_VEML7700_TRIGGER_OWN_THREAD
	k_thread_create(&veml7700_thread, veml7700_thread_stack, CONFIG_VEML7700_THREAD_STACK_SIZE,
			(k_thread_entry_t)veml7700_thread_main, data, 0, NULL,
			K_PRIO_COOP(CONFIG_VEML7700_THREAD_PRIORITY), 0, K_NO_WAIT);
#endif

	return 0;
}
