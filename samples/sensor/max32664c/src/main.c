/*
 * Copyright (c) 2025 Daniel Kampert
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/drivers/sensor/max32664c.h>

static const struct device *const sensor_hub = DEVICE_DT_GET(DT_ALIAS(sensor));

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static void update(void)
{
	struct sensor_value value;

	sensor_sample_fetch(sensor_hub);
	sensor_attr_get(sensor_hub, SENSOR_CHAN_MAX32664C_HEARTRATE, SENSOR_ATTR_MAX32664C_OP_MODE,
			&value);
	if (value.val1 == MAX32664C_OP_MODE_RAW) {
		struct sensor_value x;
		struct sensor_value y;
		struct sensor_value z;

		if (sensor_channel_get(sensor_hub, SENSOR_CHAN_ACCEL_X, &x) ||
		    sensor_channel_get(sensor_hub, SENSOR_CHAN_ACCEL_Y, &y) ||
		    sensor_channel_get(sensor_hub, SENSOR_CHAN_ACCEL_Z, &z)) {
			LOG_ERR("Failed to get accelerometer data");
			return;
		}

		LOG_INF("\tx: %i", x.val1);
		LOG_INF("\ty: %i", y.val1);
		LOG_INF("\tz: %i", z.val1);
	} else if (value.val1 == MAX32664C_OP_MODE_ALGO_AEC) {
		struct sensor_value hr;

		if (sensor_channel_get(sensor_hub, SENSOR_CHAN_MAX32664C_HEARTRATE, &hr)) {
			LOG_ERR("Failed to get heart rate data");
			return;
		}

		LOG_INF("HR: %u bpm", hr.val1);
		LOG_INF("HR Confidence: %u", hr.val2);
	} else {
		LOG_WRN("Operation mode not implemented: %u", value.val1);
	}
}

int main(void)
{
	struct sensor_value value;

	if (!device_is_ready(sensor_hub)) {
		LOG_ERR("Sensor hub not ready!");
		return -1;
	}

	LOG_INF("Sensor hub ready");

	value.val1 = MAX32664C_OP_MODE_ALGO_AEC;
	value.val2 = MAX32664C_ALGO_MODE_CONT_HRM;
	sensor_attr_set(sensor_hub, SENSOR_CHAN_MAX32664C_HEARTRATE, SENSOR_ATTR_MAX32664C_OP_MODE,
			&value);

	while (1) {
		update();
		k_msleep(1000);
	}

	return 0;
}
