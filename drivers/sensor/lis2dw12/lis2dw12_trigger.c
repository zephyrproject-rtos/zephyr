/* ST Microelectronics LIS2DW12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dw12.pdf
 */

#define DT_DRV_COMPAT st_lis2dw12

#include <kernel.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include "lis2dw12.h"

LOG_MODULE_DECLARE(LIS2DW12, CONFIG_SENSOR_LOG_LEVEL);

/**
 * lis2dw12_enable_int - enable selected int pin to generate interrupt
 */
static int lis2dw12_enable_int(const struct device *dev,
			       enum sensor_trigger_type type, int enable)
{
	const struct lis2dw12_device_config *cfg = dev->config;
	struct lis2dw12_data *lis2dw12 = dev->data;
	lis2dw12_reg_t int_route;

	if (cfg->int_pin == 1U) {
		/* set interrupt for pin INT1 */
		lis2dw12_pin_int1_route_get(lis2dw12->ctx,
				&int_route.ctrl4_int1_pad_ctrl);

		switch (type) {
		case SENSOR_TRIG_DATA_READY:
			int_route.ctrl4_int1_pad_ctrl.int1_drdy = enable;
			break;
#ifdef CONFIG_LIS2DW12_PULSE
		case SENSOR_TRIG_TAP:
			int_route.ctrl4_int1_pad_ctrl.int1_single_tap = enable;
			break;
		case SENSOR_TRIG_DOUBLE_TAP:
			int_route.ctrl4_int1_pad_ctrl.int1_tap = enable;
			break;
#endif /* CONFIG_LIS2DW12_PULSE */
		default:
			LOG_ERR("Unsupported trigger interrupt route");
			return -ENOTSUP;
		}

		return lis2dw12_pin_int1_route_set(lis2dw12->ctx,
				&int_route.ctrl4_int1_pad_ctrl);
	} else {
		/* set interrupt for pin INT2 */
		lis2dw12_pin_int2_route_get(lis2dw12->ctx,
					    &int_route.ctrl5_int2_pad_ctrl);

		switch (type) {
		case SENSOR_TRIG_DATA_READY:
			int_route.ctrl5_int2_pad_ctrl.int2_drdy = enable;
			break;
		default:
			LOG_ERR("Unsupported trigger interrupt route");
			return -ENOTSUP;
		}

		return lis2dw12_pin_int2_route_set(lis2dw12->ctx,
				&int_route.ctrl5_int2_pad_ctrl);
	}
}

/**
 * lis2dw12_trigger_set - link external trigger to event data ready
 */
int lis2dw12_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct lis2dw12_data *lis2dw12 = dev->data;
	union axis3bit16_t raw;
	int state = (handler != NULL) ? PROPERTY_ENABLE : PROPERTY_DISABLE;

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		lis2dw12->drdy_handler = handler;
		if (state) {
			/* dummy read: re-trigger interrupt */
			lis2dw12_acceleration_raw_get(lis2dw12->ctx, raw.u8bit);
		}
		return lis2dw12_enable_int(dev, SENSOR_TRIG_DATA_READY, state);
		break;
#ifdef CONFIG_LIS2DW12_PULSE
	case SENSOR_TRIG_TAP:
		lis2dw12->tap_handler = handler;
		return lis2dw12_enable_int(dev, SENSOR_TRIG_TAP, state);
		break;
	case SENSOR_TRIG_DOUBLE_TAP:
		lis2dw12->double_tap_handler = handler;
		return lis2dw12_enable_int(dev, SENSOR_TRIG_DOUBLE_TAP, state);
		break;
#endif /* CONFIG_LIS2DW12_PULSE */
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}
}

static int lis2dw12_handle_drdy_int(const struct device *dev)
{
	struct lis2dw12_data *data = dev->data;

	struct sensor_trigger drdy_trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	if (data->drdy_handler) {
		data->drdy_handler(dev, &drdy_trig);
	}

	return 0;
}

#ifdef CONFIG_LIS2DW12_PULSE
static int lis2dw12_handle_single_tap_int(const struct device *dev)
{
	struct lis2dw12_data *data = dev->data;
	sensor_trigger_handler_t handler = data->tap_handler;

	struct sensor_trigger pulse_trig = {
		.type = SENSOR_TRIG_TAP,
		.chan = SENSOR_CHAN_ALL,
	};

	if (handler) {
		handler(dev, &pulse_trig);
	}

	return 0;
}

static int lis2dw12_handle_double_tap_int(const struct device *dev)
{
	struct lis2dw12_data *data = dev->data;
	sensor_trigger_handler_t handler = data->double_tap_handler;

	struct sensor_trigger pulse_trig = {
		.type = SENSOR_TRIG_DOUBLE_TAP,
		.chan = SENSOR_CHAN_ALL,
	};

	if (handler) {
		handler(dev, &pulse_trig);
	}

	return 0;
}
#endif /* CONFIG_LIS2DW12_PULSE */

/**
 * lis2dw12_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lis2dw12_handle_interrupt(const struct device *dev)
{
	struct lis2dw12_data *lis2dw12 = dev->data;
	const struct lis2dw12_device_config *cfg = dev->config;
	lis2dw12_all_sources_t sources;

	lis2dw12_all_sources_get(lis2dw12->ctx, &sources);

	if (sources.status_dup.drdy) {
		lis2dw12_handle_drdy_int(dev);
	}
#ifdef CONFIG_LIS2DW12_PULSE
	if (sources.status_dup.single_tap) {
		lis2dw12_handle_single_tap_int(dev);
	}
	if (sources.status_dup.double_tap) {
		lis2dw12_handle_double_tap_int(dev);
	}
#endif /* CONFIG_LIS2DW12_PULSE */

	gpio_pin_interrupt_configure(lis2dw12->gpio, cfg->int_gpio_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);
}

static void lis2dw12_gpio_callback(const struct device *dev,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct lis2dw12_data *lis2dw12 =
		CONTAINER_OF(cb, struct lis2dw12_data, gpio_cb);

	if ((pins & BIT(lis2dw12->gpio_pin)) == 0U) {
		return;
	}

	gpio_pin_interrupt_configure(dev, lis2dw12->gpio_pin,
				     GPIO_INT_DISABLE);

#if defined(CONFIG_LIS2DW12_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2dw12->gpio_sem);
#elif defined(CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2dw12->work);
#endif /* CONFIG_LIS2DW12_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_LIS2DW12_TRIGGER_OWN_THREAD
static void lis2dw12_thread(struct lis2dw12_data *lis2dw12)
{
	while (1) {
		k_sem_take(&lis2dw12->gpio_sem, K_FOREVER);
		lis2dw12_handle_interrupt(lis2dw12->dev);
	}
}
#endif /* CONFIG_LIS2DW12_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD
static void lis2dw12_work_cb(struct k_work *work)
{
	struct lis2dw12_data *lis2dw12 =
		CONTAINER_OF(work, struct lis2dw12_data, work);

	lis2dw12_handle_interrupt(lis2dw12->dev);
}
#endif /* CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD */

int lis2dw12_init_interrupt(const struct device *dev)
{
	struct lis2dw12_data *lis2dw12 = dev->data;
	const struct lis2dw12_device_config *cfg = dev->config;
	int ret;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	lis2dw12->gpio = device_get_binding(cfg->int_gpio_port);
	if (lis2dw12->gpio == NULL) {
		LOG_DBG("Cannot get pointer to %s device",
			    cfg->int_gpio_port);
		return -EINVAL;
	}

	lis2dw12->dev = dev;

#if defined(CONFIG_LIS2DW12_TRIGGER_OWN_THREAD)
	k_sem_init(&lis2dw12->gpio_sem, 0, UINT_MAX);

	k_thread_create(&lis2dw12->thread, lis2dw12->thread_stack,
		       CONFIG_LIS2DW12_THREAD_STACK_SIZE,
		       (k_thread_entry_t)lis2dw12_thread, lis2dw12,
		       NULL, NULL, K_PRIO_COOP(CONFIG_LIS2DW12_THREAD_PRIORITY),
		       0, K_NO_WAIT);
#elif defined(CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD)
	lis2dw12->work.handler = lis2dw12_work_cb;
#endif /* CONFIG_LIS2DW12_TRIGGER_OWN_THREAD */

	lis2dw12->gpio_pin = cfg->int_gpio_pin;

	ret = gpio_pin_configure(lis2dw12->gpio, cfg->int_gpio_pin,
				 GPIO_INPUT | cfg->int_gpio_flags);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&lis2dw12->gpio_cb,
			   lis2dw12_gpio_callback,
			   BIT(cfg->int_gpio_pin));

	if (gpio_add_callback(lis2dw12->gpio, &lis2dw12->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* enable interrupt on int1/int2 in pulse mode */
	if (lis2dw12_int_notification_set(lis2dw12->ctx, LIS2DW12_INT_PULSED)) {
		return -EIO;
	}

	return gpio_pin_interrupt_configure(lis2dw12->gpio, cfg->int_gpio_pin,
					    GPIO_INT_EDGE_TO_ACTIVE);
}
