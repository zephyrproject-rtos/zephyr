/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2023 Linumiz
 */

#include "mc3419.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(MC3419, CONFIG_SENSOR_LOG_LEVEL);

static void mc3419_gpio_callback(const struct device *dev,
				 struct gpio_callback *cb,
				 uint32_t pin_mask)
{
	struct mc3419_driver_data *data = CONTAINER_OF(cb,
					  struct mc3419_driver_data, gpio_cb);

	const struct mc3419_config *cfg = data->gpio_dev->config;

	if ((pin_mask & BIT(cfg->int_gpio.pin)) == 0U) {
		return;
	}

#if defined(CONFIG_MC3419_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#else
	k_work_submit(&data->work);
#endif
}

static void mc3419_process_int(const struct device *dev)
{
	int ret = 0;
	const struct mc3419_config *cfg = dev->config;
	const struct mc3419_driver_data *data = dev->data;
	uint8_t int_source = 0;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, MC3419_REG_INT_STATUS, &int_source);
	if (ret < 0) {
		goto exit;
	}

	if (int_source & MC3419_DATA_READY_MASK) {
		if (data->handler[MC3419_TRIG_DATA_READY]) {
			data->handler[MC3419_TRIG_DATA_READY](dev,
				data->trigger[MC3419_TRIG_DATA_READY]);
		}
	}

	if (int_source & MC3419_ANY_MOTION_MASK) {
		if (data->handler[MC3419_TRIG_ANY_MOTION]) {
			data->handler[MC3419_TRIG_ANY_MOTION](dev,
				data->trigger[MC3419_TRIG_ANY_MOTION]);
		}
	}
exit:
	ret = i2c_reg_write_byte_dt(&cfg->i2c, MC3419_REG_INT_STATUS,
				    MC3419_INT_CLEAR);
	if (ret < 0) {
		LOG_ERR("Failed to clear interrupt (%d)", ret);
	}
}

#if defined(CONFIG_MC3419_TRIGGER_OWN_THREAD)
static void mc3419_thread(struct mc3419_driver_data *data)
{
	while (1) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		mc3419_process_int(data->gpio_dev);
	}
}
#else
static void mc3419_work_cb(struct k_work *work)
{
	struct mc3419_driver_data *data = CONTAINER_OF(work,
					  struct mc3419_driver_data, work);

	mc3419_process_int(data->gpio_dev);
}
#endif

int mc3419_configure_trigger(const struct device *dev,
			     const struct sensor_trigger *trig,
			     sensor_trigger_handler_t handler)
{
	int ret = 0;
	uint8_t buf = 0;
	const struct mc3419_config *cfg = dev->config;
	struct mc3419_driver_data *data = dev->data;

	if (!(trig->type & SENSOR_TRIG_DATA_READY) &&
	    !(trig->type & SENSOR_TRIG_MOTION)) {
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	if (trig->type & SENSOR_TRIG_DATA_READY) {
		data->handler[MC3419_TRIG_DATA_READY] = handler;
		data->trigger[MC3419_TRIG_DATA_READY] = trig;
		buf |= MC3419_DATA_READY_MASK;
	}

	if (trig->type & SENSOR_TRIG_MOTION) {
		uint8_t int_mask = MC3419_ANY_MOTION_MASK;

		buf |= MC3419_ANY_MOTION_MASK;
		data->handler[MC3419_TRIG_ANY_MOTION] = handler;
		data->trigger[MC3419_TRIG_ANY_MOTION] = trig;

		ret = i2c_reg_update_byte_dt(&cfg->i2c, MC3419_REG_MOTION_CTRL,
					     int_mask, handler ? int_mask : 0);
		if (ret < 0) {
			LOG_ERR("Failed to configure motion interrupt (%d)", ret);
			return ret;
		}
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, MC3419_REG_INT_CTRL,
				     buf, buf);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt (%d)", ret);
		return ret;
	}

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_FALLING);

	return ret;
}

int mc3419_trigger_init(const struct device *dev)
{
	int ret = 0;
	struct mc3419_driver_data *data = dev->data;
	const struct mc3419_config *cfg = dev->config;

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("GPIO port %s not ready", cfg->int_gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt gpio");
		return ret;
	}

	data->gpio_dev = dev;

#if defined(CONFIG_MC3419_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, 1);
	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_MC3419_THREAD_STACK_SIZE,
			(k_thread_entry_t)mc3419_thread, data, NULL,
			NULL, K_PRIO_COOP(CONFIG_MC3419_THREAD_PRIORITY), 0,
			K_NO_WAIT);
#else
	k_work_init(&data->work, mc3419_work_cb);
#endif
	gpio_init_callback(&data->gpio_cb, mc3419_gpio_callback,
			   BIT(cfg->int_gpio.pin));
	ret = gpio_add_callback(cfg->int_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set int callback");
		return ret;
	}

	if (cfg->int_cfg) {
		ret = i2c_reg_write_byte_dt(&cfg->i2c, MC3419_REG_COMM_CTRL,
					    MC3419_INT_ROUTE);
		if (ret < 0) {
			LOG_ERR("Failed to route the interrupt to INT2 pin (%d)", ret);
			return ret;
		}
	}

	return 0;
}
