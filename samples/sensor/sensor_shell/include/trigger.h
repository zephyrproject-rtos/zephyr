/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_SENSOR_SENSOR_SHELL_INCLUDE_TRIGGER_H
#define ZEPHYR_SAMPLES_SENSOR_SENSOR_SHELL_INCLUDE_TRIGGER_H

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

void sensor_shell_data_ready_trigger_handler(const struct device *sensor,
					     const struct sensor_trigger *trigger);

#endif /* ZEPHYR_SAMPLES_SENSOR_SENSOR_SHELL_INCLUDE_TRIGGER_H */
