/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include "adt7420.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ADT7420, CONFIG_SENSOR_LOG_LEVEL);

static void setup_int(const struct device *dev,
		      bool enable)
{
	const struct adt7420_dev_config *cfg = dev->config;
	gpio_flags_t flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, flags);
}

static void handle_int(const struct device *dev)
{
	struct adt7420_data *drv_data = dev->data;

	setup_int(dev, false);

#if defined(CONFIG_ADT7420_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADT7420_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void process_int(const struct device *dev)
{
	struct adt7420_data *drv_data = dev->data;
	const struct adt7420_dev_config *cfg = dev->config;
	uint8_t status;

	/* Clear the status */
	if (i2c_reg_read_byte_dt(&cfg->i2c,
				 ADT7420_REG_STATUS, &status) < 0) {
		return;
	}

	if (drv_data->th_handler != NULL) {
		drv_data->th_handler(dev, &drv_data->th_trigger);
	}

	setup_int(dev, true);

	/* Check for pin that asserted while we were offline */
	int pv = gpio_pin_get_dt(&cfg->int_gpio);

	if (pv > 0) {
		handle_int(dev);
	}
}

static void adt7420_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct adt7420_data *drv_data =
		CONTAINER_OF(cb, struct adt7420_data, gpio_cb);

	handle_int(drv_data->dev);
}

#if defined(CONFIG_ADT7420_TRIGGER_OWN_THREAD)
static void adt7420_thread(struct adt7420_data *drv_data)
{
	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		process_int(drv_data->dev);
	}
}

#elif defined(CONFIG_ADT7420_TRIGGER_GLOBAL_THREAD)
static void adt7420_work_cb(struct k_work *work)
{
	struct adt7420_data *drv_data =
		CONTAINER_OF(work, struct adt7420_data, work);

	process_int(drv_data->dev);
}
#endif

int adt7420_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct adt7420_data *drv_data = dev->data;
	const struct adt7420_dev_config *cfg = dev->config;

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

	setup_int(dev, false);

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}
	drv_data->th_handler = handler;

	if (handler != NULL) {
		drv_data->th_trigger = *trig;

		setup_int(dev, true);

		/* Check whether already asserted */
		int pv = gpio_pin_get_dt(&cfg->int_gpio);

		if (pv > 0) {
			handle_int(dev);
		}
	}

	return 0;
}

int adt7420_init_interrupt(const struct device *dev)
{
	struct adt7420_data *drv_data = dev->data;
	const struct adt7420_dev_config *cfg = dev->config;
	int rc;

	if (!device_is_ready(cfg->int_gpio.port)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
			cfg->int_gpio.port->name);
		return -ENODEV;
	}

	gpio_init_callback(&drv_data->gpio_cb,
			   adt7420_gpio_callback,
			   BIT(cfg->int_gpio.pin));

	rc = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT | cfg->int_gpio.dt_flags);
	if (rc < 0) {
		return rc;
	}

	rc = gpio_add_callback(cfg->int_gpio.port, &drv_data->gpio_cb);
	if (rc < 0) {
		return rc;
	}

	drv_data->dev = dev;

#if defined(CONFIG_ADT7420_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ADT7420_THREAD_STACK_SIZE,
			(k_thread_entry_t)adt7420_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_ADT7420_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_ADT7420_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = adt7420_work_cb;
#endif

	return 0;
}
