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

#include "tmd2635.h"

LOG_MODULE_DECLARE(TMD2635, CONFIG_SENSOR_LOG_LEVEL);

extern struct tmd2635_data tmd2635_driver;

void tmd2635_work_cb(struct k_work *work)
{
	LOG_DBG("Work callback was called back.");

	struct tmd2635_data *data = CONTAINER_OF(work, struct tmd2635_data, work);
	const struct device *dev = data->dev;

	if (data->p_th_handler != NULL) {
		data->p_th_handler(dev, data->p_th_trigger);
	}

	tmd2635_setup_int(dev->config, true);
}

int tmd2635_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     const struct sensor_value *val)
{
	LOG_DBG("Setting sensor attributes.");

	const struct tmd2635_config *config = dev->config;
	int ret;

	if (chan != SENSOR_CHAN_PROX) {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		ret = i2c_reg_write_byte_dt(&config->i2c, TMD2635_PIHTH_REG,
					    (255 - (uint8_t)val->val1));
		if (ret < 0) {
			return ret;
		}
	}

	if (attr == SENSOR_ATTR_LOWER_THRESH) {
		return i2c_reg_write_byte_dt(&config->i2c, TMD2635_PILTL_REG, (uint8_t)val->val1);
	}

	return 0;
}

int tmd2635_trigger_set(const struct device *dev, const struct sensor_trigger *trigg,
			sensor_trigger_handler_t handler)
{
	LOG_DBG("Setting trigger handler.");

	const struct tmd2635_config *config = dev->config;
	struct tmd2635_data *data = dev->data;
	int ret;

	tmd2635_setup_int(dev->config, false);

	if (trigg->type != SENSOR_TRIG_THRESHOLD) {
		return -ENOTSUP;
	}

	if (trigg->chan != SENSOR_CHAN_PROX) {
		return -ENOTSUP;
	}

	data->p_th_trigger = trigg;
	data->p_th_handler = handler;
	ret = i2c_reg_update_byte_dt(&config->i2c, TMD2635_INTENAB_REG, TMD2635_INTENAB_PIEN,
				     TMD2635_INTENAB_PIEN);
	if (ret < 0) {
		return ret;
	}

	tmd2635_setup_int(config, true);
	if (gpio_pin_get_dt(&config->int_gpio) > 0) {
		k_work_submit(&data->work);
	}

	return 0;
}
