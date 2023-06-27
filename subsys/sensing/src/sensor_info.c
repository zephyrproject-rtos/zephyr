/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensor.h>

int sensing_get_sensors(int *num_sensors, const struct sensing_sensor_info **info)
{
	STRUCT_SECTION_COUNT(sensing_sensor_info, num_sensors);
	*info = STRUCT_SECTION_START(sensing_sensor_info);
	return 0;
}
