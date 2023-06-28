/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensor.h>
#include <zephyr/ztest.h>

ZTEST(sensing, test_open_connections_limit)
{
	const struct sensing_sensor_info *sensor = SENSING_SENSOR_INFO_GET(
		DT_NODELABEL(accelgyro), SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D);
	const struct sensing_callback_list cb_list;
	sensing_sensor_handle_t handles[CONFIG_SENSING_MAX_CONNECTIONS + 1];

	zassert_not_null(sensor);

	/* Allocate all the connection */
	for (int i = 0; i < CONFIG_SENSING_MAX_CONNECTIONS; ++i) {
		zassert_ok(sensing_open_sensor(sensor, &cb_list, &handles[i]));
	}

	/* Try to over allocate */
	zassert_equal(-ENOSPC, sensing_open_sensor(sensor, &cb_list,
						   &handles[CONFIG_SENSING_MAX_CONNECTIONS]));

	/* Free one connection */
	zassert_ok(sensing_close_sensor(handles[0]));

	/* Allocate one */
	zassert_ok(sensing_open_sensor(sensor, &cb_list, &handles[CONFIG_SENSING_MAX_CONNECTIONS]));
}

ZTEST(sensing, test_connection_get_info)
{
	const struct sensing_sensor_info *sensor = SENSING_SENSOR_INFO_GET(
		DT_NODELABEL(accelgyro), SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D);
	const struct sensing_callback_list cb_list;
	sensing_sensor_handle_t handle;

	zassert_not_null(sensor);
	zassert_ok(sensing_open_sensor(sensor, &cb_list, &handle));

	const struct sensing_sensor_info *info = sensing_get_sensor_info(handle);
	zassert_equal_ptr(sensor, info);
}
