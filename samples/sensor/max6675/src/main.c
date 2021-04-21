/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>

/**
 * @file Sample app using the MAX6675 cold-junction-compensated K-thermocouple
 *	 to digital converter.
 *
 * This app will read and display the sensor temperature every second.
 */

void main(void)
{
	const struct device *dev;
	struct sensor_value val;

	dev = device_get_binding(DT_LABEL(DT_INST(0, maxim_max6675)));
	if (dev == NULL) {
		printf("Could not obtain MAX6675 device\n");
		return;
	}

	while (1) {
		int ret;

		ret = sensor_sample_fetch_chan(dev, SENSOR_CHAN_AMBIENT_TEMP);
		if (ret < 0) {
			printf("Could not fetch temperature (%d)\n", ret);
			return;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val);
		if (ret < 0) {
			printf("Could not get temperature (%d)\n", ret);
			return;
		}

		printf("Temperature: %.2f C\n", sensor_value_to_double(&val));

		k_sleep(K_MSEC(1000));
	}
}
