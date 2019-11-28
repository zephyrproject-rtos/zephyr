/*
 * Copyright (c) 2018 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include "apds9960.h"

extern struct apds9960_data apds9960_driver;

#include <logging/log.h>
LOG_MODULE_DECLARE(APDS9960, CONFIG_SENSOR_LOG_LEVEL);

void apds9960_work_cb(struct k_work *work)
{
	struct apds9960_data *data = CONTAINER_OF(work,
						  struct apds9960_data,
						  work);
	struct device *dev = data->dev;

	if (data->p_th_handler != NULL) {
		data->p_th_handler(dev, &data->p_th_trigger);
	}

	gpio_pin_enable_callback(data->gpio, data->gpio_pin);
}

int apds9960_attr_set(struct device *dev,
		      enum sensor_channel chan,
		      enum sensor_attribute attr,
		      const struct sensor_value *val)
{
	const struct apds9960_config *config = dev->config->config_info;
	struct apds9960_data *data = dev->driver_data;

	if (chan == SENSOR_CHAN_PROX) {
		if (attr == SENSOR_ATTR_UPPER_THRESH) {
			if (i2c_reg_write_byte(data->i2c,
					       config->i2c_address,
					       APDS9960_PIHT_REG,
					       (u8_t)val->val1)) {
				return -EIO;
			}

			return 0;
		}
		if (attr == SENSOR_ATTR_LOWER_THRESH) {
			if (i2c_reg_write_byte(data->i2c,
					       config->i2c_address,
					       APDS9960_PILT_REG,
					       (u8_t)val->val1)) {
				return -EIO;
			}

			return 0;
		}
	}

	return -ENOTSUP;
}

int apds9960_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct apds9960_config *config = dev->config->config_info;
	struct apds9960_data *data = dev->driver_data;

	gpio_pin_disable_callback(data->gpio, config->gpio_pin);

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		if (trig->chan == SENSOR_CHAN_PROX) {
			data->p_th_handler = handler;
			if (i2c_reg_update_byte(data->i2c,
						config->i2c_address,
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

	gpio_pin_enable_callback(data->gpio, config->gpio_pin);

	return 0;
}
