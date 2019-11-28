/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/i2c.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include "lsm6dsl.h"

LOG_MODULE_DECLARE(LSM6DSL, CONFIG_SENSOR_LOG_LEVEL);

int lsm6dsl_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct lsm6dsl_data *drv_data = dev->driver_data;

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	gpio_pin_disable_callback(drv_data->gpio, DT_INST_0_ST_LSM6DSL_IRQ_GPIOS_PIN);

	drv_data->data_ready_handler = handler;
	if (handler == NULL) {
		return 0;
	}

	drv_data->data_ready_trigger = *trig;

	gpio_pin_enable_callback(drv_data->gpio, DT_INST_0_ST_LSM6DSL_IRQ_GPIOS_PIN);

	return 0;
}

static void lsm6dsl_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, u32_t pins)
{
	struct lsm6dsl_data *drv_data =
		CONTAINER_OF(cb, struct lsm6dsl_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, DT_INST_0_ST_LSM6DSL_IRQ_GPIOS_PIN);

#if defined(CONFIG_LSM6DSL_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_LSM6DSL_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void lsm6dsl_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct lsm6dsl_data *drv_data = dev->driver_data;

	if (drv_data->data_ready_handler != NULL) {
		drv_data->data_ready_handler(dev,
					     &drv_data->data_ready_trigger);
	}

	gpio_pin_enable_callback(drv_data->gpio, DT_INST_0_ST_LSM6DSL_IRQ_GPIOS_PIN);
}

#ifdef CONFIG_LSM6DSL_TRIGGER_OWN_THREAD
static void lsm6dsl_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct lsm6dsl_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

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

int lsm6dsl_init_interrupt(struct device *dev)
{
	struct lsm6dsl_data *drv_data = dev->driver_data;

	/* setup data ready gpio interrupt */
	drv_data->gpio = device_get_binding(DT_INST_0_ST_LSM6DSL_IRQ_GPIOS_CONTROLLER);
	if (drv_data->gpio == NULL) {
		LOG_ERR("Cannot get pointer to %s device.",
			    DT_INST_0_ST_LSM6DSL_IRQ_GPIOS_CONTROLLER);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, DT_INST_0_ST_LSM6DSL_IRQ_GPIOS_PIN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb,
			   lsm6dsl_gpio_callback,
			   BIT(DT_INST_0_ST_LSM6DSL_IRQ_GPIOS_PIN));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback.");
		return -EIO;
	}

	/* enable data-ready interrupt */
	if (drv_data->hw_tf->update_reg(drv_data,
			       LSM6DSL_REG_INT1_CTRL,
			       LSM6DSL_MASK_INT1_CTRL_DRDY_XL |
			       LSM6DSL_MASK_INT1_CTRL_DRDY_G,
			       BIT(LSM6DSL_SHIFT_INT1_CTRL_DRDY_XL) |
			       BIT(LSM6DSL_SHIFT_INT1_CTRL_DRDY_G)) < 0) {
		LOG_ERR("Could not enable data-ready interrupt.");
		return -EIO;
	}

#if defined(CONFIG_LSM6DSL_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_LSM6DSL_THREAD_STACK_SIZE,
			(k_thread_entry_t)lsm6dsl_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_LSM6DSL_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_LSM6DSL_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = lsm6dsl_work_cb;
	drv_data->dev = dev;
#endif

	gpio_pin_enable_callback(drv_data->gpio, DT_INST_0_ST_LSM6DSL_IRQ_GPIOS_PIN);

	return 0;
}
