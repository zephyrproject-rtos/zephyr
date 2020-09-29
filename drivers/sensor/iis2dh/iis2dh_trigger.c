/* ST Microelectronics IIS2DH 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2dh.pdf
 */

#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include "iis2dh.h"

LOG_MODULE_DECLARE(IIS2DH, CONFIG_SENSOR_LOG_LEVEL);

/**
 * iis2dh_enable_int - enable selected int pin to generate interrupt
 */
static int iis2dh_enable_drdy(const struct device *dev,
			      enum sensor_trigger_type type, int enable)
{
	struct iis2dh_data *iis2dh = dev->data;
	iis2dh_ctrl_reg3_t reg3;

	/* set interrupt for pin INT1 */
	iis2dh_pin_int1_config_get(iis2dh->ctx, &reg3);

	reg3.i1_drdy1 = enable;

	return iis2dh_pin_int1_config_set(iis2dh->ctx, &reg3);
}

/**
 * iis2dh_trigger_set - link external trigger to event data ready
 */
int iis2dh_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	struct iis2dh_data *iis2dh = dev->data;
	union axis3bit16_t raw;
	int state = (handler != NULL) ? PROPERTY_ENABLE : PROPERTY_DISABLE;

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		iis2dh->drdy_handler = handler;
		if (state) {
			/* dummy read: re-trigger interrupt */
			iis2dh_acceleration_raw_get(iis2dh->ctx, raw.u8bit);
		}
		return iis2dh_enable_drdy(dev, SENSOR_TRIG_DATA_READY, state);
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}
}

static int iis2dh_handle_drdy_int(const struct device *dev)
{
	struct iis2dh_data *data = dev->data;

	struct sensor_trigger drdy_trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	if (data->drdy_handler) {
		data->drdy_handler(dev, &drdy_trig);
	}

	return 0;
}

/**
 * iis2dh_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void iis2dh_handle_interrupt(const struct device *dev)
{
	struct iis2dh_data *iis2dh = dev->data;
	const struct iis2dh_device_config *cfg = dev->config;

	iis2dh_handle_drdy_int(dev);

	gpio_pin_interrupt_configure(iis2dh->gpio, cfg->int_gpio_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);
}

static void iis2dh_gpio_callback(const struct device *dev,
				 struct gpio_callback *cb, uint32_t pins)
{
	struct iis2dh_data *iis2dh =
		CONTAINER_OF(cb, struct iis2dh_data, gpio_cb);

	if ((pins & BIT(iis2dh->gpio_pin)) == 0U) {
		return;
	}

	gpio_pin_interrupt_configure(dev, iis2dh->gpio_pin,
				     GPIO_INT_DISABLE);

#if defined(CONFIG_IIS2DH_TRIGGER_OWN_THREAD)
	k_sem_give(&iis2dh->gpio_sem);
#elif defined(CONFIG_IIS2DH_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&iis2dh->work);
#endif /* CONFIG_IIS2DH_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_IIS2DH_TRIGGER_OWN_THREAD
static void iis2dh_thread(struct iis2dh_data *iis2dh)
{
	while (1) {
		k_sem_take(&iis2dh->gpio_sem, K_FOREVER);
		iis2dh_handle_interrupt(iis2dh->dev);
	}
}
#endif /* CONFIG_IIS2DH_TRIGGER_OWN_THREAD */

#ifdef CONFIG_IIS2DH_TRIGGER_GLOBAL_THREAD
static void iis2dh_work_cb(struct k_work *work)
{
	struct iis2dh_data *iis2dh =
		CONTAINER_OF(work, struct iis2dh_data, work);

	iis2dh_handle_interrupt(iis2dh->dev);
}
#endif /* CONFIG_IIS2DH_TRIGGER_GLOBAL_THREAD */

int iis2dh_init_interrupt(const struct device *dev)
{
	struct iis2dh_data *iis2dh = dev->data;
	const struct iis2dh_device_config *cfg = dev->config;
	int ret;

	/* setup data ready gpio interrupt */
	iis2dh->gpio = device_get_binding(cfg->int_gpio_port);
	if (iis2dh->gpio == NULL) {
		LOG_DBG("Cannot get pointer to %s device",
			    cfg->int_gpio_port);
		return -EINVAL;
	}

	iis2dh->dev = dev;

#if defined(CONFIG_IIS2DH_TRIGGER_OWN_THREAD)
	k_sem_init(&iis2dh->gpio_sem, 0, UINT_MAX);

	k_thread_create(&iis2dh->thread, iis2dh->thread_stack,
		       CONFIG_IIS2DH_THREAD_STACK_SIZE,
		       (k_thread_entry_t)iis2dh_thread, iis2dh,
		       0, NULL, K_PRIO_COOP(CONFIG_IIS2DH_THREAD_PRIORITY),
		       0, K_NO_WAIT);
#elif defined(CONFIG_IIS2DH_TRIGGER_GLOBAL_THREAD)
	iis2dh->work.handler = iis2dh_work_cb;
#endif /* CONFIG_IIS2DH_TRIGGER_OWN_THREAD */

	iis2dh->gpio_pin = cfg->int_gpio_pin;

	ret = gpio_pin_configure(iis2dh->gpio, cfg->int_gpio_pin,
				 GPIO_INPUT | cfg->int_gpio_flags);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&iis2dh->gpio_cb,
			   iis2dh_gpio_callback,
			   BIT(cfg->int_gpio_pin));

	if (gpio_add_callback(iis2dh->gpio, &iis2dh->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* enable drdy on int1 in pulse mode */
	if (iis2dh_int1_pin_notification_mode_set(iis2dh->ctx, IIS2DH_INT1_PULSED)) {
		return -EIO;
	}

	return gpio_pin_interrupt_configure(iis2dh->gpio, cfg->int_gpio_pin,
					    GPIO_INT_EDGE_TO_ACTIVE);
}
