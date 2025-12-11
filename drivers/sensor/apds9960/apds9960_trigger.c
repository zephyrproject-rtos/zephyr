/*
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "apds9960.h"

extern struct apds9960_data apds9960_driver;

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(APDS9960, CONFIG_SENSOR_LOG_LEVEL);

void apds9960_work_cb(struct k_work *work)
{
	struct apds9960_data *data = CONTAINER_OF(work,
						  struct apds9960_data,
						  work);
	const struct device *dev = data->dev;

	if (data->p_th_handler != NULL) {
		data->p_th_handler(dev, data->p_th_trigger);
	}

	apds9960_setup_int(dev->config, true);
}

int apds9960_attr_set(const struct device *dev,
		      enum sensor_channel chan,
		      enum sensor_attribute attr,
		      const struct sensor_value *val)
{
	const struct apds9960_config *config = dev->config;

	if (chan == SENSOR_CHAN_PROX) {
		if (attr == SENSOR_ATTR_UPPER_THRESH) {
			if (i2c_reg_write_byte_dt(&config->i2c,
						  APDS9960_PIHT_REG,
						  (uint8_t)val->val1)) {
				return -EIO;
			}

			return 0;
		}
		if (attr == SENSOR_ATTR_LOWER_THRESH) {
			if (i2c_reg_write_byte_dt(&config->i2c,
						  APDS9960_PILT_REG,
						  (uint8_t)val->val1)) {
				return -EIO;
			}

			return 0;
		}
	}

	return -ENOTSUP;
}

int apds9960_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct apds9960_config *config = dev->config;
	struct apds9960_data *data = dev->data;

	apds9960_setup_int(dev->config, false);

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		if (trig->chan == SENSOR_CHAN_PROX) {
			data->p_th_handler = handler;
			data->p_th_trigger = trig;
			if (i2c_reg_update_byte_dt(&config->i2c,
						   APDS9960_ENABLE_REG,
						   APDS9960_ENABLE_PIEN,
						   APDS9960_ENABLE_PIEN)) {
				return -EIO;
			}
		} else {
			return -ENOTSUP;
		}
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	apds9960_setup_int(config, true);
	if (gpio_pin_get_dt(&config->int_gpio) > 0) {
		k_work_submit(&data->work);
	}

	return 0;
}
