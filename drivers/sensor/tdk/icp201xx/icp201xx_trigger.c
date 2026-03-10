/*
 * Copyright (c) 2025 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include "icp201xx_drv.h"
#include "Icp201xxDriver.h"

LOG_MODULE_DECLARE(ICP201XX, CONFIG_SENSOR_LOG_LEVEL);

static void icp201xx_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct icp201xx_data *drv_data = CONTAINER_OF(cb, struct icp201xx_data, gpio_cb);
	const struct icp201xx_config *cfg = (const struct icp201xx_config *)drv_data->dev->config;

	ARG_UNUSED(pins);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

#if defined(CONFIG_ICP201XX_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ICP201XX_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void icp201xx_thread_cb(const struct device *dev)
{
	struct icp201xx_data *drv_data = dev->data;
	const struct icp201xx_config *cfg = dev->config;
	uint8_t i_status;

	icp201xx_mutex_lock(dev);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

	inv_icp201xx_get_int_status(&drv_data->icp_device, &i_status);
	if (i_status) {
		if ((i_status & ICP201XX_INT_STATUS_FIFO_WMK_HIGH) &&
		    (drv_data->drdy_handler != NULL)) {
			drv_data->drdy_handler(dev, drv_data->drdy_trigger);
		}
		if ((i_status & ICP201XX_INT_STATUS_PRESS_DELTA) &&
		    (drv_data->delta_handler != NULL)) {
			drv_data->delta_handler(dev, drv_data->delta_trigger);
		}
		if ((i_status & ICP201XX_INT_STATUS_PRESS_ABS) &&
		    (drv_data->threshold_handler != NULL)) {
			drv_data->threshold_handler(dev, drv_data->threshold_trigger);
		}
		inv_icp201xx_clear_int_status(&drv_data->icp_device, i_status);
	}
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_LEVEL_LOW);
	icp201xx_mutex_unlock(dev);
}

#if defined(CONFIG_ICP201XX_TRIGGER_OWN_THREAD)
static void icp201xx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	struct device *dev = (struct device *)p1;
	struct icp201xx_data *drv_data = (struct icp201xx_data *)dev->data;

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		icp201xx_thread_cb(dev);
	}
}
#elif defined(CONFIG_ICP201XX_TRIGGER_GLOBAL_THREAD)
static void icp201xx_work_handler(struct k_work *work)
{
	struct icp201xx_data *data = CONTAINER_OF(work, struct icp201xx_data, work);

	icp201xx_thread_cb(data->dev);
}
#endif

static int icp201xx_fifo_interrupt(const struct device *dev, uint8_t fifo_watermark)
{
	int rc = 0;
	struct icp201xx_data *data = (struct icp201xx_data *)dev->data;

	inv_icp201xx_flush_fifo(&(data->icp_device));

	rc |= inv_icp201xx_set_fifo_notification_config(
		&(data->icp_device), ICP201XX_INT_MASK_FIFO_WMK_HIGH, fifo_watermark, 0);

	inv_icp201xx_app_warmup(&(data->icp_device), data->op_mode, ICP201XX_MEAS_MODE_CONTINUOUS);
	return rc;
}

static int icp201xx_pressure_interrupt(const struct device *dev, struct sensor_value pressure)
{
	int16_t pressure_value, pressure_delta_value;
	uint8_t int_mask = 0;
	int rc = 0;
	struct icp201xx_data *data = (struct icp201xx_data *)dev->data;

	inv_icp201xx_get_press_notification_config(&(data->icp_device), &int_mask, &pressure_value,
						   &pressure_delta_value);
	/* PABS = (P(kPa)-70kPa)/40kPa*2^13 */
	pressure_value =
		(8192 * (pressure.val1 - 70) + ((8192 * (uint64_t)pressure.val2) / 1000000)) / 40;
	rc |= inv_icp201xx_set_press_notification_config(&(data->icp_device),
							 ~int_mask | ICP201XX_INT_MASK_PRESS_ABS,
							 pressure_value, pressure_delta_value);
	return rc;
}

static int icp201xx_pressure_change_interrupt(const struct device *dev,
					      struct sensor_value pressure_delta)
{
	int16_t pressure_value, pressure_delta_value;
	uint8_t int_mask = 0;
	int rc = 0;
	struct icp201xx_data *data = (struct icp201xx_data *)dev->data;

	inv_icp201xx_get_press_notification_config(&(data->icp_device), &int_mask, &pressure_value,
						   &pressure_delta_value);
	/* PDELTA = (P(kPa)/80)* 2^14 */
	pressure_delta_value =
		(16384 * pressure_delta.val1 + (16384 * (uint64_t)pressure_delta.val2 / 1000000)) /
		80;
	rc |= inv_icp201xx_set_press_notification_config(&(data->icp_device),
							 ~int_mask | ICP201XX_INT_MASK_PRESS_DELTA,
							 pressure_value, pressure_delta_value);
	return rc;
}

int icp201xx_trigger_init(const struct device *dev)
{
	struct icp201xx_data *drv_data = dev->data;
	const struct icp201xx_config *cfg = dev->config;
	int result = 0;

	if (!cfg->gpio_int.port) {
		LOG_ERR("trigger enabled but no interrupt gpio supplied");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->gpio_int)) {
		LOG_ERR("gpio_int gpio not ready");
		return -ENODEV;
	}

	drv_data->dev = dev;
	gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	gpio_init_callback(&drv_data->gpio_cb, icp201xx_gpio_callback, BIT(cfg->gpio_int.pin));
	result = gpio_add_callback(cfg->gpio_int.port, &drv_data->gpio_cb);

	if (result < 0) {
		LOG_ERR("Failed to set gpio callback");
		return result;
	}

	k_mutex_init(&drv_data->mutex);

#if defined(CONFIG_ICP201XX_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ICP201XX_THREAD_STACK_SIZE, icp201xx_thread, (struct device *)dev,
			NULL, NULL, K_PRIO_COOP(CONFIG_ICP201XX_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&drv_data->thread, "icp201xx");
#elif defined(CONFIG_ICP201XX_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = icp201xx_work_handler;
#endif

	return 0;
}

int icp201xx_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	int rc = 0;
	struct icp201xx_data *drv_data = dev->data;
	const struct icp201xx_config *cfg = dev->config;

	icp201xx_mutex_lock(dev);
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
	if (handler == NULL) {
		return -1;
	}

	if ((trig->type != SENSOR_TRIG_DATA_READY) && (trig->type != SENSOR_TRIG_DELTA) &&
	    (trig->type != SENSOR_TRIG_THRESHOLD)) {
		icp201xx_mutex_unlock(dev);
		return -ENOTSUP;
	}

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		rc = icp201xx_fifo_interrupt(dev, 1);
		drv_data->drdy_handler = handler;
		drv_data->drdy_trigger = trig;
	} else if (trig->type == SENSOR_TRIG_DELTA) {
		rc = icp201xx_pressure_change_interrupt(dev, drv_data->pressure_change);
		drv_data->delta_handler = handler;
		drv_data->delta_trigger = trig;
	} else if (trig->type == SENSOR_TRIG_THRESHOLD) {
		rc = icp201xx_pressure_interrupt(dev, drv_data->pressure_threshold);
		drv_data->threshold_handler = handler;
		drv_data->threshold_trigger = trig;
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_LEVEL_LOW);
	icp201xx_mutex_unlock(dev);

	return rc;
}
