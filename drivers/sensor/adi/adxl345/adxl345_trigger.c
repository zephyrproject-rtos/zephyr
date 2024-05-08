/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl345

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include "adxl345.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ADXL345, CONFIG_SENSOR_LOG_LEVEL);

static void adxl345_thread_cb(const struct device *dev)
{
	struct adxl345_dev_data *drv_data = dev->data;
	uint8_t status;
	int rc;

	// Read and clear the interrupt status
	rc = adxl345_reg_read_byte(drv_data->dev, ADXL345_INT_SOURCE_REG, &status);
	if (0 != rc) {
		return;
	}

	if (NULL != drv_data->drdy_handler) {
		if (FIELD_GET(ADXL345_INT_SOURCE_DATA_READY_BIT, status) != 0) {
			drv_data->drdy_handler(dev, drv_data->drdy_trigger);
		}
	}

	if (NULL != drv_data->active_handler) {
		if (FIELD_GET(ADXL345_INT_SOURCE_ACTIVE_BIT, status) != 0) {
			drv_data->active_handler(dev, drv_data->drdy_trigger);
		}
	}

	if (NULL != drv_data->inactive_handler) {
		if (FIELD_GET(ADXL345_INT_SOURCE_INACTIVE_BIT, status) != 0) {
			drv_data->inactive_handler(dev, drv_data->drdy_trigger);
		}
	}
}

static void adxl345_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct adxl345_dev_data *drv_data = CONTAINER_OF(cb, struct adxl345_dev_data, gpio_cb);

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
static void adxl345_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct adxl345_dev_data *drv_data = p1;

	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		adxl345_thread_cb(drv_data->dev);
	}
}
#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
static void adxl345_work_cb(struct k_work *work)
{
	struct adxl345_dev_data *drv_data = CONTAINER_OF(work, struct adxl345_dev_data, work);
	adxl345_thread_cb(drv_data->dev);
}
#endif

int adxl345_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct adxl345_dev_config *cfg = dev->config;
	struct adxl345_dev_data *drv_data = dev->data;
	uint8_t int_mask = 0, status = 0;
	int rc;

	if (!cfg->interrupt.port) {
		return -ENOTSUP;
	}

	if (NULL == handler) {
		return -EINVAL;
	}

	rc = gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_DISABLE);
	if (0 != rc) {
		return rc;
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY: {
		drv_data->drdy_handler = handler;
		drv_data->drdy_trigger = trig;
		int_mask = ADXL345_INT_ENABLE_DATA_READY_BIT;
		break;
	}
	case SENSOR_TRIG_MOTION: {
		drv_data->active_handler = handler;
		drv_data->active_trigger = trig;
		int_mask = ADXL345_INT_ENABLE_ACTIVE_BIT;
		adxl345_reg_write_mask(dev, ADXL345_ACT_INACT_CTL_REG,
				       ADXL345_ACT_INACT_CTL_ACT_AXIS_MSK,
				       ADXL345_ACT_INACT_CTL_ACT_AXIS_MSK);
		break;
	}
	case SENSOR_TRIG_STATIONARY: {
		drv_data->inactive_handler = handler;
		drv_data->inactive_trigger = trig;
		int_mask = ADXL345_INT_ENABLE_INACTIVE_BIT;
		adxl345_reg_write_mask(dev, ADXL345_ACT_INACT_CTL_REG,
				       ADXL345_ACT_INACT_CTL_INACT_AXIS_MSK,
				       ADXL345_ACT_INACT_CTL_INACT_AXIS_MSK);
		break;
	}
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	rc = gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
	if (0 != rc) {
		return rc;
	}

	rc = adxl345_reg_write_mask(dev, ADXL345_INT_ENABLE_REG, int_mask, int_mask);
	if (0 != rc) {
		return rc;
	}

	rc = adxl345_reg_read_byte(dev, ADXL345_INT_SOURCE_REG, &status);
	if (0 != rc) {
		return rc;
	}

	return rc;
}

int adxl345_init_interrupt(const struct device *dev)
{
	const struct adxl345_dev_config *cfg = dev->config;
	struct adxl345_dev_data *drv_data = dev->data;
	int rc;

	if (NULL == cfg->interrupt.port) {
		LOG_ERR("No GPIO port defined in devicetree file");
		return -ENOTSUP;
	}

	if (false == gpio_is_ready_dt(&cfg->interrupt)) {
		LOG_ERR("GPIO port %s not ready", cfg->interrupt.port->name);
		return -EINVAL;
	}

	rc = gpio_pin_configure_dt(&cfg->interrupt, GPIO_INPUT);
	if (0 != rc) {
		return rc;
	}

	gpio_init_callback(&drv_data->gpio_cb, adxl345_gpio_callback, BIT(cfg->interrupt.pin));

	rc = gpio_add_callback(cfg->interrupt.port, &drv_data->gpio_cb);
	if (0 != rc) {
		LOG_ERR("Failed to set gpio callback!");
		return rc;
	}

	// reset sensor registers
	rc = adxl345_reg_write_byte(dev, ADXL345_INT_ENABLE_REG, 0);

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
	// TODO: validate return code of thread creation
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack, CONFIG_ADXL345_THREAD_STACK_SIZE,
			(k_thread_entry_t)adxl345_thread, drv_data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ADXL345_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = adxl345_work_cb;
#endif

	drv_data->dev = dev;
	return 0;
}
