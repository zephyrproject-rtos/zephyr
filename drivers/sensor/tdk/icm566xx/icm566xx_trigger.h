/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM566XX_TRIGGER_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM566XX_TRIGGER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

/* Configure triggers for the icm566xx sensor */
int icm566xx_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

/* Initialization for icm566xx Triggers module */
int icm566xx_trigger_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM566XX_TRIGGER_H_ */
