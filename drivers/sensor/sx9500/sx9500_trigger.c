/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT semtech_sx9500

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

#include "sx9500.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(SX9500, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_SX9500_TRIGGER_OWN_THREAD
static K_KERNEL_STACK_DEFINE(sx9500_thread_stack, CONFIG_SX9500_THREAD_STACK_SIZE);
static struct k_thread sx9500_thread;
#endif

int sx9500_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct sx9500_data *data = dev->data;
	const struct sx9500_config *cfg = dev->config;

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		if (i2c_reg_update_byte_dt(&cfg->i2c,
					   SX9500_REG_IRQ_MSK,
					   SX9500_CONV_DONE_IRQ,
					   SX9500_CONV_DONE_IRQ) < 0) {
			return -EIO;
		}
		data->handler_drdy = handler;
		data->trigger_drdy = trig;
		break;

	case SENSOR_TRIG_NEAR_FAR:
		if (i2c_reg_update_byte_dt(&cfg->i2c,
					   SX9500_REG_IRQ_MSK,
					   SX9500_NEAR_FAR_IRQ,
					   SX9500_NEAR_FAR_IRQ) < 0) {
			return -EIO;
		}
		data->handler_near_far = handler;
		data->trigger_near_far = trig;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static void sx9500_gpio_thread_cb(const struct device *dev)
{
	struct sx9500_data *data = dev->data;
	const struct sx9500_config *cfg = dev->config;
	uint8_t reg_val;

	if (i2c_reg_read_byte_dt(&cfg->i2c, SX9500_REG_IRQ_SRC, &reg_val) < 0) {
		LOG_DBG("sx9500: error reading IRQ source register");
		return;
	}

	if ((reg_val & SX9500_CONV_DONE_IRQ) && data->handler_drdy) {
		data->handler_drdy(dev, data->trigger_drdy);
	}

	if ((reg_val & SX9500_NEAR_FAR_IRQ) && data->handler_near_far) {
		data->handler_near_far(dev, data->trigger_near_far);
	}
}

#ifdef CONFIG_SX9500_TRIGGER_OWN_THREAD

static void sx9500_gpio_cb(const struct device *port,
			   struct gpio_callback *cb, uint32_t pins)
{
	struct sx9500_data *data =
		CONTAINER_OF(cb, struct sx9500_data, gpio_cb);

	ARG_UNUSED(pins);

	k_sem_give(&data->sem);
}

static void sx9500_thread_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct sx9500_data *data = p1;

	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		sx9500_gpio_thread_cb(data->dev);
	}
}

#else /* CONFIG_SX9500_TRIGGER_GLOBAL_THREAD */

static void sx9500_gpio_cb(const struct device *port,
			   struct gpio_callback *cb, uint32_t pins)
{
	struct sx9500_data *data =
		CONTAINER_OF(cb, struct sx9500_data, gpio_cb);

	ARG_UNUSED(pins);

	k_work_submit(&data->work);
}
#endif /* CONFIG_SX9500_TRIGGER_GLOBAL_THREAD */

#ifdef CONFIG_SX9500_TRIGGER_GLOBAL_THREAD
static void sx9500_work_cb(struct k_work *work)
{
	struct sx9500_data *data =
		CONTAINER_OF(work, struct sx9500_data, work);

	sx9500_gpio_thread_cb(data->dev);
}
#endif

int sx9500_setup_interrupt(const struct device *dev)
{
	struct sx9500_data *data = dev->data;
	const struct sx9500_config *cfg = dev->config;
	int ret;

#ifdef CONFIG_SX9500_TRIGGER_OWN_THREAD
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);
#else
	data->work.handler = sx9500_work_cb;
#endif

	data->dev = dev;

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
			cfg->int_gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, sx9500_gpio_cb, BIT(cfg->int_gpio.pin));

	ret = gpio_add_callback(cfg->int_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_SX9500_TRIGGER_OWN_THREAD
	k_thread_create(&sx9500_thread, sx9500_thread_stack,
			CONFIG_SX9500_THREAD_STACK_SIZE,
			sx9500_thread_main, data, 0, NULL,
			K_PRIO_COOP(CONFIG_SX9500_THREAD_PRIORITY),
			0, K_NO_WAIT);
#endif

	return 0;
}
