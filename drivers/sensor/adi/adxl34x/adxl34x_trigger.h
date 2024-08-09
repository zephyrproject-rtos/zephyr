/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL34X_TRIGGER_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL34X_TRIGGER_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adxl34x.h>

int adxl34x_trigger_init(const struct device *dev);
int adxl34x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
int adxl34x_trigger_reset(const struct device *dev);
int adxl34x_trigger_flush(const struct device *dev);
void adxl34x_handle_motion_events(const struct device *dev, struct adxl34x_int_source int_source);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL34X_TRIGGER_H_ */
