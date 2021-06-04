/* Bosch BMP388 pressure sensor
 *
 * Copyright (c) 2020 Facebook, Inc. and its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp388-ds001.pdf
 */

#define DT_DRV_COMPAT bosch_bmp388

#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include "bmp388.h"

LOG_MODULE_DECLARE(BMP388, CONFIG_SENSOR_LOG_LEVEL);

static void bmp388_handle_interrupts(const void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct bmp388_data *data = DEV_DATA(dev);

	struct sensor_trigger drdy_trigger = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_PRESS,
	};

	if (data->handler_drdy) {
		data->handler_drdy(dev, &drdy_trigger);
	}
}

#ifdef CONFIG_BMP388_TRIGGER_OWN_THREAD
static K_THREAD_STACK_DEFINE(bmp388_thread_stack,
			     CONFIG_BMP388_THREAD_STACK_SIZE);
static struct k_thread bmp388_thread;

static void bmp388_thread_main(void *arg1, void *unused1, void *unused2)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	const struct device *dev = (const struct device *)arg1;
	struct bmp388_data *data = DEV_DATA(dev);

	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		bmp388_handle_interrupts(dev);
	}
}
#endif

#ifdef CONFIG_BMP388_TRIGGER_GLOBAL_THREAD
static void bmp388_work_handler(struct k_work *work)
{
	struct bmp388_data *data = CONTAINER_OF(work,
						struct bmp388_data,
						work);

	bmp388_handle_interrupts(data->dev);
}
#endif

static void bmp388_gpio_callback(const struct device *port,
				 struct gpio_callback *cb,
				 uint32_t pin)
{
	struct bmp388_data *data = CONTAINER_OF(cb,
						struct bmp388_data,
						gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pin);

#if defined(CONFIG_BMP388_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#elif defined(CONFIG_BMP388_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#elif defined(CONFIG_BMP388_TRIGGER_DIRECT)
	bmp388_handle_interrupts(data->dev);
#endif
}

int bmp388_trigger_set(
	const struct device *dev,
	const struct sensor_trigger *trig,
	sensor_trigger_handler_t handler)
{
	struct bmp388_data *data = DEV_DATA(dev);

#ifdef CONFIG_PM_DEVICE
	if (data->device_power_state != PM_DEVICE_STATE_ACTIVE) {
		return -EBUSY;
	}
#endif

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	if (bmp388_reg_field_update(
		    dev,
		    BMP388_REG_INT_CTRL,
		    BMP388_INT_CTRL_DRDY_EN_MASK,
		    (handler != NULL) << BMP388_INT_CTRL_DRDY_EN_POS) < 0) {
		LOG_ERR("Failed to enable DRDY interrupt");
		return -EIO;
	}

	data->handler_drdy = handler;

	return 0;
}

int bmp388_trigger_mode_init(const struct device *dev)
{
	struct bmp388_data *data = DEV_DATA(dev);
	const struct bmp388_config *cfg = DEV_CFG(dev);

	if (!device_is_ready(cfg->gpio_int.port)) {
		LOG_ERR("INT device is not ready");
		return -ENODEV;
	}

#if defined(CONFIG_BMP388_TRIGGER_OWN_THREAD)
	k_sem_init(&data->sem, 0, 1);
	k_thread_create(
		&bmp388_thread,
		bmp388_thread_stack,
		CONFIG_BMP388_THREAD_STACK_SIZE,
		bmp388_thread_main,
		(void *)dev,
		NULL,
		NULL,
		K_PRIO_COOP(CONFIG_BMP388_THREAD_PRIORITY),
		0,
		K_NO_WAIT);
#elif defined(CONFIG_BMP388_TRIGGER_GLOBAL_THREAD)
	data->work.handler = bmp388_work_handler;
#endif

#if defined(CONFIG_BMP388_TRIGGER_GLOBAL_THREAD) || \
	defined(CONFIG_BMP388_TRIGGER_DIRECT)
	data->dev = dev;
#endif

	gpio_pin_configure(cfg->gpio_int.port,
			   cfg->gpio_int.pin,
			   GPIO_INPUT | cfg->gpio_int.dt_flags);

	gpio_init_callback(&data->gpio_cb,
			   bmp388_gpio_callback,
			   BIT(cfg->gpio_int.pin));
	gpio_add_callback(cfg->gpio_int.port, &data->gpio_cb);
	gpio_pin_interrupt_configure(cfg->gpio_int.port,
				     cfg->gpio_int.pin,
				     GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}
