/* Bosch BMM150 pressure sensor
 *
 * Copyright (c) 2020 Facebook, Inc. and its affiliates
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmm150-ds001.pdf
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>

#include "bmm150.h"

LOG_MODULE_DECLARE(BMM150, CONFIG_SENSOR_LOG_LEVEL);

static void bmm150_handle_interrupts(const void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct bmm150_data *data = dev->data;

	if (data->drdy_handler) {
		data->drdy_handler(dev, data->drdy_trigger);
	}
}

#ifdef CONFIG_BMM150_TRIGGER_OWN_THREAD
static K_THREAD_STACK_DEFINE(bmm150_thread_stack,
			     CONFIG_BMM150_THREAD_STACK_SIZE);
static struct k_thread bmm150_thread;

static void bmm150_thread_main(void *arg1, void *unused1, void *unused2)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	const struct device *dev = (const struct device *)arg1;
	struct bmm150_data *data = dev->data;

	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		bmm150_handle_interrupts(dev);
	}
}
#endif

#ifdef CONFIG_BMM150_TRIGGER_GLOBAL_THREAD
static void bmm150_work_handler(struct k_work *work)
{
	struct bmm150_data *data = CONTAINER_OF(work,
						struct bmm150_data,
						work);

	bmm150_handle_interrupts(data->dev);
}
#endif

static void bmm150_gpio_callback(const struct device *port,
				 struct gpio_callback *cb,
				 uint32_t pin)
{
	struct bmm150_data *data = CONTAINER_OF(cb,
						struct bmm150_data,
						gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pin);

#if defined(CONFIG_BMM150_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#elif defined(CONFIG_BMM150_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#elif defined(CONFIG_BMM150_TRIGGER_DIRECT)
	bmm150_handle_interrupts(data->dev);
#endif
}

int bmm150_trigger_set(
	const struct device *dev,
	const struct sensor_trigger *trig,
	sensor_trigger_handler_t handler)
{
	uint16_t values[BMM150_AXIS_XYZR_MAX];
	struct bmm150_data *data = dev->data;
	const struct bmm150_config *cfg = dev->config;

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

	if (bmm150_reg_update_byte(dev,
				   BMM150_REG_INT_DRDY,
				   BMM150_MASK_DRDY_EN,
				    (handler != NULL) << BMM150_SHIFT_DRDY_EN) < 0) {
		LOG_ERR("Failed to enable DRDY interrupt");
		return -EIO;
	}

	/* Clean data registers */
	if (cfg->bus_io->read(&cfg->bus, BMM150_REG_X_L, (uint8_t *)values, sizeof(values)) < 0) {
		LOG_ERR("failed to read sample");
		return -EIO;
	}

	return 0;
}

int bmm150_trigger_mode_init(const struct device *dev)
{
	struct bmm150_data *data = dev->data;
	const struct bmm150_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->drdy_int.port)) {
		LOG_ERR("INT device is not ready");
		return -ENODEV;
	}

#if defined(CONFIG_BMM150_TRIGGER_OWN_THREAD)
	k_sem_init(&data->sem, 0, 1);
	k_thread_create(
		&bmm150_thread,
		bmm150_thread_stack,
		CONFIG_BMM150_THREAD_STACK_SIZE,
		bmm150_thread_main,
		(void *)dev,
		NULL,
		NULL,
		K_PRIO_COOP(CONFIG_BMM150_THREAD_PRIORITY),
		0,
		K_NO_WAIT);
#elif defined(CONFIG_BMM150_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->work, bmm150_work_handler);
#endif

#if defined(CONFIG_BMM150_TRIGGER_GLOBAL_THREAD) || \
	defined(CONFIG_BMM150_TRIGGER_DIRECT)
	data->dev = dev;
#endif

	ret = gpio_pin_configure_dt(&cfg->drdy_int, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb,
			   bmm150_gpio_callback,
			   BIT(cfg->drdy_int.pin));

	ret = gpio_add_callback(cfg->drdy_int.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->drdy_int,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
