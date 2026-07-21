/*
 * Copyright 2022 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_INA2XX_TRIGGER_H_
#define ZEPHYR_DRIVERS_SENSOR_INA2XX_TRIGGER_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

struct ina2xx_trigger {
	struct gpio_callback gpio_cb;
	struct k_work conversion_work;
	sensor_trigger_handler_t handler_alert;
	const struct sensor_trigger *trig_alert;
};

int ina2xx_trigger_mode_init(struct ina2xx_trigger *trigg,
			     const struct gpio_dt_spec *alert_gpio);

#endif /* ZEPHYR_DRIVERS_SENSOR_INA2XX_TRIGGER_H_ */
