/*
 * Copyright (c) 2020 arithmetics.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_fdc2x1x

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "fdc2x1x.h"

#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(FDC2X1X, CONFIG_SENSOR_LOG_LEVEL);

static void fdc2x1x_thread_cb(const struct device *dev)
{
	struct fdc2x1x_data *drv_data = dev->data;
	uint16_t status;

#ifdef CONFIG_PM_DEVICE
	enum pm_device_state state;

	/* INTB asserts after exiting shutdown mode. Drop this interrupt */
	(void)pm_device_state_get(dev, &state);
	if (state == PM_DEVICE_STATE_OFF) {
		return;
	}
#endif

	/* Clear the status */
	if (fdc2x1x_get_status(dev, &status) < 0) {
		LOG_ERR("Unable to get status.");
		return;
	}

	k_mutex_lock(&drv_data->trigger_mutex, K_FOREVER);
	if ((drv_data->drdy_handler != NULL) && FDC2X1X_STATUS_DRDY(status)) {
		drv_data->drdy_handler(dev, drv_data->drdy_trigger);
	}
	k_mutex_unlock(&drv_data->trigger_mutex);
}

static void fdc2x1x_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct fdc2x1x_data *drv_data =
		CONTAINER_OF(cb, struct fdc2x1x_data, gpio_cb);

#ifdef CONFIG_FDC2X1X_TRIGGER_OWN_THREAD
	k_sem_give(&drv_data->gpio_sem);
#elif CONFIG_FDC2X1X_TRIGGER_GLOBAL_THREAD
	k_work_submit(&drv_data->work);
#endif
}

#ifdef CONFIG_FDC2X1X_TRIGGER_OWN_THREAD
static void fdc2x1x_thread(struct fdc2x1x_data *drv_data)
{
	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		fdc2x1x_thread_cb(drv_data->dev);
	}
}

#elif CONFIG_FDC2X1X_TRIGGER_GLOBAL_THREAD
static void fdc2x1x_work_cb(struct k_work *work)
{
	struct fdc2x1x_data *drv_data =
		CONTAINER_OF(work, struct fdc2x1x_data, work);

	fdc2x1x_thread_cb(drv_data->dev);
}
#endif

int fdc2x1x_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct fdc2x1x_data *drv_data = dev->data;
	const struct fdc2x1x_config *cfg = dev->config;
	uint16_t status, int_mask, int_en;
	int ret;

	gpio_pin_interrupt_configure_dt(&cfg->intb_gpio, GPIO_INT_DISABLE);

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		k_mutex_lock(&drv_data->trigger_mutex, K_FOREVER);
		drv_data->drdy_handler = handler;
		drv_data->drdy_trigger = trig;
		k_mutex_unlock(&drv_data->trigger_mutex);

		int_mask = FDC2X1X_ERROR_CONFIG_DRDY_2INT_MSK;
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		ret = -ENOTSUP;
		goto out;
	}

	if (handler) {
		int_en = int_mask;
		drv_data->int_config |= int_mask;
	} else {
		int_en = 0U;
	}

	ret = fdc2x1x_reg_write_mask(dev,
				     FDC2X1X_ERROR_CONFIG, int_mask, int_en);

	/* Clear INTB pin by reading STATUS register */
	fdc2x1x_get_status(dev, &status);
out:
	gpio_pin_interrupt_configure_dt(&cfg->intb_gpio, GPIO_INT_EDGE_TO_ACTIVE);

	return ret;
}

int fdc2x1x_init_interrupt(const struct device *dev)
{
	struct fdc2x1x_data *drv_data = dev->data;
	const struct fdc2x1x_config *cfg = dev->config;
	int ret;

	k_mutex_init(&drv_data->trigger_mutex);

	if (!device_is_ready(cfg->intb_gpio.port)) {
		LOG_ERR("%s: intb_gpio device not ready", cfg->intb_gpio.port->name);
		return -ENODEV;
	}

	ret = fdc2x1x_set_interrupt_pin(dev, true);
	if (ret) {
		return ret;
	}

	gpio_pin_configure_dt(&cfg->intb_gpio, GPIO_INPUT);

	gpio_init_callback(&drv_data->gpio_cb,
			   fdc2x1x_gpio_callback,
			   BIT(cfg->intb_gpio.pin));

	if (gpio_add_callback(cfg->intb_gpio.port, &drv_data->gpio_cb) < 0) {
		LOG_ERR("Failed to set gpio callback!");
		return -EIO;
	}

	drv_data->dev = dev;

#ifdef CONFIG_FDC2X1X_TRIGGER_OWN_THREAD
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_FDC2X1X_THREAD_STACK_SIZE,
			(k_thread_entry_t)fdc2x1x_thread,
			drv_data, 0, NULL,
			K_PRIO_COOP(CONFIG_FDC2X1X_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif CONFIG_FDC2X1X_TRIGGER_GLOBAL_THREAD
	drv_data->work.handler = fdc2x1x_work_cb;
#endif

	return 0;
}
