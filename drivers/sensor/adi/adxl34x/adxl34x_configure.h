/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL34X_CONFIGURE_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL34X_CONFIGURE_H_

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor/adxl34x.h>

int adxl34x_configure(const struct device *dev, struct adxl34x_cfg *new_cfg);
int adxl34x_get_configuration(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL34X_CONFIGURE_H_ */
