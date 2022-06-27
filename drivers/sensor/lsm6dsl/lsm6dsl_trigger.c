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

static inline void setup_irq(struct lsm6dsl_data *drv_data,
			     uint32_t irq_pin, bool enable)
{
	unsigned int flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure(drv_data->gpio,
				     irq_pin, flags);
}

static inline void handle_irq(struct lsm6dsl_data *drv_data,
			      uint32_t irq_pin)
{
	setup_irq(drv_data, irq_pin, false);

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
	if (!drv_data->gpio) {
		LOG_ERR("triggers not supported");
		return -ENOTSUP;
	}

	setup_irq(drv_data, config->irq_pin, false);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = *trig;

	setup_irq(drv_data, config->irq_pin, true);
	if (gpio_pin_get(drv_data->gpio, config->irq_pin) > 0) {
		handle_irq(drv_data, config->irq_pin);
	}

	return 0;
}

static void lsm6dsl_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct lsm6dsl_data *drv_data =
		CONTAINER_OF(cb, struct lsm6dsl_data, gpio_cb);
	const struct lsm6dsl_config *config = drv_data->dev->config;

	ARG_UNUSED(pins);

	handle_irq(drv_data, config->irq_pin);
}

static void lsm6dsl_thread_cb(const struct device *dev)
{
	const struct lsm6dsl_config *config = dev->config;
	struct lsm6dsl_data *drv_data = dev->data;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     &drv_data->data_ready_trigger);
	}

	setup_irq(drv_data, config->irq_pin, true);
}

#ifdef CONFIG_LSM6DSL_TRIGGER_OWN_THREAD
static void lsm6dsl_thread(struct lsm6dsl_data *drv_data)
{
	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		lsm6dsl_thread_cb(drv_data->dev);
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

	/* setup data ready gpio interrupt */
	drv_data->gpio = device_get_binding(config->irq_dev_name);
	if (drv_data->gpio == NULL) {
		LOG_INF("Cannot get pointer for irq_dev_name");
		goto end;
	}

	gpio_pin_configure(drv_data->gpio, config->irq_pin,
			   GPIO_INPUT | config->irq_flags);

	gpio_init_callback(&drv_data->gpio_cb,
			   lsm6dsl_gpio_callback, BIT(config->irq_pin));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
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
			(k_thread_entry_t)lsm6dsl_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_LSM6DSL_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_LSM6DSL_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = lsm6dsl_work_cb;
#endif

	setup_irq(drv_data, config->irq_pin, true);

end:
	return 0;
}
