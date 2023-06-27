/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensor.h>
#include <zephyr/ztest.h>

#include <string.h>

ZTEST_SUITE(sensing, NULL, NULL, NULL, NULL, NULL);

ZTEST(sensing, test_list_sensors)
{
	int num_sensors;
	const struct sensing_sensor_info *sensors;
	const struct sensor_info *expected_info = &SENSOR_INFO_DT_NAME(DT_NODELABEL(icm42688));

	zassert_ok(sensing_get_sensors(&num_sensors, &sensors));
	zassert_equal(2, num_sensors, "num_sensors=%d", num_sensors);

	zassert_equal_ptr(expected_info, sensors[0].info);
	zassert_equal_ptr(expected_info, sensors[1].info);

	zassert_true(sensors[0].type == SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D ||
		     sensors[0].type == SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D);
	zassert_true(sensors[1].type == SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D ||
		     sensors[1].type == SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D);
	zassert_not_equal(sensors[0].type, sensors[1].type);
}

ZTEST(sensing, test_get_single_node)
{
	const struct sensor_info *expected_info = &SENSOR_INFO_DT_NAME(DT_NODELABEL(icm42688));
	const struct sensing_sensor_info *info = SENSING_SENSOR_INFO_GET(
		DT_NODELABEL(accelgyro), SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D);

	zassert_equal_ptr(expected_info, info->info);
	zassert_equal(SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D, info->type);

	info = SENSING_SENSOR_INFO_GET(DT_NODELABEL(accelgyro),
				       SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D);

	zassert_equal_ptr(expected_info, info->info);
	zassert_equal(SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D, info->type);
}
