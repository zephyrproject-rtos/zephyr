/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/ztest.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensing_sensor.h>

#define DT_DRV_COMPAT	zephyr_sensing
#define MAX_SENSOR_TYPES 2
#define DT_SENSOR_INFO(node)		\
	{				\
		.name = DT_NODE_FULL_NAME(node),	\
		.friendly_name = DT_PROP(node, friendly_name), \
		.sensor_types = DT_PROP(node, sensor_types), \
		.sensor_type_count = DT_PROP_LEN(node, sensor_types), \
	}

struct sensor_info_t {
	const char *name;
	const char *friendly_name;
	const int sensor_types[MAX_SENSOR_TYPES];
	int sensor_type_count;
};

static const struct sensor_info_t sensors[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_DRV_INST(0), DT_SENSOR_INFO, (,))
};

static int get_total_sensor_counts(void)
{
	int total = 0;

	for (int i = 0; i < ARRAY_SIZE(sensors); i++) {
		total += sensors[i].sensor_type_count;
	}
	return total;
}

static bool check_sensor_type(const struct sensing_sensor_info *info,
			      const struct sensor_info_t *sensor)
{
	for (int i = 0; i < sensor->sensor_type_count; ++i) {
		if (info->type == sensor->sensor_types[i]) {
			return true;
		}
	}
	return false;
}

/**
 * @brief Test Get Sensors
 *
 * This test verifies sensing_get_sensors.
 */
ZTEST(sensing_tests, test_sensing_get_sensors)
{
	const struct sensing_sensor_info *info;
	int ret, total_sensor_counts = get_total_sensor_counts();
	int num = total_sensor_counts;

	ret = sensing_get_sensors(&num, &info);
	zassert_equal(ret, 0, "Sensing Get Sensors failed");
	zassert_equal(num, total_sensor_counts, "Expected %d sensors, but got %d",
		      total_sensor_counts, num);
	zassert_not_null(info, "Expected sensor info to be not null");

	for (int i = 0; i < num; ++i) {
		bool found = false;

		for (int j = 0; j < ARRAY_SIZE(sensors); ++j) {
			if (strcmp(info[i].name, sensors[j].name) == 0) {
				zassert_true(strcmp(info[i].friendly_name,
						    sensors[j].friendly_name) == 0,
					     "Mismatch in friendly name for sensor '%s'",
					     info[i].name);
				zassert_true(check_sensor_type(&info[i], &sensors[j]),
					     "Mismatch in sensor type for sensor '%s'",
					     info[i].name);
				found = true;
				break;
			}
		}
		zassert_true(found, "Sensor '%s' not found", info[i].name);
	}
}

ZTEST_SUITE(sensing_tests, NULL, NULL, NULL, NULL, NULL);
