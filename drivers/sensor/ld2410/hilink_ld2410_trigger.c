/*
 * Copyright (c) 2023 Fabian Blatz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hilink_ld2410

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/sensor/ld2410.h>
#include "hilink_ld2410.h"

LOG_MODULE_DECLARE(LD2410, CONFIG_SENSOR_LOG_LEVEL);

static void setup_int(const struct device *dev, bool enable)
{
	const struct ld2410_config *drv_cfg = dev->config;

	gpio_flags_t flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&drv_cfg->int_gpios, flags);
}

static void process_int(const struct device *dev)
{
	struct ld2410_data *drv_data = dev->data;

	if (drv_data->th_handler != NULL) {
		drv_data->th_handler(dev, drv_data->th_trigger);
	}
	setup_int(dev, true);
}

int ld2410_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct ld2410_data *drv_data = dev->data;

	if ((enum sensor_trigger_type_ld2410)trig->type == SENSOR_TRIG_HUMAN_PRESENCE) {
		drv_data->th_handler = handler;
		drv_data->th_trigger = trig;
		setup_int(dev, true);
	} else {
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	return 0;
}

static void ld2410_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct ld2410_data *drv_data = CONTAINER_OF(cb, struct ld2410_data, gpio_cb);

	setup_int(drv_data->gpio_dev, false);

#if defined(CONFIG_LD2410_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_LD2410_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

#if defined(CONFIG_LD2410_TRIGGER_OWN_THREAD)
static void ld2410_thread(struct ld2410_data *drv_data)
{
	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		process_int(drv_data->gpio_dev);
	}
}

#elif defined(CONFIG_LD2410_TRIGGER_GLOBAL_THREAD)
static void ld2410_work_cb(struct k_work *work)
{
	struct ld2410_data *drv_data = CONTAINER_OF(work, struct ld2410_data, work);

	process_int(drv_data->gpio_dev);
}
#endif

int ld2410_init_interrupt(const struct device *dev)
{
	struct ld2410_data *drv_data = dev->data;
	const struct ld2410_config *drv_cfg = dev->config;
	int rc;

	if (!device_is_ready(drv_cfg->int_gpios.port)) {
		LOG_ERR("GPIO port %s not ready", drv_cfg->int_gpios.port->name);
		return -ENODEV;
	}

	rc = gpio_pin_configure_dt(&drv_cfg->int_gpios, GPIO_INPUT);
	if (rc < 0) {
		return rc;
	}

	drv_data->gpio_dev = dev;
#if defined(CONFIG_LD2410_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack, CONFIG_LD2410_THREAD_STACK_SIZE,
			(k_thread_entry_t)ld2410_thread, drv_data, NULL, NULL,
			K_PRIO_COOP(CONFIG_LD2410_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_LD2410_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = ld2410_work_cb;
#endif

	gpio_init_callback(&drv_data->gpio_cb, ld2410_gpio_callback, BIT(drv_cfg->int_gpios.pin));

	rc = gpio_add_callback(drv_cfg->int_gpios.port, &drv_data->gpio_cb);
	if (rc < 0) {
		LOG_ERR("Could not set gpio callback.");
		return rc;
	}

	return 0;
}
