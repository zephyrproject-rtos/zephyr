/*
 * Copyright (c) 2024, Han Wu
 * email: wuhanstudios@gmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "icm20608.h"

LOG_MODULE_DECLARE(ICM20608, CONFIG_SENSOR_LOG_LEVEL);

#define ICM20608_DRDY_EN BIT(0)

int icm20608_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct icm20608_data *drv_data = dev->data;
	const struct icm20608_config *cfg = dev->config;

	if (!cfg->irq_pin.port) {
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	gpio_pin_interrupt_configure_dt(&cfg->irq_pin, GPIO_INT_DISABLE);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = trig;

	gpio_pin_interrupt_configure_dt(&cfg->irq_pin, GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

static void icm20608_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct icm20608_data *drv_data = CONTAINER_OF(cb, struct icm20608_data, gpio_cb);
	const struct icm20608_config *cfg = drv_data->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->irq_pin, GPIO_INT_DISABLE);

#if defined(CONFIG_ICM20608_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ICM20608_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void icm20608_thread_cb(const struct device *dev)
{
	struct icm20608_data *drv_data = dev->data;
	const struct icm20608_config *cfg = dev->config;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev, drv_data->data_ready_trigger);
	}

	gpio_pin_interrupt_configure_dt(&cfg->irq_pin, GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_ICM20608_TRIGGER_OWN_THREAD
static void icm20608_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct icm20608_data *drv_data = p1;

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		icm20608_thread_cb(drv_data->dev);
	}
}
#endif

#ifdef CONFIG_ICM20608_TRIGGER_GLOBAL_THREAD
static void icm20608_work_cb(struct k_work *work)
{
	struct icm20608_data *drv_data = CONTAINER_OF(work, struct icm20608_data, work);

	icm20608_thread_cb(drv_data->dev);
}
#endif

int icm20608_init_interrupt(const struct device *dev)
{
	struct icm20608_data *drv_data = dev->data;
	const struct icm20608_config *cfg = dev->config;

	if (!gpio_is_ready_dt(&cfg->irq_pin)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	drv_data->dev = dev;

	gpio_pin_configure_dt(&cfg->irq_pin, GPIO_INPUT);

	gpio_init_callback(&drv_data->gpio_cb, icm20608_gpio_callback, BIT(cfg->irq_pin.pin));

	if (gpio_add_callback(cfg->irq_pin.port, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Failed to set gpio callback");
		return -EIO;
	}

	/* enable data ready interrupt */
	if (i2c_reg_write_byte_dt(&cfg->i2c, ICM20608_REG_INT_ENABLE, ICM20608_DRDY_EN) < 0) {
		LOG_ERR("Failed to enable data ready interrupt.");
		return -EIO;
	}

#if defined(CONFIG_ICM20608_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ICM20608_THREAD_STACK_SIZE, icm20608_thread, drv_data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ICM20608_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ICM20608_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = icm20608_work_cb;
#endif

	gpio_pin_interrupt_configure_dt(&cfg->irq_pin, GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}
