/*
 * Copyright (c) 2026 Malluri Sai Raghu
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

#define NTC_NODE DT_ALIAS(ntc_thermistor)

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(NTC_NODE);
	struct sensor_value temp;

	if (!device_is_ready(dev)) {
		printf("NTC thermistor device not ready\n");
		return -1;
	}

	while (1) {
		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		printf("Temperature: %d.%06d C\n", temp.val1, temp.val2);
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
