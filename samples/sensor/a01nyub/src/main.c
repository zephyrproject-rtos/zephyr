/*
 * Copyright (c) 2023 SteadConnect
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

LOG_MODULE_REGISTER(main);

int main(void)
{
	LOG_INF("Booted DFRobot A01NYUB distance sensor sample");

	const struct device *dev;
	struct sensor_value val;

	dev = DEVICE_DT_GET_ONE(dfrobot_a01nyub);

	if (!device_is_ready(dev)) {
		LOG_ERR("sensor: device not found.");
		return 0;
	}

	while (1) {
		if (sensor_sample_fetch(dev) != 0) {
			LOG_ERR("sensor: sample fetch fail.");
		}

		if (sensor_channel_get(dev, SENSOR_CHAN_DISTANCE, &val) != 0) {
			LOG_ERR("sensor: channel get fail.");
		}

		LOG_INF("sensor: distance [m]: %d.%06d", val.val1, val.val2);

		k_sleep(K_SECONDS(2));
	}
	return 0;
}
