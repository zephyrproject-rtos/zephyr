/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ldc1x1x

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "ldc1x1x.h"

LOG_MODULE_DECLARE(ldc1x1x, CONFIG_SENSOR_LOG_LEVEL);

static void ldc1x1x_handle_drdy(const struct device *dev)
{
	struct ldc1x1x_data *data = dev->data;
	sensor_trigger_handler_t handler;
	struct sensor_trigger trigger;

	k_mutex_lock(&data->trigger_mutex, K_FOREVER);
	handler = data->drdy_handler;
	trigger = data->drdy_trigger;
	k_mutex_unlock(&data->trigger_mutex);

	if (handler != NULL) {
		handler(dev, &trigger);
	}
}

static void ldc1x1x_gpio_callback(const struct device *port, struct gpio_callback *cb,
				  gpio_port_pins_t pins)
{
	struct ldc1x1x_data *data = CONTAINER_OF(cb, struct ldc1x1x_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

#ifdef CONFIG_LDC1X1X_TRIGGER_OWN_THREAD
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_LDC1X1X_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#ifdef CONFIG_LDC1X1X_TRIGGER_OWN_THREAD
static void ldc1x1x_thread(void *arg1, void *arg2, void *arg3)
{
	struct ldc1x1x_data *data = arg1;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		ldc1x1x_handle_drdy(data->dev);
	}
}
#elif defined(CONFIG_LDC1X1X_TRIGGER_GLOBAL_THREAD)
static void ldc1x1x_work_handler(struct k_work *work)
{
	struct ldc1x1x_data *data = CONTAINER_OF(work, struct ldc1x1x_data, work);

	ldc1x1x_handle_drdy(data->dev);
}
#endif

int ldc1x1x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct ldc1x1x_data *data = dev->data;
	const struct ldc1x1x_config *cfg = dev->config;
	uint16_t int_en;
	int ret;

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->intb_gpio, GPIO_INT_DISABLE);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&data->trigger_mutex, K_FOREVER);
	data->drdy_handler = handler;
	data->drdy_trigger = *trig;
	k_mutex_unlock(&data->trigger_mutex);

	if (handler != NULL) {
		int_en = LDC1X1X_ERROR_CONFIG_DRDY_2INT_SET(1);
		data->int_config = int_en;
	} else {
		int_en = 0;
		data->int_config = 0;
	}

	ret = ldc1x1x_reg_write_mask(dev, LDC1X1X_ERROR_CONFIG, LDC1X1X_ERROR_CONFIG_DRDY_2INT_MASK,
				     int_en);
	if (ret == 0) {
		uint16_t status;

		ret = ldc1x1x_get_status(dev, &status);
	}

	if (ret == 0) {
		ret = gpio_pin_interrupt_configure_dt(&cfg->intb_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	} else {
		(void)gpio_pin_interrupt_configure_dt(&cfg->intb_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	}

	return ret;
}

int ldc1x1x_init_interrupt(const struct device *dev)
{
	struct ldc1x1x_data *data = dev->data;
	const struct ldc1x1x_config *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->intb_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->intb_gpio.port);
		return -ENODEV;
	}

	k_mutex_init(&data->trigger_mutex);
	data->dev = dev;

	ret = ldc1x1x_set_interrupt_pin(dev, true);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->intb_gpio, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, ldc1x1x_gpio_callback, BIT(cfg->intb_gpio.pin));
	ret = gpio_add_callback(cfg->intb_gpio.port, &data->gpio_cb);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_LDC1X1X_TRIGGER_OWN_THREAD
	k_sem_init(&data->gpio_sem, 0, UINT_MAX);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_LDC1X1X_THREAD_STACK_SIZE,
			ldc1x1x_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_LDC1X1X_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_LDC1X1X_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->work, ldc1x1x_work_handler);
#endif

	return gpio_pin_interrupt_configure_dt(&cfg->intb_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}
