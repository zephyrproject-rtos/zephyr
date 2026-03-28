/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "mpu9250.h"

LOG_MODULE_DECLARE(MPU9250, CONFIG_SENSOR_LOG_LEVEL);


#define MPU9250_REG_INT_EN	0x38
#define MPU9250_DRDY_EN		BIT(0)


int mpu9250_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct mpu9250_data *drv_data = dev->data;
	const struct mpu9250_config *cfg = dev->config;
	int ret;

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_pin, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to disable gpio interrupt.");
		return ret;
	}

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = trig;

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_pin,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to enable gpio interrupt.");
		return ret;
	}

	return 0;
}

static void mpu9250_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct mpu9250_data *drv_data =
		CONTAINER_OF(cb, struct mpu9250_data, gpio_cb);
	const struct mpu9250_config *cfg = drv_data->dev->config;
	int ret;

	ARG_UNUSED(pins);

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_pin, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Disabling gpio interrupt failed with err: %d", ret);
		return;
	}

#if defined(CONFIG_MPU9250_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_MPU9250_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void mpu9250_thread_cb(const struct device *dev)
{
	struct mpu9250_data *drv_data = dev->data;
	const struct mpu9250_config *cfg = dev->config;
	int ret;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     drv_data->data_ready_trigger);
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_pin,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Enabling gpio interrupt failed with err: %d", ret);
	}

}

#ifdef CONFIG_MPU9250_TRIGGER_OWN_THREAD
static void mpu9250_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct mpu9250_data *drv_data = p1;

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		mpu9250_thread_cb(drv_data->dev);
	}
}
#endif

#ifdef CONFIG_MPU9250_TRIGGER_GLOBAL_THREAD
static void mpu9250_work_cb(struct k_work *work)
{
	struct mpu9250_data *drv_data =
		CONTAINER_OF(work, struct mpu9250_data, work);

	mpu9250_thread_cb(drv_data->dev);
}
#endif

int mpu9250_init_interrupt(const struct device *dev)
{
	struct mpu9250_data *drv_data = dev->data;
	const struct mpu9250_config *cfg = dev->config;
	int ret;

	/* setup data ready gpio interrupt */
	if (!gpio_is_ready_dt(&cfg->int_pin)) {
		LOG_ERR("Interrupt pin is not ready.");
		return -EIO;
	}

	drv_data->dev = dev;

	ret = gpio_pin_configure_dt(&cfg->int_pin, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt pin.");
		return ret;
	}

	gpio_init_callback(&drv_data->gpio_cb,
			   mpu9250_gpio_callback,
			   BIT(cfg->int_pin.pin));

	ret = gpio_add_callback(cfg->int_pin.port, &drv_data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set gpio callback.");
		return ret;
	}

	/* enable data ready interrupt */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, MPU9250_REG_INT_EN,
				    MPU9250_DRDY_EN);
	if (ret < 0) {
		LOG_ERR("Failed to enable data ready interrupt.");
		return ret;
	}

#if defined(CONFIG_MPU9250_TRIGGER_OWN_THREAD)
	ret = k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);
	if (ret < 0) {
		LOG_ERR("Failed to enable semaphore");
		return ret;
	}

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_MPU9250_THREAD_STACK_SIZE,
			mpu9250_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_MPU9250_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_MPU9250_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = mpu9250_work_cb;
#endif

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_pin,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to enable interrupt");
		return ret;
	}

	return 0;
}
