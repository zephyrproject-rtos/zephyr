/*
 * Copyright (c) 2024 Ilia Kharin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PINNACLE_PINNACLE_TRIGGER_H_
#define ZEPHYR_DRIVERS_SENSOR_PINNACLE_PINNACLE_TRIGGER_H_

#define DT_DRV_COMPAT cirque_pinnacle

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "pinnacle.h"

LOG_MODULE_DECLARE(PINNACLE, CONFIG_SENSOR_LOG_LEVEL);

int pinnacle_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct pinnacle_data *drv_data = dev->data;

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	drv_data->th_handler = handler;
	drv_data->th_trigger = trig;

	return 0;
}

static void pinnacle_data_ready_gpio_callback(const struct device *dev,
					      struct gpio_callback *cb,
					      uint32_t pins)
{
	struct pinnacle_data *drv_data =
		CONTAINER_OF(cb, struct pinnacle_data, dr_cb_data);

#if defined(CONFIG_PINNACLE_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->dr_sem);
#elif defined(CONFIG_PINNACLE_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static int pinnacle_handle_interrupt(const struct device *dev)
{
	struct pinnacle_data *drv_data = dev->data;

	if (drv_data->th_handler != NULL) {
		drv_data->th_handler(dev, drv_data->th_trigger);
	}

	return 0;
}

#if defined(CONFIG_PINNACLE_TRIGGER_OWN_THREAD)
static void pinnacle_thread(void *p1, void *p2, void *p3)
{

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct pinnacle_data *drv_data = p1;

	while (1) {
		k_sem_take(&drv_data->dr_sem, K_FOREVER);
		pinnacle_handle_interrupt(drv_data->dev);
	}
}
#elif defined(CONFIG_PINNACLE_TRIGGER_GLOBAL_THREAD)
static void pinnacle_work_cb(struct k_work *work)
{
	struct pinnacle_data *drv_data =
		CONTAINER_OF(work, struct pinnacle_data, work);

	pinnacle_handle_interrupt(drv_data->dev);
}
#endif

int pinnacle_init_interrupt(const struct device *dev)
{
	struct pinnacle_data *drv_data = dev->data;
	const struct pinnacle_config *config = dev->config;
	const struct gpio_dt_spec *gpio = &config->dr_gpio;

	drv_data->dev = dev;

#if defined(CONFIG_PINNACLE_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->dr_sem, 0, 1);

	k_thread_create(&drv_data->dr_thread, drv_data->dr_thread_stack,
			CONFIG_PINNACLE_THREAD_STACK_SIZE,
			pinnacle_thread, drv_data, NULL, NULL,
			K_PRIO_COOP(CONFIG_PINNACLE_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_PINNACLE_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = pinnacle_work_cb;
#endif

	/* Configure GPIO pin for HW_DR signal */
	int ret = gpio_is_ready_dt(gpio);

	if (!ret) {
		LOG_ERR("GPIO device %s/%d is not ready",
			gpio->port->name, gpio->pin);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Failed to configure %s/%d as input",
			gpio->port->name, gpio->pin);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("Failed to configured interrupt for %s/%d",
			gpio->port->name, gpio->pin);
		return ret;
	}

	gpio_init_callback(&drv_data->dr_cb_data,
			   pinnacle_data_ready_gpio_callback,
			   BIT(gpio->pin));

	ret = gpio_add_callback(gpio->port, &drv_data->dr_cb_data);
	if (ret) {
		LOG_ERR("Failed to configured interrupt for %s/%d",
			gpio->port->name, gpio->pin);
		return ret;
	}

	return 0;
}

#endif /* ZEPHYR_DRIVERS_SENSOR_PINNACLE_PINNACLE_TRIGGER_H_ */
