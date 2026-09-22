/*
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "tmd2620.h"

LOG_MODULE_DECLARE(TMD2620, CONFIG_SENSOR_LOG_LEVEL);

extern struct tmd2620_data tmd2620_driver;

void tmd2620_work_cb(struct k_work *work)
{
	LOG_DBG("Work callback was called back.");

	struct tmd2620_data *data = CONTAINER_OF(work, struct tmd2620_data, work);
	const struct device *dev = data->dev;

	if (data->p_th_handler != NULL) {
		data->p_th_handler(dev, data->p_th_trigger);
	}

	tmd2620_setup_int(dev->config, true);
}

int tmd2620_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     const struct sensor_value *val)
{
	LOG_DBG("Setting sensor attributes.");

	const struct tmd2620_config *config = dev->config;
	int ret;

	if (chan != SENSOR_CHAN_PROX) {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		ret = i2c_reg_write_byte_dt(&config->i2c, TMD2620_PIHT_REG,
						(255 - (uint8_t)val->val1));
		if (ret < 0) {
			return ret;
		}
	}

	if (attr == SENSOR_ATTR_LOWER_THRESH) {
		return i2c_reg_write_byte_dt(&config->i2c, TMD2620_PILT_REG, (uint8_t)val->val1);
	}

	return 0;
}

int tmd2620_trigger_set(const struct device *dev, const struct sensor_trigger *trigg,
			sensor_trigger_handler_t handler)
{
	LOG_DBG("Setting trigger handler.");

	const struct tmd2620_config *config = dev->config;
	struct tmd2620_data *data = dev->data;
	int ret;

	tmd2620_setup_int(dev->config, false);

	if (trigg->type != SENSOR_TRIG_THRESHOLD) {
		return -ENOTSUP;
	}

	if (trigg->chan != SENSOR_CHAN_PROX) {
		return -ENOTSUP;
	}

	data->p_th_trigger = trigg;
	data->p_th_handler = handler;
	ret = i2c_reg_update_byte_dt(&config->i2c, TMD2620_INTENAB_REG,
				TMD2620_INTENAB_PIEN, TMD2620_INTENAB_PIEN);
	if (ret < 0) {
		return ret;
	}

	tmd2620_setup_int(config, true);
	if (gpio_pin_get_dt(&config->int_gpio) > 0) {
		k_work_submit(&data->work);
	}

	return 0;
}
