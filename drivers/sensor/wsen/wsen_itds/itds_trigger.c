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
				 const struct sensor_trigger *trig,
				 sensor_trigger_handler_t handler)
{
	struct itds_device_data *ddata = dev->data;
	const struct itds_device_config *cfg = dev->config;
	uint8_t drdy_en = 0U;
	int ret;

	ddata->handler_drdy = handler;
	ddata->trigger_drdy = trig;
	if (ddata->handler_drdy) {
		drdy_en = ITDS_MASK_INT_DRDY;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, ITDS_REG_CTRL4,
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
	const struct itds_device_config *cfg = dev->config;

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

	if (trig->chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -ENOTSUP;
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		return itds_trigger_drdy_set(dev, trig->chan, trig, handler);

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

	if (i2c_reg_read_byte_dt(&cfg->i2c, ITDS_REG_STATUS,
			      &status) < 0) {
		return;
	}

	if (status & ITDS_EVENT_DRDY) {
		if (ddata->handler_drdy) {
			ddata->handler_drdy(dev, ddata->trigger_drdy);
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

	/* dts doesn't have GPIO int pin set, so we dont support
	 * trigger mode for this instance
	 */
	if (!cfg->int_gpio.port) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
				cfg->int_gpio.port->name);
		return -ENODEV;
	}

	ddata->work.handler = itds_work_handler;
	ddata->dev = dev;

	gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);

	gpio_init_callback(&ddata->gpio_cb, itds_gpio_callback,
			   BIT(cfg->int_gpio.pin));

	gpio_add_callback(cfg->int_gpio.port, &ddata->gpio_cb);
	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);

	/* enable global interrupt */
	return i2c_reg_update_byte_dt(&cfg->i2c, ITDS_REG_CTRL7,
				   ITDS_MASK_INT_EN, ITDS_MASK_INT_EN);
}
