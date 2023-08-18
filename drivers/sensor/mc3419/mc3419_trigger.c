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

	const struct mc3419_config *cfg = data->dev->config;

	if ((pin_mask & BIT(cfg->int_gpio.pin)) == 0U) {
		return;
	}

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_DISABLE);
#if defined CONFIG_MC3419_TRIGGER_OWN_THREAD
	k_sem_give(&data->trig_sem);
#elif defined CONFIG_MC3419_TRIGGER_GLOBAL_THREAD
	k_work_submit(&data->work);
#endif
}

static void mc3419_drdy_interrupt_handler(const struct device *dev)
{
	struct mc3419_driver_data *data = dev->data;

	if (data->handler_drdy) {
		data->handler_drdy(dev, data->trigger_drdy);
	}
}

#if defined CONFIG_MC3419_MOTION
static void mc3419_motion_interrupt_handler(const struct device *dev)
{
	struct mc3419_driver_data *data = dev->data;

	if (data->motion_handler) {
		data->motion_handler(dev, data->motion_trigger);
	}
}
#endif

static void mc3419_interrupt_handler(const struct device *dev)
{
	int ret = 0;
	const struct mc3419_config *cfg = dev->config;
	uint8_t int_source = 0;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, MC3419_REG_INT_STATUS, &int_source);
	if (ret < 0) {
		goto exit;
	}

	if (int_source & MC3419_DATA_READY_MASK) {
		mc3419_drdy_interrupt_handler(dev);
	}
#if defined CONFIG_MC3419_MOTION
	if (int_source & MC3419_ANY_MOTION_MASK) {
		mc3419_motion_interrupt_handler(dev);
	}
#endif

exit:
	ret = i2c_reg_write_byte_dt(&cfg->i2c, MC3419_REG_INT_STATUS, MC3419_INT_CLEAR);
	if (ret < 0) {
		goto exit;
	}

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_FALLING);
}

#if defined CONFIG_MC3419_TRIGGER_OWN_THREAD
static void mc3419_thread(struct mc3419_driver_data *data)
{
	while (1) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		mc3419_interrupt_handler(data->dev);
	}
}
#endif

#if defined CONFIG_MC3419_TRIGGER_GLOBAL_THREAD
static void mc3419_work_cb(struct k_work *work)
{
	struct mc3419_driver_data *data = CONTAINER_OF(work,
					struct mc3419_driver_data, work);

	mc3419_interrupt_handler(data->dev);
}
#endif

int mc3419_trigger_set(const struct device *dev,
				const struct sensor_trigger *trig,
				sensor_trigger_handler_t handler)
{
	int ret = 0;
	uint8_t buf = 0;
	const struct mc3419_config *cfg = dev->config;
	struct mc3419_driver_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);

	ret = mc3419_set_op_mode(cfg, MC3419_MODE_STANDBY);
	if (ret < 0) {
		return ret;
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		data->handler_drdy = handler;
		data->trigger_drdy = trig;
		buf = MC3419_DRDY_CTRL;
		break;
#if defined CONFIG_MC3419_MOTION
	case SENSOR_TRIG_MOTION:
		uint8_t int_mask = MC3419_ANY_MOTION_MASK;
		buf = MC3419_MOTION_CTRL;
		data->motion_handler = handler;
		data->motion_trigger = trig;

		ret = i2c_reg_update_byte_dt(&cfg->i2c, MC3419_REG_MOTION_CTRL, int_mask,
									handler ? int_mask : 0);
		if (ret < 0) {
			LOG_ERR("Failed to configure motion interrupt (%d)", ret);
			goto error;
		}

		break;
#endif
	default:
		LOG_ERR("Unsupported sensor trigger");
		ret = -ENOTSUP;
		goto error;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, MC3419_REG_INT_CTRL, buf);
	if (ret < 0) {
		LOG_ERR("Failed to configure trigger (%d)", ret);
		goto error;
	}

#if defined CONFIG_MC3419_DRDY_INT2 || defined CONFIG_MC3419_MOTION_INT2
	ret = i2c_reg_write_byte_dt(&cfg->i2c, MC3419_REG_COMM_CTRL, MC3419_INT_ROUTE);
	if (ret < 0) {
		LOG_ERR("Failed to route the interrupt to INT2 pin (%d)", ret);
		goto error;
	}
#endif
	ret = mc3419_set_op_mode(cfg, MC3419_MODE_WAKE);
	if (ret < 0) {
		LOG_ERR("Failed to set wake mode");
		goto error;
	}

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_FALLING);

	LOG_INF("Trigger set\n");
error:
	k_sem_give(&data->sem);

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

	data->dev = dev;

#if defined CONFIG_MC3419_TRIGGER_OWN_THREAD
	k_sem_init(&data->trig_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_MC3419_THREAD_STACK_SIZE,
			(k_thread_entry_t)mc3419_thread, data, NULL,
			NULL, K_PRIO_COOP(CONFIG_MC3419_THREAD_PRIORITY), 0,
			K_NO_WAIT);
#endif
#if defined CONFIG_MC3419_TRIGGER_GLOBAL_THREAD
	data->work.handler = mc3419_work_cb;
#endif
	gpio_init_callback(&data->gpio_cb, mc3419_gpio_callback,
					BIT(cfg->int_gpio.pin));
	ret = gpio_add_callback(cfg->int_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set int callback");
		return ret;
	}

	return 0;
}
