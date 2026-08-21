/*
 * Copyright (c) 2026 Carl Zeiss AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

#include <zephyr/drivers/sensor/s9706.h>

#if !DT_HAS_COMPAT_STATUS_OKAY(hamamatsu_s9706)
#error "No hamamatsu,s9706 compatible node found in the device tree"
#endif

int main(void)
{
	const struct device *const s9706 = DEVICE_DT_GET_ANY(hamamatsu_s9706);
	struct sensor_value r, g, b;

	if (!device_is_ready(s9706)) {
		printf("Device %s is not ready.\n", s9706->name);
		return 0;
	}

	struct sensor_value integration_time;

	sensor_attr_get(s9706, SENSOR_CHAN_ALL, SENSOR_ATTR_S9706_INTEGRATION_TIME,
			&integration_time);

	printf("S9706 integration time : %d us\n", integration_time.val1);

	while (true) {
		if (sensor_sample_fetch(s9706)) {
			printf("Failed to fetch sample from S9706 device\n");
			return 0;
		}

		sensor_channel_get(s9706, SENSOR_CHAN_RED, &r);
		sensor_channel_get(s9706, SENSOR_CHAN_GREEN, &g);
		sensor_channel_get(s9706, SENSOR_CHAN_BLUE, &b);

		printf("S9706: R %d G %d B %d\n", r.val1, g.val1, b.val1);

		k_sleep(K_MSEC(500));
	}
}
