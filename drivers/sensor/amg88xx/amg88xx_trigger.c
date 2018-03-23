/*
 * Copyright (c) 2017 Phytec Messtechnik GmbH
 * Copyright (c) 2017 Benedict Ohl (Benedict-Ohl@web.de)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <gpio.h>
#include <i2c.h>
#include <misc/util.h>
#include <kernel.h>
#include <sensor.h>
#include "amg88xx.h"

extern struct amg88xx_data amg88xx_driver;

int amg88xx_attr_set(struct device *dev,
		     enum sensor_channel chan,
		     enum sensor_attribute attr,
		     const struct sensor_value *val)
{
	struct amg88xx_data *drv_data = dev->driver_data;
	s16_t int_level = (val->val1 * 1000000 + val->val2) /
			  AMG88XX_TREG_LSB_SCALING;
	u8_t intl_reg;
	u8_t inth_reg;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	SYS_LOG_DBG("set threshold to %d", int_level);

	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		intl_reg = AMG88XX_INTHL;
		inth_reg = AMG88XX_INTHH;
	} else if (attr == SENSOR_ATTR_LOWER_THRESH) {
		intl_reg = AMG88XX_INTLL;
		inth_reg = AMG88XX_INTLH;
	} else {
		return -ENOTSUP;
	}

	if (amg88xx_reg_write(drv_data, intl_reg, (u8_t)int_level)) {
		SYS_LOG_DBG("Failed to set INTxL attribute!");
		return -EIO;
	}

	if (amg88xx_reg_write(drv_data, inth_reg, (u8_t)(int_level >> 8))) {
		SYS_LOG_DBG("Failed to set INTxH attribute!");
		return -EIO;
	}

	return 0;
}

static void amg88xx_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, u32_t pins)
{
	struct amg88xx_data *drv_data =
		CONTAINER_OF(cb, struct amg88xx_data, gpio_cb);

	gpio_pin_disable_callback(dev, CONFIG_AMG88XX_GPIO_PIN_NUM);

#if defined(CONFIG_AMG88XX_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_AMG88XX_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void amg88xx_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct amg88xx_data *drv_data = dev->driver_data;
	u8_t status;

	if (amg88xx_reg_read(drv_data, AMG88XX_STAT, &status) < 0) {
		return;
	}

	if (drv_data->drdy_handler != NULL) {
		drv_data->drdy_handler(dev, &drv_data->drdy_trigger);
	}

	if (drv_data->th_handler != NULL) {
		drv_data->th_handler(dev, &drv_data->th_trigger);
	}

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_AMG88XX_GPIO_PIN_NUM);
}

#ifdef CONFIG_AMG88XX_TRIGGER_OWN_THREAD
static void amg88xx_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct amg88xx_data *drv_data = dev->driver_data;

	ARG_UNUSED(unused);

	while (42) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		amg88xx_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_AMG88XX_TRIGGER_GLOBAL_THREAD
static void amg88xx_work_cb(struct k_work *work)
{
	struct amg88xx_data *drv_data =
		CONTAINER_OF(work, struct amg88xx_data, work);
	amg88xx_thread_cb(drv_data->dev);
}
#endif

int amg88xx_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct amg88xx_data *drv_data = dev->driver_data;

	amg88xx_reg_write(drv_data, AMG88XX_INTC,
			  AMG88XX_INTC_DISABLED);
	gpio_pin_disable_callback(drv_data->gpio, CONFIG_AMG88XX_GPIO_PIN_NUM);

	if (trig->type == SENSOR_TRIG_THRESHOLD) {
		drv_data->th_handler = handler;
		drv_data->th_trigger = *trig;
	} else {
		SYS_LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_AMG88XX_GPIO_PIN_NUM);
	amg88xx_reg_write(drv_data, AMG88XX_INTC,
			  AMG88XX_INTC_ABS_MODE);
	return 0;
}

int amg88xx_init_interrupt(struct device *dev)
{
	struct amg88xx_data *drv_data = dev->driver_data;

	/* setup gpio interrupt */
	drv_data->gpio = device_get_binding(CONFIG_AMG88XX_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device!",
		    CONFIG_AMG88XX_GPIO_DEV_NAME);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, CONFIG_AMG88XX_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb,
			   amg88xx_gpio_callback,
			   BIT(CONFIG_AMG88XX_GPIO_PIN_NUM));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		SYS_LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

#if defined(CONFIG_AMG88XX_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_AMG88XX_THREAD_STACK_SIZE,
			(k_thread_entry_t)amg88xx_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_AMG88XX_THREAD_PRIORITY),
			0, 0);
#elif defined(CONFIG_AMG88XX_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = amg88xx_work_cb;
	drv_data->dev = dev;
#endif

	return 0;
}
