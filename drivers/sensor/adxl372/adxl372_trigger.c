/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl372

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "adxl372.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ADXL372, CONFIG_SENSOR_LOG_LEVEL);

static void adxl372_thread_cb(const struct device *dev)
{
	const struct adxl372_dev_config *cfg = dev->config;
	struct adxl372_data *drv_data = dev->data;
	uint8_t status1, status2;
	int ret;

	/* Clear the status */
	if (adxl372_get_status(dev, &status1, &status2, NULL) < 0) {
		return;
	}

	if (drv_data->th_handler != NULL) {
		/* In max peak mode we wait until we settle below the inactivity
		 * threshold and then call the trigger handler.
		 */
		if (cfg->max_peak_detect_mode &&
			ADXL372_STATUS_2_INACT(status2)) {
			drv_data->th_handler(dev, &drv_data->th_trigger);
		} else if (!cfg->max_peak_detect_mode &&
			(ADXL372_STATUS_2_INACT(status2) ||
			ADXL372_STATUS_2_ACTIVITY(status2))) {
			drv_data->th_handler(dev, &drv_data->th_trigger);
		}
	}

	if ((drv_data->drdy_handler != NULL) &&
		ADXL372_STATUS_1_DATA_RDY(status1)) {
		drv_data->drdy_handler(dev, &drv_data->drdy_trigger);
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt,
					      GPIO_INT_EDGE_TO_ACTIVE);
	__ASSERT(ret == 0, "Interrupt configuration failed");
}

static void adxl372_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct adxl372_data *drv_data =
		CONTAINER_OF(cb, struct adxl372_data, gpio_cb);
	const struct adxl372_dev_config *cfg = drv_data->dev->config;

	gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_DISABLE);

#if defined(CONFIG_ADXL372_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADXL372_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

#if defined(CONFIG_ADXL372_TRIGGER_OWN_THREAD)
static void adxl372_thread(struct adxl372_data *drv_data)
{
	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		adxl372_thread_cb(drv_data->dev);
	}
}

#elif defined(CONFIG_ADXL372_TRIGGER_GLOBAL_THREAD)
static void adxl372_work_cb(struct k_work *work)
{
	struct adxl372_data *drv_data =
		CONTAINER_OF(work, struct adxl372_data, work);

	adxl372_thread_cb(drv_data->dev);
}
#endif

int adxl372_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct adxl372_dev_config *cfg = dev->config;
	struct adxl372_data *drv_data = dev->data;
	uint8_t int_mask, int_en, status1, status2;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt,
					      GPIO_INT_DISABLE);
	if (ret < 0) {
		return ret;
	}

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		drv_data->th_handler = handler;
		drv_data->th_trigger = *trig;
		int_mask = ADXL372_INT1_MAP_ACT_MSK |
			   ADXL372_INT1_MAP_INACT_MSK;
		break;
	case SENSOR_TRIG_DATA_READY:
		drv_data->drdy_handler = handler;
		drv_data->drdy_trigger = *trig;
		int_mask = ADXL372_INT1_MAP_DATA_RDY_MSK;
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	if (handler) {
		int_en = int_mask;
	} else {
		int_en = 0U;
	}

	ret = drv_data->hw_tf->write_reg_mask(dev, ADXL372_INT1_MAP, int_mask, int_en);
	if (ret < 0) {
		return ret;
	}

	ret = adxl372_get_status(dev, &status1, &status2, NULL); /* Clear status */
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int adxl372_init_interrupt(const struct device *dev)
{
	const struct adxl372_dev_config *cfg = dev->config;
	struct adxl372_data *drv_data = dev->data;
	int ret;

	if (!device_is_ready(cfg->interrupt.port)) {
		LOG_ERR("GPIO port %s not ready", cfg->interrupt.port->name);
		return -EINVAL;
	}

	ret = gpio_pin_configure_dt(&cfg->interrupt, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&drv_data->gpio_cb,
			   adxl372_gpio_callback,
			   BIT(cfg->interrupt.pin));

	ret = gpio_add_callback(cfg->interrupt.port, &drv_data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set gpio callback!");
		return ret;
	}

	drv_data->dev = dev;

#if defined(CONFIG_ADXL372_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ADXL372_THREAD_STACK_SIZE,
			(k_thread_entry_t)adxl372_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_ADXL372_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_ADXL372_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = adxl372_work_cb;
#endif

	return 0;
}
