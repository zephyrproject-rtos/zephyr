/*
 * Copyright (c) 2023 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl367

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "adxl367.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ADXL367, CONFIG_SENSOR_LOG_LEVEL);

static void adxl367_thread_cb(const struct device *dev)
{
	const struct adxl367_dev_config *cfg = dev->config;
	struct adxl367_data *drv_data = dev->data;
	uint8_t status;
	int ret;

	/* Clear the status */
	ret = drv_data->hw_tf->read_reg(dev, ADXL367_STATUS, &status);
	if (ret != 0) {
		return;
	}

	if (drv_data->th_handler != NULL) {
		if (((FIELD_GET(ADXL367_STATUS_INACT, status)) != 0) ||
			(FIELD_GET(ADXL367_STATUS_ACT, status)) != 0) {
			drv_data->th_handler(dev, drv_data->th_trigger);
		}
	}

	if ((drv_data->drdy_handler != NULL) &&
		(FIELD_GET(ADXL367_STATUS_DATA_RDY, status) != 0)) {
		drv_data->drdy_handler(dev, drv_data->drdy_trigger);
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt,
					      GPIO_INT_EDGE_TO_ACTIVE);
	__ASSERT(ret == 0, "Interrupt configuration failed");
}

static void adxl367_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct adxl367_data *drv_data =
		CONTAINER_OF(cb, struct adxl367_data, gpio_cb);
	const struct adxl367_dev_config *cfg = drv_data->dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_DISABLE);

#if defined(CONFIG_ADXL367_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADXL367_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

#if defined(CONFIG_ADXL367_TRIGGER_OWN_THREAD)
static void adxl367_thread(struct adxl367_data *drv_data)
{
	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		adxl367_thread_cb(drv_data->dev);
	}
}

#elif defined(CONFIG_ADXL367_TRIGGER_GLOBAL_THREAD)
static void adxl367_work_cb(struct k_work *work)
{
	struct adxl367_data *drv_data =
		CONTAINER_OF(work, struct adxl367_data, work);

	adxl367_thread_cb(drv_data->dev);
}
#endif

int adxl367_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct adxl367_dev_config *cfg = dev->config;
	struct adxl367_data *drv_data = dev->data;
	uint8_t int_mask, int_en, status;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt,
					      GPIO_INT_DISABLE);
	if (ret != 0) {
		return ret;
	}

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		drv_data->th_handler = handler;
		drv_data->th_trigger = trig;
		int_mask = ADXL367_ACT_INT | ADXL367_INACT_INT;
		break;
	case SENSOR_TRIG_DATA_READY:
		drv_data->drdy_handler = handler;
		drv_data->drdy_trigger = trig;
		int_mask = ADXL367_DATA_RDY;
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	if (handler != NULL) {
		int_en = int_mask;
	} else {
		int_en = 0U;
	}

	ret = drv_data->hw_tf->write_reg_mask(dev, ADXL367_INTMAP1_LOWER, int_mask, int_en);
	if (ret != 0) {
		return ret;
	}

	/* Clear status */
	ret = drv_data->hw_tf->read_reg(dev, ADXL367_STATUS, &status);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int adxl367_init_interrupt(const struct device *dev)
{
	const struct adxl367_dev_config *cfg = dev->config;
	struct adxl367_data *drv_data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&cfg->interrupt)) {
		LOG_ERR("GPIO port %s not ready", cfg->interrupt.port->name);
		return -EINVAL;
	}

	ret = gpio_pin_configure_dt(&cfg->interrupt, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&drv_data->gpio_cb,
			   adxl367_gpio_callback,
			   BIT(cfg->interrupt.pin));

	ret = gpio_add_callback(cfg->interrupt.port, &drv_data->gpio_cb);
	if (ret != 0) {
		LOG_ERR("Failed to set gpio callback!");
		return ret;
	}

	drv_data->dev = dev;

#if defined(CONFIG_ADXL367_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ADXL367_THREAD_STACK_SIZE,
			(k_thread_entry_t)adxl367_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_ADXL367_THREAD_PRIORITY),
			0, K_NO_WAIT);

	k_thread_name_set(&drv_data->thread, dev->name);
#elif defined(CONFIG_ADXL367_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = adxl367_work_cb;
#endif

	return 0;
}
