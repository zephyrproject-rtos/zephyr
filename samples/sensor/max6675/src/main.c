/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

/**
 * @file Sample app using the MAX6675 cold-junction-compensated K-thermocouple
 *	 to digital converter.
 *
 * This app will read and display the sensor temperature every second.
 */

void main(void)
{
	const struct device *dev = DEVICE_DT_GET_ONE(maxim_max6675);
	struct sensor_value val;

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
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
