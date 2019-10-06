/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main);

void main(void)
{
	struct sensor_value oversampling_rate = { 8192, 0 };
	struct device *dev = device_get_binding(DT_INST_0_MEAS_MS5837_LABEL);

	if (dev == NULL) {
		LOG_ERR("Could not find MS5837 device, aborting test.");
		return;
	}

	if (sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_OVERSAMPLING,
				&oversampling_rate) != 0) {
		LOG_ERR("Could not set oversampling rate of %d "
			"on MS5837 device, aborting test.",
			oversampling_rate.val1);
		return;
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
}
