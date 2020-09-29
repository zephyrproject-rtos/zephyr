/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl362

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>

#include "adxl362.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(ADXL362, CONFIG_SENSOR_LOG_LEVEL);

static void adxl362_thread_cb(const struct device *dev)
{
	struct adxl362_data *drv_data = dev->data;
	uint8_t status_buf;

	/* Clears activity and inactivity interrupt */
	if (adxl362_get_status(dev, &status_buf)) {
		LOG_ERR("Unable to get status.");
		return;
	}

	k_mutex_lock(&drv_data->trigger_mutex, K_FOREVER);
	if (drv_data->th_handler != NULL) {
		if (ADXL362_STATUS_CHECK_INACT(status_buf) ||
		    ADXL362_STATUS_CHECK_ACTIVITY(status_buf)) {
			drv_data->th_handler(dev, &drv_data->th_trigger);
		}
	}

	if (drv_data->drdy_handler != NULL &&
	    ADXL362_STATUS_CHECK_DATA_READY(status_buf)) {
		drv_data->drdy_handler(dev, &drv_data->drdy_trigger);
	}
	k_mutex_unlock(&drv_data->trigger_mutex);
}

static void adxl362_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct adxl362_data *drv_data =
		CONTAINER_OF(cb, struct adxl362_data, gpio_cb);

#if defined(CONFIG_ADXL362_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADXL362_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

#if defined(CONFIG_ADXL362_TRIGGER_OWN_THREAD)
static void adxl362_thread(struct adxl362_data *drv_data)
{
	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		adxl362_thread_cb(drv_data->dev);
	}
}
#elif defined(CONFIG_ADXL362_TRIGGER_GLOBAL_THREAD)
static void adxl362_work_cb(struct k_work *work)
{
	struct adxl362_data *drv_data =
		CONTAINER_OF(work, struct adxl362_data, work);

	adxl362_thread_cb(drv_data->dev);
}
#endif

int adxl362_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct adxl362_data *drv_data = dev->data;
	uint8_t int_mask, int_en, status_buf;

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		k_mutex_lock(&drv_data->trigger_mutex, K_FOREVER);
		drv_data->th_handler = handler;
		drv_data->th_trigger = *trig;
		k_mutex_unlock(&drv_data->trigger_mutex);
		int_mask = ADXL362_INTMAP1_ACT |
			   ADXL362_INTMAP1_INACT;
		/* Clear activity and inactivity interrupts */
		adxl362_get_status(dev, &status_buf);
		break;
	case SENSOR_TRIG_DATA_READY:
		k_mutex_lock(&drv_data->trigger_mutex, K_FOREVER);
		drv_data->drdy_handler = handler;
		drv_data->drdy_trigger = *trig;
		k_mutex_unlock(&drv_data->trigger_mutex);
		int_mask = ADXL362_INTMAP1_DATA_READY;
		adxl362_clear_data_ready(dev);
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	if (handler) {
		int_en = int_mask;
	} else {
		int_en = 0U;
	}

	return adxl362_reg_write_mask(dev, ADXL362_REG_INTMAP1, int_mask, int_en);
}

int adxl362_init_interrupt(const struct device *dev)
{
	struct adxl362_data *drv_data = dev->data;
	const struct adxl362_config *cfg = dev->config;
	int ret;

	k_mutex_init(&drv_data->trigger_mutex);

	drv_data->gpio = device_get_binding(cfg->gpio_port);
	if (drv_data->gpio == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			cfg->gpio_port);
		return -EINVAL;
	}

	ret = adxl362_set_interrupt_mode(dev, CONFIG_ADXL362_INTERRUPT_MODE);

	if (ret) {
		return -EFAULT;
	}

	gpio_pin_configure(drv_data->gpio, cfg->int_gpio,
			   GPIO_INPUT | cfg->int_flags);

	gpio_init_callback(&drv_data->gpio_cb,
			   adxl362_gpio_callback,
			   BIT(cfg->int_gpio));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Failed to set gpio callback!");
		return -EIO;
	}

	drv_data->dev = dev;

#if defined(CONFIG_ADXL362_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ADXL362_THREAD_STACK_SIZE,
			(k_thread_entry_t)adxl362_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_ADXL362_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_ADXL362_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = adxl362_work_cb;
#endif

	gpio_pin_interrupt_configure(drv_data->gpio, cfg->int_gpio,
				     GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}
