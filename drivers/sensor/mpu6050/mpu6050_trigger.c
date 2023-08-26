/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "mpu6050.h"

LOG_MODULE_DECLARE(MPU6050, CONFIG_SENSOR_LOG_LEVEL);

int mpu6050_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct mpu6050_data *drv_data = dev->data;
	const struct mpu6050_config *cfg = dev->config;

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = trig;

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

static void mpu6050_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct mpu6050_data *drv_data =
		CONTAINER_OF(cb, struct mpu6050_data, gpio_cb);
	const struct mpu6050_config *cfg = drv_data->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_MPU6050_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_MPU6050_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void mpu6050_thread_cb(const struct device *dev)
{
	struct mpu6050_data *drv_data = dev->data;
	const struct mpu6050_config *cfg = dev->config;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     drv_data->data_ready_trigger);
	}

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_MPU6050_TRIGGER_OWN_THREAD
static void mpu6050_thread(struct mpu6050_data *drv_data)
{
	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		mpu6050_thread_cb(drv_data->dev);
	}
}
#endif

#ifdef CONFIG_MPU6050_TRIGGER_GLOBAL_THREAD
static void mpu6050_work_cb(struct k_work *work)
{
	struct mpu6050_data *drv_data =
		CONTAINER_OF(work, struct mpu6050_data, work);

	mpu6050_thread_cb(drv_data->dev);
}
#endif

int mpu6050_init_interrupt(const struct device *dev)
{
	struct mpu6050_data *drv_data = dev->data;
	const struct mpu6050_config *cfg = dev->config;

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	drv_data->dev = dev;

	gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);

	gpio_init_callback(&drv_data->gpio_cb,
			   mpu6050_gpio_callback,
			   BIT(cfg->int_gpio.pin));

	if (gpio_add_callback(cfg->int_gpio.port, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Failed to set gpio callback");
		return -EIO;
	}

	/* enable data ready interrupt */
	if (i2c_reg_write_byte_dt(&cfg->i2c, MPU6050_REG_INT_EN,
				  MPU6050_DRDY_EN) < 0) {
		LOG_ERR("Failed to enable data ready interrupt.");
		return -EIO;
	}

#if defined(CONFIG_MPU6050_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_MPU6050_THREAD_STACK_SIZE,
			(k_thread_entry_t)mpu6050_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_MPU6050_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_MPU6050_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = mpu6050_work_cb;
#endif

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}
