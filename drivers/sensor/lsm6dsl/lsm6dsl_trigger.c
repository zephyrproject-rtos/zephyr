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

static int lsm6dsl_sign_motion_config(const struct device *dev)
{
	struct lsm6dsl_data *drv_data = dev->data;
	uint8_t val;

	/* enable embedded functions register (Bank A only) */
	val = BIT(LSM6DSL_SHIFT_FUNC_CFG_EN);
	if (drv_data->hw_tf->write_data(dev, LSM6DSL_REG_FUNC_CFG_ACCESS, &val, sizeof(val)) < 0) {
		LOG_ERR("Could not enable FUNC_CFG_ACCESS register.");
		return -EIO;
	}

	/* set significant motion threshold register */
	val = CONFIG_LSM6DSL_TRIGGER_SIGN_MOTION_THRESH;
	if (drv_data->hw_tf->write_data(dev, LSM6DSL_BANK_A_SM_THS, &val, sizeof(val)) < 0) {
		LOG_ERR("Could not set significant motion threshold value.");
		return -EIO;
	}

	/* disable embedded functions register */
	val = 0;
	if (drv_data->hw_tf->write_data(dev, LSM6DSL_REG_FUNC_CFG_ACCESS, &val, sizeof(val)) < 0) {
		LOG_ERR("Could not disable FUNC_CFG_ACCESS register.");
		return -EIO;
	}

	/* enable control 10 register */
	val = BIT(LSM6DSL_SHIFT_CTRL10_C_SIGN_MOTION_EN) | BIT(LSM6DSL_SHIFT_CTRL10_C_FUNC_EN);
	if (drv_data->hw_tf->write_data(dev, LSM6DSL_REG_CTRL10_C, &val, sizeof(val)) < 0) {
		LOG_ERR("Could not enable significant motion in CTRL10 register.");
		return -EIO;
	}

	/* enable only significant motion interrupt */
	val = BIT(LSM6DSL_SHIFT_INT1_CTRL_SIGN_MOT);
	if (drv_data->hw_tf->write_data(dev, LSM6DSL_REG_INT1_CTRL, &val, sizeof(val)) < 0) {
		LOG_ERR("Could not enable interrupt for significant motion detection.");
		return -EIO;
	}

	return 0;
}

static int lsm6dsl_drdy_config(const struct device *dev)
{
	struct lsm6dsl_data *drv_data = dev->data;
	uint8_t val;

	/* disable control 10 register */
	val = 0;
	if (drv_data->hw_tf->update_reg(dev, LSM6DSL_REG_CTRL10_C,
					LSM6DSL_MASK_CTRL10_C_SIGN_MOTION_EN |
						LSM6DSL_MASK_CTRL10_C_FUNC_EN,
					val) < 0) {
		LOG_ERR("Could not disable significant motion in CTRL10 register.");
		return -EIO;
	}

	/* enable only data-ready interrupt */
	val = BIT(LSM6DSL_MASK_INT1_CTRL_DRDY_XL) | BIT(LSM6DSL_SHIFT_INT1_CTRL_DRDY_G);
	if (drv_data->hw_tf->write_data(dev, LSM6DSL_REG_INT1_CTRL, &val, sizeof(val)) < 0) {
		LOG_ERR("Could not enable data-ready interrupt.");
		return -EIO;
	}

	return 0;
}

int lsm6dsl_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct lsm6dsl_config *config = dev->config;
	struct lsm6dsl_data *drv_data = dev->data;

	/* If irq_gpio is not configured in DT just return error */
	if (!config->int_gpio.port) {
		LOG_ERR("triggers not supported");
		return -ENOTSUP;
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
	setup_irq(dev, false);

	drv_data->data_ready_handler = handler;
	/* Disable other handlers */
	drv_data->motion_handler = NULL;
	if (handler == NULL) {
		return 0;
	}
		lsm6dsl_drdy_config(dev);

	drv_data->data_ready_trigger = trig;

	setup_irq(dev, true);
	if (gpio_pin_get_dt(&config->int_gpio) > 0) {
		handle_irq(dev);
	}
		break;

	case SENSOR_TRIG_MOTION:
		setup_irq(dev, false);

		drv_data->motion_handler = handler;
		/* Disable other handlers */
		drv_data->data_ready_handler = NULL;
		if (handler == NULL) {
			return 0;
		}
		lsm6dsl_sign_motion_config(dev);
		drv_data->motion_trigger = trig;

		setup_irq(dev, true);
		if (gpio_pin_get_dt(&config->int_gpio) > 0) {
			handle_irq(dev);
		}
		break;

	default:
		LOG_WRN("Trigger %u not supported.", trig->type);
		return -ENOTSUP;
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
		drv_data->data_ready_handler(dev, drv_data->data_ready_trigger);
	}

	if (drv_data->motion_handler != NULL) {
		drv_data->motion_handler(dev, drv_data->motion_trigger);
	}

	setup_irq(dev, true);
}

#ifdef CONFIG_LSM6DSL_TRIGGER_OWN_THREAD
static void lsm6dsl_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
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

	if (!gpio_is_ready_dt(&config->int_gpio)) {
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

	/* enable interrupt based on CONFIG_LSM6DSL_TRIGGER_DEFAULT value */
#if CONFIG_LSM6DSL_TRIGGER_DEFAULT == 0
	lsm6dsl_drdy_config(dev);
#elif CONFIG_LSM6DSL_TRIGGER_DEFAULT == 1
	lsm6dsl_sign_motion_config(dev);
#endif

	drv_data->dev = dev;

#if defined(CONFIG_LSM6DSL_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_LSM6DSL_THREAD_STACK_SIZE,
			lsm6dsl_thread, (void *)dev,
			NULL, NULL, K_PRIO_COOP(CONFIG_LSM6DSL_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_LSM6DSL_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = lsm6dsl_work_cb;
#endif

	return 0;
}
