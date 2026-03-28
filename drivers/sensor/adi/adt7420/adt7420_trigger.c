/*
 * Copyright (c) 2018, 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adt7420.h>

#include "adt7420.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ADT7420, CONFIG_SENSOR_LOG_LEVEL);

static void setup_int(const struct device *dev,
		      bool enable)
{
	const struct adt7420_dev_config *cfg = dev->config;
	gpio_flags_t flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, flags);
}

static void handle_int(const struct device *dev)
{
	struct adt7420_data *drv_data = dev->data;

	setup_int(dev, false);

#if defined(CONFIG_ADT7420_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADT7420_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->int_work);
#endif
}

static void process_int(const struct device *dev)
{
	struct adt7420_data *drv_data = dev->data;
	const struct adt7420_dev_config *cfg = dev->config;
	uint8_t status;

	/* Clear the status */
	if (i2c_reg_read_byte_dt(&cfg->i2c,
				 ADT7420_REG_STATUS, &status) < 0) {
		return;
	}

	if (drv_data->th_handler != NULL) {
		drv_data->th_handler(dev, drv_data->th_trigger);
	}

	setup_int(dev, true);

	/* Check for pin that asserted while we were offline */
	int pv = gpio_pin_get_dt(&cfg->int_gpio);

	if (pv > 0) {
		handle_int(dev);
	}
}

static void adt7420_int_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct adt7420_data *drv_data =
		CONTAINER_OF(cb, struct adt7420_data, int_gpio_cb);

	handle_int(drv_data->dev);
}

static void setup_ct(const struct device *dev,
		      bool enable)
{
	const struct adt7420_dev_config *cfg = dev->config;
	gpio_flags_t flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->ct_gpio, flags);
}

static void handle_ct(const struct device *dev)
{
	struct adt7420_data *drv_data = dev->data;

	setup_ct(dev, false);

#if defined(CONFIG_ADT7420_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADT7420_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->ct_work);
#endif
}

static void process_ct(const struct device *dev)
{
	struct adt7420_data *drv_data = dev->data;
	const struct adt7420_dev_config *cfg = dev->config;
	uint8_t status;

	/* Clear the status */
	if (i2c_reg_read_byte_dt(&cfg->i2c,
				 ADT7420_REG_STATUS, &status) < 0) {
		return;
	}

	if (drv_data->ct_handler != NULL) {
		drv_data->ct_handler(dev, drv_data->ct_trigger);
	}

	setup_ct(dev, true);

	/* Check for pin that asserted while we were offline */
	int pv = gpio_pin_get_dt(&cfg->ct_gpio);

	if (pv > 0) {
		handle_ct(dev);
	}
}

static void adt7420_ct_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct adt7420_data *drv_data =
		CONTAINER_OF(cb, struct adt7420_data, ct_gpio_cb);

	handle_ct(drv_data->dev);
}

#if defined(CONFIG_ADT7420_TRIGGER_OWN_THREAD)
static void adt7420_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct adt7420_data *drv_data = p1;
	const struct adt7420_dev_config *cfg = drv_data->dev->config;

	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);

		/* Check which interrupt(s) fired and process them */
		if (cfg->int_gpio.port && gpio_pin_get_dt(&cfg->int_gpio) > 0) {
			process_int(drv_data->dev);
		}

		if (cfg->ct_gpio.port && gpio_pin_get_dt(&cfg->ct_gpio) > 0) {
			process_ct(drv_data->dev);
		}
	}
}

#elif defined(CONFIG_ADT7420_TRIGGER_GLOBAL_THREAD)
static void adt7420_work_int_cb(struct k_work *work)
{
	struct adt7420_data *drv_data =
		CONTAINER_OF(work, struct adt7420_data, int_work);

	process_int(drv_data->dev);
}

static void adt7420_work_ct_cb(struct k_work *work)
{
	struct adt7420_data *drv_data =
		CONTAINER_OF(work, struct adt7420_data, ct_work);

	process_ct(drv_data->dev);
}
#endif

int adt7420_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct adt7420_data *drv_data = dev->data;
	const struct adt7420_dev_config *cfg = dev->config;

	if (!cfg->int_gpio.port && !cfg->ct_gpio.port) {
		return -ENOTSUP;
	}

	setup_int(dev, false);
	setup_ct(dev, false);

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	if (trig->chan == SENSOR_CHAN_ADT7420_CRIT_TEMP) {
		drv_data->ct_handler = handler;
		if (handler != NULL) {
			drv_data->ct_trigger = trig;

			setup_ct(dev, true);

			/* Check whether already asserted */
			int pv = gpio_pin_get_dt(&cfg->ct_gpio);

			if (pv > 0) {
				handle_ct(dev);
			}
		}
	} else {
		drv_data->th_handler = handler;

		if (handler != NULL) {
			drv_data->th_trigger = trig;

			setup_int(dev, true);

			/* Check whether already asserted */
			int pv = gpio_pin_get_dt(&cfg->int_gpio);

			if (pv > 0) {
				handle_int(dev);
			}
		}
	}

	return 0;
}

int adt7420_init_interrupt(const struct device *dev)
{
	struct adt7420_data *drv_data = dev->data;
	const struct adt7420_dev_config *cfg = dev->config;
	int rc;

	drv_data->dev = dev;

	/* INT GPIO */
	if (cfg->int_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->int_gpio)) {
			LOG_ERR("%s: device %s is not ready", dev->name,
				cfg->int_gpio.port->name);
			return -ENODEV;
		}
		gpio_init_callback(&drv_data->int_gpio_cb,
				adt7420_int_gpio_callback,
				BIT(cfg->int_gpio.pin));

		rc = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT | cfg->int_gpio.dt_flags);
		if (rc < 0) {
			return rc;
		}

		rc = gpio_add_callback(cfg->int_gpio.port, &drv_data->int_gpio_cb);
		if (rc < 0) {
			return rc;
		}
#if defined(CONFIG_ADT7420_TRIGGER_GLOBAL_THREAD)
		drv_data->int_work.handler = adt7420_work_int_cb;
#endif
	}

	/* CT GPIO */
	if (cfg->ct_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->ct_gpio)) {
			LOG_ERR("%s: device %s is not ready", dev->name,
				cfg->ct_gpio.port->name);
			return -ENODEV;
		}
		gpio_init_callback(&drv_data->ct_gpio_cb,
				adt7420_ct_gpio_callback,
				BIT(cfg->ct_gpio.pin));

		rc = gpio_pin_configure_dt(&cfg->ct_gpio, GPIO_INPUT | cfg->ct_gpio.dt_flags);
		if (rc < 0) {
			return rc;
		}

		rc = gpio_add_callback(cfg->ct_gpio.port, &drv_data->ct_gpio_cb);
		if (rc < 0) {
			return rc;
		}
#if defined(CONFIG_ADT7420_TRIGGER_GLOBAL_THREAD)
		drv_data->ct_work.handler = adt7420_work_ct_cb;
#endif
	}

#if defined(CONFIG_ADT7420_TRIGGER_OWN_THREAD)
	/* Create single thread to handle both interrupts */
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ADT7420_THREAD_STACK_SIZE,
			adt7420_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_ADT7420_THREAD_PRIORITY),
			0, K_NO_WAIT);

	k_thread_name_set(&drv_data->thread, dev->name);
#endif
	return 0;
}
