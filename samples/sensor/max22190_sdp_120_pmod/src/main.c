/*
 * Copyright (c) 2010-2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(adi_max22190);
	struct sensor_value val;

	if (!device_is_ready(dev)) {
		printk("sensor: device not ready.\n");
		return 0;
	}

	while (1) {
		int ret;

		ret = sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
		if (ret < 0) {
			printf("Could not fetch (%d)\n", ret);
			return 0;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_ALL, &val);
		if (ret < 0) {
			printf("Could not get  (%d)\n", ret);
			return 0;
		}

		for (int ch_n = 0; ch_n < 8; ch_n++) {
			printf("IN_%d=%c ", ch_n + 1, (val.val1 >> ch_n) & 1 ? 'H' : 'L');
			printf("WB_%d=%c ", ch_n + 1, (val.val2 >> ch_n) & 1 ? 'H' : 'L');
		}
		printf("\n");

		k_sleep(K_MSEC(1000));
	}
	return 0;
}
