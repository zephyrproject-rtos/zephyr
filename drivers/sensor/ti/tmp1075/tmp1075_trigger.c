/*
 * Copyright (c) 2024 Arrow Electronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "tmp1075.h"

LOG_MODULE_DECLARE(TMP1075, CONFIG_SENSOR_LOG_LEVEL);

/*
 * @brief GPIO alert line interrupt callback
 * @param gpio - not used
 * @param cb - callback structure for interrupt handler
 * @param pins - not used
 */
void tmp1075_trigger_handle_alert(const struct device *gpio, struct gpio_callback *cb,
				  gpio_port_pins_t pins)
{
	struct tmp1075_data *drv_data = CONTAINER_OF(cb, struct tmp1075_data, temp_alert_gpio_cb);
	/* Successful read, call set callbacks */
	if (drv_data->temp_alert_handler) {
		drv_data->temp_alert_handler(drv_data->tmp1075_dev, drv_data->temp_alert_trigger);
	}
}

/*
 * @brief callback implementation for setting the custom trigger handler in the userspace
 * @param dev - sensor device struct pointer
 * @param trig - trigger struct pointer to be set up
 * @param handler - pointer to custom callback handler which the user would like to use
 * @return 0 if ok - -ENOTSUP in case of err
 */
int tmp1075_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	struct tmp1075_data *drv_data = dev->data;

	if (trig->type == SENSOR_TRIG_THRESHOLD) {
		drv_data->temp_alert_handler = handler;
		drv_data->temp_alert_trigger = trig;
		return 0;
	}

	return -ENOTSUP;
}
