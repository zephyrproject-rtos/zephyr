/*
 * Copyright (c) 2021 Jimmy Johnson <catch22@fastmail.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "tmp108.h"

LOG_MODULE_DECLARE(TMP108, CONFIG_SENSOR_LOG_LEVEL);

void tmp108_trigger_handle_alert(const struct device *gpio,
				 struct gpio_callback *cb,
				 gpio_port_pins_t pins)
{

	struct tmp108_data *drv_data = CONTAINER_OF(cb,
						    struct tmp108_data,
						    temp_alert_gpio_cb);

	/* Successful read, call set callbacks */
	if (drv_data->temp_alert_handler) {
		drv_data->temp_alert_handler(drv_data->tmp108_dev,
					     drv_data->temp_alert_trigger);
	}
}

int tmp_108_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct tmp108_data *drv_data = dev->data;

	if (trig->type == SENSOR_TRIG_THRESHOLD) {
		drv_data->temp_alert_handler = handler;
		drv_data->temp_alert_trigger = trig;
		return 0;
	}

	return -ENOTSUP;
}
