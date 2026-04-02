/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

int main(void)
{
	struct sensor_value oversampling_rate = { 8192, 0 };
	const struct device *const dev = DEVICE_DT_GET_ANY(meas_ms5837);

	if (dev == NULL) {
		LOG_ERR("Could not find MS5837 device, aborting test.");
		return 0;
	}
	if (!device_is_ready(dev)) {
		LOG_ERR("MS5837 device %s is not ready, aborting test.",
			dev->name);
		return 0;
	}

	if (sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_OVERSAMPLING,
				&oversampling_rate) != 0) {
		LOG_ERR("Could not set oversampling rate of %d "
			"on MS5837 device, aborting test.",
			oversampling_rate.val1);
		return 0;
	}

	while (1) {
		struct sensor_value temp;
		struct sensor_value press;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);

		printf("Temperature: %d.%06d, Pressure: %d.%06d\n", temp.val1,
		       temp.val2, press.val1, press.val2);

		k_sleep(K_MSEC(10000));
	}
	return 0;
}
