/*
 * Copyright (c) 2025 Croxel
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <math.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	const struct device *als31300;
	struct sensor_value x_val, y_val, z_val, temp_val;
	int ret;

	printk("ALS31300 3D Hall-Effect Sensor Sample\n");

	/* Get the ALS31300 device */
	als31300 = DEVICE_DT_GET(DT_NODELABEL(als31300));
	if (!device_is_ready(als31300)) {
		LOG_ERR("ALS31300 device not ready");
		return -1;
	}

	LOG_INF("ALS31300 device found and ready");

	while (1) {
		/* Fetch sensor data */
		ret = sensor_sample_fetch(als31300);
		if (ret < 0) {
			LOG_ERR("Failed to fetch sensor data: %d", ret);
			k_sleep(K_MSEC(1000));
			continue;
		}

		/* Get magnetic field data for all three axes */
		ret = sensor_channel_get(als31300, SENSOR_CHAN_MAGN_X, &x_val);
		if (ret < 0) {
			LOG_ERR("Failed to get X-axis data: %d", ret);
			continue;
		}

		ret = sensor_channel_get(als31300, SENSOR_CHAN_MAGN_Y, &y_val);
		if (ret < 0) {
			LOG_ERR("Failed to get Y-axis data: %d", ret);
			continue;
		}

		ret = sensor_channel_get(als31300, SENSOR_CHAN_MAGN_Z, &z_val);
		if (ret < 0) {
			LOG_ERR("Failed to get Z-axis data: %d", ret);
			continue;
		}

		/* Get temperature data */
		ret = sensor_channel_get(als31300, SENSOR_CHAN_AMBIENT_TEMP, &temp_val);
		if (ret < 0) {
			LOG_ERR("Failed to get temperature data: %d", ret);
			continue;
		}

		/* Log the sensor readings */
		LOG_INF("Magnetic Field (Gauss): X=%d.%06d, Y=%d.%06d, Z=%d.%06d", x_val.val1,
			abs(x_val.val2), y_val.val1, abs(y_val.val2), z_val.val1, abs(z_val.val2));

		LOG_INF("Temperature: %d.%06d°C", temp_val.val1, abs(temp_val.val2));

		/* Sleep for 500ms */
		k_sleep(K_MSEC(500));
	}

	return 0;
}
