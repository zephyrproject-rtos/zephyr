/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensing, CONFIG_SENSING_LOG_LEVEL);

int sensing_get_sensors(int *num_sensors, const struct sensing_sensor_info **info)
{
	STRUCT_SECTION_COUNT(sensing_sensor_info, num_sensors);
	*info = STRUCT_SECTION_START(sensing_sensor_info);
	return 0;
}
