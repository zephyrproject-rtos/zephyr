/*
 * Copyright (c) 2021, Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

void main(void)
{
	const struct device *dev;
	int ret;

	printf("Quadrature Decoder sample application\n\n");

	dev = device_get_binding("QDEC_0");
	if (!dev) {
		printf("error: no QDEC_0 device\n");
		return;
	}

	while (1) {
		struct sensor_value value;

		ret = sensor_sample_fetch(dev);
		if (ret) {
			printf("sensor_sample_fetch failed returns: %d\n", ret);
			break;
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &value);
		if (ret) {
			printf("sensor_channel_get failed returns: %d\n", ret);
			break;
		}

		printf("Position is %d\n", value.val1);

		k_sleep(K_MSEC(2000));
	}
}
