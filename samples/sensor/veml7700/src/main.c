/*
 * Copyright (c) 2022, Nikolaus Huber
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <sys/__assert.h>

int main(void)
{
	const struct device *veml7700 = DEVICE_DT_GET_ANY(vishay_veml7700);

	if (veml7700 == NULL) {
		printf("Failed to get device binding\n");
		return -1;
	}

	if (!device_is_ready(veml7700)) {
		printf("Device %s is not ready\n", veml7700->name);
		return -1;
	}

	struct sensor_value lux;

	while (true) {
		sensor_sample_fetch(veml7700);
		sensor_channel_get(veml7700, SENSOR_CHAN_LIGHT, &lux);
		printf("Lux: %f\n", sensor_value_to_double(&lux));
		k_msleep(2000);
	}

	return 0;
}
