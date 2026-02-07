/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 * Copyright (c) 2026 Swarovski Optik AG & Co KG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM45605_TRIGGER_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM45605_TRIGGER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

/* Configure triggers for the icm45605 sensor */
int icm45605_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

/* Initialization for icm45605 Triggers module */
int icm45605_trigger_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM45605_TRIGGER_H_ */
