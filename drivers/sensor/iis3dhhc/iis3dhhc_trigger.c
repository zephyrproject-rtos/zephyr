/* ST Microelectronics IIS3DHHC accelerometer sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis3dhhc.pdf
 */

#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include "iis3dhhc.h"

LOG_MODULE_DECLARE(IIS3DHHC, CONFIG_SENSOR_LOG_LEVEL);

/**
 * iis3dhhc_enable_int - enable selected int pin to generate interrupt
 */
static int iis3dhhc_enable_int(struct device *dev, int enable)
{
	struct iis3dhhc_data *iis3dhhc = dev->driver_data;

	/* set interrupt */
#ifdef CONFIG_IIS3DHHC_DRDY_INT1
	return iis3dhhc_drdy_on_int1_set(iis3dhhc->ctx, enable);
#else
	return iis3dhhc_drdy_on_int2_set(iis3dhhc->ctx, enable);
#endif
}

/**
 * iis3dhhc_trigger_set - link external trigger to event data ready
 */
int iis3dhhc_trigger_set(struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct iis3dhhc_data *iis3dhhc = dev->driver_data;
	union axis3bit16_t raw;

	if (trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
		iis3dhhc->handler_drdy = handler;
		if (handler) {
			/* dummy read: re-trigger interrupt */
			iis3dhhc_acceleration_raw_get(iis3dhhc->ctx, raw.u8bit);
			return iis3dhhc_enable_int(dev, PROPERTY_ENABLE);
		} else {
			return iis3dhhc_enable_int(dev, PROPERTY_DISABLE);
		}
	}

	return -ENOTSUP;
}

/**
 * iis3dhhc_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void iis3dhhc_handle_interrupt(void *arg)
{
	struct device *dev = arg;
	struct iis3dhhc_data *iis3dhhc = dev->driver_data;
	struct sensor_trigger drdy_trigger = {
		.type = SENSOR_TRIG_DATA_READY,
	};
	const struct iis3dhhc_config *cfg = dev->config->config_info;

	if (iis3dhhc->handler_drdy != NULL) {
		iis3dhhc->handler_drdy(dev, &drdy_trigger);
	}

	gpio_pin_interrupt_configure(iis3dhhc->gpio, cfg->int_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);
}

static void iis3dhhc_gpio_callback(struct device *dev,
				    struct gpio_callback *cb, u32_t pins)
{
	struct iis3dhhc_data *iis3dhhc =
		CONTAINER_OF(cb, struct iis3dhhc_data, gpio_cb);
	const struct iis3dhhc_config *cfg = dev->config->config_info;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure(iis3dhhc->gpio, cfg->int_pin,
				     GPIO_INT_DISABLE);

#if defined(CONFIG_IIS3DHHC_TRIGGER_OWN_THREAD)
	k_sem_give(&iis3dhhc->gpio_sem);
#elif defined(CONFIG_IIS3DHHC_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&iis3dhhc->work);
#endif /* CONFIG_IIS3DHHC_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_IIS3DHHC_TRIGGER_OWN_THREAD
static void iis3dhhc_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct iis3dhhc_data *iis3dhhc = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&iis3dhhc->gpio_sem, K_FOREVER);
		iis3dhhc_handle_interrupt(dev);
	}
}
#endif /* CONFIG_IIS3DHHC_TRIGGER_OWN_THREAD */

#ifdef CONFIG_IIS3DHHC_TRIGGER_GLOBAL_THREAD
static void iis3dhhc_work_cb(struct k_work *work)
{
	struct iis3dhhc_data *iis3dhhc =
		CONTAINER_OF(work, struct iis3dhhc_data, work);

	iis3dhhc_handle_interrupt(iis3dhhc->dev);
}
#endif /* CONFIG_IIS3DHHC_TRIGGER_GLOBAL_THREAD */

int iis3dhhc_init_interrupt(struct device *dev)
{
	struct iis3dhhc_data *iis3dhhc = dev->driver_data;
	const struct iis3dhhc_config *cfg = dev->config->config_info;
	int ret;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	iis3dhhc->gpio = device_get_binding(cfg->int_port);
	if (iis3dhhc->gpio == NULL) {
		LOG_DBG("Cannot get pointer to %s device", cfg->int_port);
		return -EINVAL;
	}

#if defined(CONFIG_IIS3DHHC_TRIGGER_OWN_THREAD)
	k_sem_init(&iis3dhhc->gpio_sem, 0, UINT_MAX);

	k_thread_create(&iis3dhhc->thread, iis3dhhc->thread_stack,
		       CONFIG_IIS3DHHC_THREAD_STACK_SIZE,
		       (k_thread_entry_t)iis3dhhc_thread, dev,
		       0, NULL, K_PRIO_COOP(CONFIG_IIS3DHHC_THREAD_PRIORITY),
		       0, K_NO_WAIT);
#elif defined(CONFIG_IIS3DHHC_TRIGGER_GLOBAL_THREAD)
	iis3dhhc->work.handler = iis3dhhc_work_cb;
	iis3dhhc->dev = dev;
#endif /* CONFIG_IIS3DHHC_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure(iis3dhhc->gpio, cfg->int_pin,
				 GPIO_INPUT | cfg->int_flags);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&iis3dhhc->gpio_cb,
			   iis3dhhc_gpio_callback,
			   BIT(cfg->int_pin));

	if (gpio_add_callback(iis3dhhc->gpio, &iis3dhhc->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* enable interrupt on int1/int2 in pulse mode */
	if (iis3dhhc_drdy_notification_mode_set(iis3dhhc->ctx, IIS3DHHC_PULSED)) {
		return -EIO;
	}

	return gpio_pin_interrupt_configure(iis3dhhc->gpio, cfg->int_pin,
					    GPIO_INT_EDGE_TO_ACTIVE);
}
