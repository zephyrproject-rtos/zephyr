/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Würth Elektronic WSEN-ITDS 3-axis accel sensor driver
 *
 * Copyright (c) 2020 Linumiz
 * Author: Saravanan Sekar <saravanan@linumiz.com>
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "itds.h"
LOG_MODULE_DECLARE(ITDS, CONFIG_SENSOR_LOG_LEVEL);

static int itds_trigger_drdy_set(const struct device *dev,
				 enum sensor_channel chan,
				 sensor_trigger_handler_t handler)
{
	struct itds_device_data *ddata = dev->data;
	const struct itds_device_config *cfg = dev->config;
	uint8_t drdy_en = 0U;
	int ret;

	ddata->handler_drdy = handler;
	if (ddata->handler_drdy) {
		drdy_en = ITDS_MASK_INT_DRDY;
	}

	ret = i2c_reg_update_byte(ddata->i2c, cfg->i2c_addr, ITDS_REG_CTRL4,
				  ITDS_MASK_INT_DRDY, drdy_en);
	if (ret) {
		return ret;
	}

	return 0;
}

int itds_trigger_set(const struct device *dev,
		     const struct sensor_trigger *trig,
		     sensor_trigger_handler_t handler)
{
	if (trig->chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		return itds_trigger_drdy_set(dev, trig->chan, handler);

	default:
		return -ENOTSUP;
	}
}

static void itds_work_handler(struct k_work *work)
{
	struct itds_device_data *ddata =
			CONTAINER_OF(work, struct itds_device_data, work);
	const struct device *dev = (const struct device *)ddata->dev;
	const struct itds_device_config *cfg = dev->config;
	uint8_t status;
	struct sensor_trigger drdy_trigger = {
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	if (i2c_reg_read_byte(ddata->i2c, cfg->i2c_addr, ITDS_REG_STATUS,
			      &status) < 0) {
		return;
	}

	if (status & ITDS_EVENT_DRDY) {
		if (ddata->handler_drdy) {
			drdy_trigger.type = SENSOR_TRIG_DATA_READY;
			ddata->handler_drdy(dev, &drdy_trigger);
		}
	}
}

static void itds_gpio_callback(const struct device *port,
			       struct gpio_callback *cb, uint32_t pin)
{
	struct itds_device_data *ddata =
			CONTAINER_OF(cb, struct itds_device_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pin);

	k_work_submit(&ddata->work);
}

int itds_trigger_mode_init(const struct device *dev)
{
	struct itds_device_data *ddata = dev->data;
	const struct itds_device_config *cfg = dev->config;

	ddata->gpio = device_get_binding(cfg->gpio_port);
	if (!ddata->gpio) {
		LOG_DBG("Gpio controller %s not found.", cfg->gpio_port);
		return -EINVAL;
	}

	ddata->work.handler = itds_work_handler;
	ddata->dev = dev;

	gpio_pin_configure(ddata->gpio, cfg->int_pin,
			   GPIO_INPUT | cfg->int_flags);

	gpio_init_callback(&ddata->gpio_cb, itds_gpio_callback,
			   BIT(cfg->int_pin));

	gpio_add_callback(ddata->gpio, &ddata->gpio_cb);
	gpio_pin_interrupt_configure(ddata->gpio, cfg->int_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);

	/* enable global interrupt */
	return i2c_reg_update_byte(ddata->i2c, cfg->i2c_addr, ITDS_REG_CTRL7,
				   ITDS_MASK_INT_EN, ITDS_MASK_INT_EN);
}
