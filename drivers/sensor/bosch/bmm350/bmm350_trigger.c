/*
 * Copyright (c) 2024 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BMM350s accessed via I2C.
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

#include "bmm350.h"

LOG_MODULE_DECLARE(BMM350, CONFIG_SENSOR_LOG_LEVEL);

static void bmm350_handle_interrupts(const void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct bmm350_data *data = dev->data;

	if (data->drdy_handler) {
		data->drdy_handler(dev, data->drdy_trigger);
	}
}

#ifdef CONFIG_BMM350_TRIGGER_OWN_THREAD
static K_THREAD_STACK_DEFINE(bmm350_thread_stack, CONFIG_BMM350_THREAD_STACK_SIZE);
static struct k_thread bmm350_thread;

static void bmm350_thread_main(void *arg1, void *unused1, void *unused2)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	const struct device *dev = (const struct device *)arg1;
	struct bmm350_data *data = dev->data;

	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		bmm350_handle_interrupts(dev);
	}
}
#endif

#ifdef CONFIG_BMM350_TRIGGER_GLOBAL_THREAD
static void bmm350_work_handler(struct k_work *work)
{
	struct bmm350_data *data = CONTAINER_OF(work, struct bmm350_data, work);

	bmm350_handle_interrupts(data->dev);
}
#endif

static void bmm350_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pin)
{
	struct bmm350_data *data = CONTAINER_OF(cb, struct bmm350_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pin);

#if defined(CONFIG_BMM350_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#elif defined(CONFIG_BMM350_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

int bmm350_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct bmm350_data *data = dev->data;
	int ret = 0;

#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		return -EBUSY;
	}
#endif

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	data->drdy_trigger = trig;
	data->drdy_handler = handler;

	/* Set PMU command configuration */
	ret = bmm350_reg_write(dev, BMM350_REG_INT_CTRL, BMM350_DATA_READY_INT_CTRL);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

int bmm350_trigger_mode_init(const struct device *dev)
{
	struct bmm350_data *data = dev->data;
	const struct bmm350_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->drdy_int.port)) {
		LOG_ERR("INT device is not ready");
		return -ENODEV;
	}

#if defined(CONFIG_BMM350_TRIGGER_OWN_THREAD)
	k_sem_init(&data->sem, 0, 1);
	k_thread_create(&bmm350_thread, bmm350_thread_stack, CONFIG_BMM350_THREAD_STACK_SIZE,
			bmm350_thread_main, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_BMM350_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_BMM350_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->work, bmm350_work_handler);
#endif

#if defined(CONFIG_BMM350_TRIGGER_GLOBAL_THREAD)
	data->dev = dev;
#endif

	ret = gpio_pin_configure_dt(&cfg->drdy_int, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, bmm350_gpio_callback, BIT(cfg->drdy_int.pin));

	ret = gpio_add_callback(cfg->drdy_int.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->drdy_int, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
