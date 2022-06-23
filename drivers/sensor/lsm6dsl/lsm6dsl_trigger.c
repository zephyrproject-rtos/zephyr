/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lsm6dsl

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "lsm6dsl.h"

LOG_MODULE_DECLARE(LSM6DSL, CONFIG_SENSOR_LOG_LEVEL);

static inline void setup_irq(const struct device *dev, bool enable)
{
	const struct lsm6dsl_config *config = dev->config;

	unsigned int flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&config->int_gpio, flags);
}

static inline void handle_irq(const struct device *dev)
{
	struct lsm6dsl_data *drv_data = dev->data;

	setup_irq(dev, false);

#if defined(CONFIG_LSM6DSL_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_LSM6DSL_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

int lsm6dsl_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct lsm6dsl_config *config = dev->config;
	struct lsm6dsl_data *drv_data = dev->data;

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	/* If irq_gpio is not configured in DT just return error */
	if (!config->int_gpio.port) {
		LOG_ERR("triggers not supported");
		return -ENOTSUP;
	}

	setup_irq(dev, false);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = *trig;

	setup_irq(dev, true);
	if (gpio_pin_get_dt(&config->int_gpio) > 0) {
		handle_irq(dev);
	}

	return 0;
}

static void lsm6dsl_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct lsm6dsl_data *drv_data =
		CONTAINER_OF(cb, struct lsm6dsl_data, gpio_cb);

	ARG_UNUSED(pins);

	handle_irq(drv_data->dev);
}

static void lsm6dsl_thread_cb(const struct device *dev)
{
	struct lsm6dsl_data *drv_data = dev->data;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     &drv_data->data_ready_trigger);
	}

	setup_irq(dev, true);
}

#ifdef CONFIG_LSM6DSL_TRIGGER_OWN_THREAD
static void lsm6dsl_thread(const struct device *dev)
{
	struct lsm6dsl_data *drv_data = dev->data;

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		lsm6dsl_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_LSM6DSL_TRIGGER_GLOBAL_THREAD
static void lsm6dsl_work_cb(struct k_work *work)
{
	struct lsm6dsl_data *drv_data =
		CONTAINER_OF(work, struct lsm6dsl_data, work);

	lsm6dsl_thread_cb(drv_data->dev);
}
#endif

int lsm6dsl_init_interrupt(const struct device *dev)
{
	const struct lsm6dsl_config *config = dev->config;
	struct lsm6dsl_data *drv_data = dev->data;

	if (!device_is_ready(config->int_gpio.port)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);

	gpio_init_callback(&drv_data->gpio_cb,
			   lsm6dsl_gpio_callback, BIT(config->int_gpio.pin));

	if (gpio_add_callback(config->int_gpio.port, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback.");
		return -EIO;
	}

	/* enable data-ready interrupt */
	if (drv_data->hw_tf->update_reg(dev,
			       LSM6DSL_REG_INT1_CTRL,
			       LSM6DSL_MASK_INT1_CTRL_DRDY_XL |
			       LSM6DSL_MASK_INT1_CTRL_DRDY_G,
			       BIT(LSM6DSL_SHIFT_INT1_CTRL_DRDY_XL) |
			       BIT(LSM6DSL_SHIFT_INT1_CTRL_DRDY_G)) < 0) {
		LOG_ERR("Could not enable data-ready interrupt.");
		return -EIO;
	}

	drv_data->dev = dev;

#if defined(CONFIG_LSM6DSL_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_LSM6DSL_THREAD_STACK_SIZE,
			(k_thread_entry_t)lsm6dsl_thread, (void *)dev,
			NULL, NULL, K_PRIO_COOP(CONFIG_LSM6DSL_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_LSM6DSL_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = lsm6dsl_work_cb;
#endif

	setup_irq(dev, true);

	return 0;
}
