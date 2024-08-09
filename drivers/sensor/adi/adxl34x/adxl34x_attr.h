/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL34X_ATTR_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL34X_ATTR_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

int adxl34x_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     const struct sensor_value *val);
int adxl34x_attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		     struct sensor_value *val);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL34X_ATTR_H_ */
