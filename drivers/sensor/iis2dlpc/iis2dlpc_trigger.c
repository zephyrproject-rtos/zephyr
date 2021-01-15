/* ST Microelectronics IIS2DLPC 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2dlpc.pdf
 */

#define DT_DRV_COMPAT st_iis2dlpc

#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include "iis2dlpc.h"

LOG_MODULE_DECLARE(IIS2DLPC, CONFIG_SENSOR_LOG_LEVEL);

/**
 * iis2dlpc_enable_int - enable selected int pin to generate interrupt
 */
static int iis2dlpc_enable_int(const struct device *dev,
			       enum sensor_trigger_type type, int enable)
{
	const struct iis2dlpc_dev_config *cfg = dev->config;
	struct iis2dlpc_data *iis2dlpc = dev->data;
	iis2dlpc_reg_t int_route;

	switch (type) {
	case SENSOR_TRIG_DATA_READY:
		if (cfg->drdy_int == 1) {
			/* set interrupt for pin INT1 */
			iis2dlpc_pin_int1_route_get(iis2dlpc->ctx,
					&int_route.ctrl4_int1_pad_ctrl);
			int_route.ctrl4_int1_pad_ctrl.int1_drdy = enable;

			return iis2dlpc_pin_int1_route_set(iis2dlpc->ctx,
					&int_route.ctrl4_int1_pad_ctrl);
		} else {
			/* set interrupt for pin INT2 */
			iis2dlpc_pin_int2_route_get(iis2dlpc->ctx,
					&int_route.ctrl5_int2_pad_ctrl);
			int_route.ctrl5_int2_pad_ctrl.int2_drdy = enable;

			return iis2dlpc_pin_int2_route_set(iis2dlpc->ctx,
					&int_route.ctrl5_int2_pad_ctrl);
		}
		break;
#ifdef CONFIG_IIS2DLPC_TAP
	case SENSOR_TRIG_TAP:
		/* set interrupt for pin INT1 */
		iis2dlpc_pin_int1_route_get(iis2dlpc->ctx,
				&int_route.ctrl4_int1_pad_ctrl);
		int_route.ctrl4_int1_pad_ctrl.int1_single_tap = enable;

		return iis2dlpc_pin_int1_route_set(iis2dlpc->ctx,
				&int_route.ctrl4_int1_pad_ctrl);
	case SENSOR_TRIG_DOUBLE_TAP:
		/* set interrupt for pin INT1 */
		iis2dlpc_pin_int1_route_get(iis2dlpc->ctx,
				&int_route.ctrl4_int1_pad_ctrl);
		int_route.ctrl4_int1_pad_ctrl.int1_tap = enable;

		return iis2dlpc_pin_int1_route_set(iis2dlpc->ctx,
				&int_route.ctrl4_int1_pad_ctrl);
#endif /* CONFIG_IIS2DLPC_TAP */
	default:
		LOG_ERR("Unsupported trigger interrupt route %d", type);
		return -ENOTSUP;
	}
}

/**
 * iis2dlpc_trigger_set - link external trigger to event data ready
 */
int iis2dlpc_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct iis2dlpc_data *iis2dlpc = dev->data;
	int16_t raw[3];
	int state = (handler != NULL) ? PROPERTY_ENABLE : PROPERTY_DISABLE;

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		iis2dlpc->drdy_handler = handler;
		if (state) {
			/* dummy read: re-trigger interrupt */
			iis2dlpc_acceleration_raw_get(iis2dlpc->ctx, raw);
		}
		return iis2dlpc_enable_int(dev, SENSOR_TRIG_DATA_READY, state);
#ifdef CONFIG_IIS2DLPC_TAP
	case SENSOR_TRIG_TAP:
		iis2dlpc->tap_handler = handler;
		return iis2dlpc_enable_int(dev, SENSOR_TRIG_TAP, state);
	case SENSOR_TRIG_DOUBLE_TAP:
		iis2dlpc->double_tap_handler = handler;
		return iis2dlpc_enable_int(dev, SENSOR_TRIG_DOUBLE_TAP, state);
#endif /* CONFIG_IIS2DLPC_TAP */
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}
}

static int iis2dlpc_handle_drdy_int(const struct device *dev)
{
	struct iis2dlpc_data *data = dev->data;

	struct sensor_trigger drdy_trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	if (data->drdy_handler) {
		data->drdy_handler(dev, &drdy_trig);
	}

	return 0;
}

#ifdef CONFIG_IIS2DLPC_TAP
static int iis2dlpc_handle_single_tap_int(const struct device *dev)
{
	struct iis2dlpc_data *data = dev->data;
	sensor_trigger_handler_t handler = data->tap_handler;

	struct sensor_trigger tap_trig = {
		.type = SENSOR_TRIG_TAP,
		.chan = SENSOR_CHAN_ALL,
	};

	if (handler) {
		handler(dev, &tap_trig);
	}

	return 0;
}

static int iis2dlpc_handle_double_tap_int(const struct device *dev)
{
	struct iis2dlpc_data *data = dev->data;
	sensor_trigger_handler_t handler = data->double_tap_handler;

	struct sensor_trigger tap_trig = {
		.type = SENSOR_TRIG_DOUBLE_TAP,
		.chan = SENSOR_CHAN_ALL,
	};

	if (handler) {
		handler(dev, &tap_trig);
	}

	return 0;
}
#endif /* CONFIG_IIS2DLPC_TAP */

/**
 * iis2dlpc_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void iis2dlpc_handle_interrupt(const struct device *dev)
{
	struct iis2dlpc_data *iis2dlpc = dev->data;
	const struct iis2dlpc_dev_config *cfg = dev->config;
	iis2dlpc_all_sources_t sources;

	iis2dlpc_all_sources_get(iis2dlpc->ctx, &sources);

	if (sources.status_dup.drdy) {
		iis2dlpc_handle_drdy_int(dev);
	}
#ifdef CONFIG_IIS2DLPC_TAP
	if (sources.status_dup.single_tap) {
		iis2dlpc_handle_single_tap_int(dev);
	}
	if (sources.status_dup.double_tap) {
		iis2dlpc_handle_double_tap_int(dev);
	}
#endif /* CONFIG_IIS2DLPC_TAP */

	gpio_pin_interrupt_configure(iis2dlpc->gpio, cfg->irq_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);
}

static void iis2dlpc_gpio_callback(const struct device *dev,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct iis2dlpc_data *iis2dlpc =
		CONTAINER_OF(cb, struct iis2dlpc_data, gpio_cb);

	if ((pins & BIT(iis2dlpc->gpio_pin)) == 0U) {
		return;
	}

	gpio_pin_interrupt_configure(dev, iis2dlpc->gpio_pin,
				     GPIO_INT_DISABLE);

#if defined(CONFIG_IIS2DLPC_TRIGGER_OWN_THREAD)
	k_sem_give(&iis2dlpc->gpio_sem);
#elif defined(CONFIG_IIS2DLPC_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&iis2dlpc->work);
#endif /* CONFIG_IIS2DLPC_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_IIS2DLPC_TRIGGER_OWN_THREAD
static void iis2dlpc_thread(struct iis2dlpc_data *iis2dlpc)
{
	while (1) {
		k_sem_take(&iis2dlpc->gpio_sem, K_FOREVER);
		iis2dlpc_handle_interrupt(iis2dlpc->dev);
	}
}
#endif /* CONFIG_IIS2DLPC_TRIGGER_OWN_THREAD */

#ifdef CONFIG_IIS2DLPC_TRIGGER_GLOBAL_THREAD
static void iis2dlpc_work_cb(struct k_work *work)
{
	struct iis2dlpc_data *iis2dlpc =
		CONTAINER_OF(work, struct iis2dlpc_data, work);

	iis2dlpc_handle_interrupt(iis2dlpc->dev);
}
#endif /* CONFIG_IIS2DLPC_TRIGGER_GLOBAL_THREAD */

int iis2dlpc_init_interrupt(const struct device *dev)
{
	struct iis2dlpc_data *iis2dlpc = dev->data;
	const struct iis2dlpc_dev_config *cfg = dev->config;
	int ret;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	iis2dlpc->gpio = device_get_binding(cfg->irq_dev_name);
	if (iis2dlpc->gpio == NULL) {
		LOG_DBG("Cannot get pointer to %s device",
			    cfg->irq_dev_name);
		return -EINVAL;
	}

	iis2dlpc->dev = dev;

#if defined(CONFIG_IIS2DLPC_TRIGGER_OWN_THREAD)
	k_sem_init(&iis2dlpc->gpio_sem, 0, UINT_MAX);

	k_thread_create(&iis2dlpc->thread, iis2dlpc->thread_stack,
		       CONFIG_IIS2DLPC_THREAD_STACK_SIZE,
		       (k_thread_entry_t)iis2dlpc_thread, iis2dlpc,
		       NULL, NULL, K_PRIO_COOP(CONFIG_IIS2DLPC_THREAD_PRIORITY),
		       0, K_NO_WAIT);
#elif defined(CONFIG_IIS2DLPC_TRIGGER_GLOBAL_THREAD)
	iis2dlpc->work.handler = iis2dlpc_work_cb;
#endif /* CONFIG_IIS2DLPC_TRIGGER_OWN_THREAD */

	iis2dlpc->gpio_pin = cfg->irq_pin;

	ret = gpio_pin_configure(iis2dlpc->gpio, cfg->irq_pin,
				 GPIO_INPUT | cfg->irq_flags);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&iis2dlpc->gpio_cb,
			   iis2dlpc_gpio_callback,
			   BIT(cfg->irq_pin));

	if (gpio_add_callback(iis2dlpc->gpio, &iis2dlpc->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* enable interrupt on int1/int2 in pulse mode */
	if (iis2dlpc_int_notification_set(iis2dlpc->ctx, IIS2DLPC_INT_PULSED)) {
		return -EIO;
	}

	return gpio_pin_interrupt_configure(iis2dlpc->gpio, cfg->irq_pin,
					    GPIO_INT_EDGE_TO_ACTIVE);
}
