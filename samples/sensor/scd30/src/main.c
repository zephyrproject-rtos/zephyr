/*
 * Copyright (c) 2021 Advanced Climate Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>

#define SCD30_SAMPLE_TIME_SECONDS 5U

void main(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(sensirion_scd30);
	struct sensor_value sample_period = {
		.val1 = SCD30_SAMPLE_TIME_SECONDS,
	};
	int rc;

	if (!device_is_ready(dev)) {
		printf("Could not get SCD30 device\n");
		return;
	}

	rc = sensor_attr_set(dev, SENSOR_CHAN_ALL,
				SENSOR_ATTR_SAMPLING_PERIOD, &sample_period);
	if (rc != 0) {
		printk("Failed to set sample period. (%d)", rc);
		return;
	}

	while (true) {
		struct sensor_value co2_concentration;

		rc = sensor_sample_fetch(dev);
		if (rc == 0) {
			rc = sensor_channel_get(dev, SENSOR_CHAN_CO2, &co2_concentration);
		}

		if (rc == -ENODATA) {
			printk("SCD30: no new measurment yet.\n");
			printk("Waiting for 1 second and retrying...\n");
			k_sleep(K_SECONDS(1));
			continue;
		} else if (rc != 0) {
			printf("SCD30 channel get: failed: %d\n", rc);
			break;
		}

		printf("SCD30: %d ppm\n", co2_concentration.val1);

		k_sleep(K_SECONDS(SCD30_SAMPLE_TIME_SECONDS));
	}
}
