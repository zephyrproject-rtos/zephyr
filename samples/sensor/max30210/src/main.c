/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// #include <zephyr/drivers/sensor/max30011.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/max30210.h>

#define MAX30210_NODE DT_NODELABEL(max30210)
static const struct device *max30210_dev = DEVICE_DT_GET(MAX30210_NODE);

int main(void)
{
	printf("Starting MAX30210 sample application\n");
	if (!device_is_ready(max30210_dev)) {
		printf("MAX30210 device not ready\n");
		return -ENODEV;
	}
	printf("MAX30210 device is ready: %s\n", max30210_dev->name);

	struct sensor_value temp_sampling_freq = {
		.val1 = 2, // Set your desired sampling frequency in Hz
		.val2 = 0
	};
	
	int ret = sensor_attr_set(max30210_dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_MAX30210_CONTINUOUS_CONVERSION_MODE, NULL);
	ret = sensor_attr_set(max30210_dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_SAMPLING_FREQUENCY, &temp_sampling_freq);

	while (1){

	
	struct sensor_value temp_value;
	ret = sensor_sample_fetch(max30210_dev);
	if (ret < 0) {
		printf("Failed to fetch sample: %d\n", ret);
		return ret;
	}
	ret = sensor_channel_get(max30210_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp_value);
	if (ret < 0) {
		printf("Failed to get temperature channel: %d\n", ret);
		return ret;
	}
	printf("Temperature is: %f C \n", sensor_value_to_float(&temp_value));
	}
	return 0;
}
