/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <gpio.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>
#include "adxl372.h"

static void adxl372_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct adxl372_data *drv_data = dev->driver_data;
	const struct adxl372_dev_config *cfg = dev->config->config_info;
	u8_t status1, status2;

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

	gpio_pin_enable_callback(drv_data->gpio, cfg->int_gpio);
}

static void adxl372_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, u32_t pins)
{
	struct adxl372_data *drv_data =
		CONTAINER_OF(cb, struct adxl372_data, gpio_cb);
	const struct adxl372_dev_config *cfg = dev->config->config_info;

	gpio_pin_disable_callback(dev, cfg->int_gpio);

#if defined(CONFIG_ADXL372_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADXL372_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

#if defined(CONFIG_ADXL372_TRIGGER_OWN_THREAD)
static void adxl372_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct adxl372_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		adxl372_thread_cb(dev);
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

int adxl372_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct adxl372_data *drv_data = dev->driver_data;
	const struct adxl372_dev_config *cfg = dev->config->config_info;
	u8_t int_mask, int_en, status1, status2;
	int ret;

	gpio_pin_disable_callback(drv_data->gpio, cfg->int_gpio);

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
		SYS_LOG_ERR("Unsupported sensor trigger");
		ret = -ENOTSUP;
		goto out;
	}

	if (handler) {
		int_en = int_mask;
	} else {
		int_en = 0;
	};

	ret = adxl372_reg_write_mask(dev, ADXL372_INT1_MAP, int_mask, int_en);

	adxl372_get_status(dev, &status1, &status2, NULL); /* Clear status */
out:
	gpio_pin_enable_callback(drv_data->gpio, cfg->int_gpio);

	return ret;
}

int adxl372_init_interrupt(struct device *dev)
{
	struct adxl372_data *drv_data = dev->driver_data;
	const struct adxl372_dev_config *cfg = dev->config->config_info;

	drv_data->gpio = device_get_binding(cfg->gpio_port);
	if (drv_data->gpio == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device!",
		    cfg->gpio_port);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, cfg->int_gpio,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb,
			   adxl372_gpio_callback,
			   BIT(cfg->int_gpio));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		SYS_LOG_ERR("Failed to set gpio callback!");
		return -EIO;
	}

#if defined(CONFIG_ADXL372_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ADXL372_THREAD_STACK_SIZE,
			(k_thread_entry_t)adxl372_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_ADXL372_THREAD_PRIORITY),
			0, 0);
#elif defined(CONFIG_ADXL372_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = adxl372_work_cb;
	drv_data->dev = dev;
#endif

	return 0;
}
