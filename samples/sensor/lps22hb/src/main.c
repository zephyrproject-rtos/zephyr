/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <sys/util.h>

static void process_sample(const struct device *dev)
{
	static unsigned int obs;
	struct sensor_value pressure, temp;

	if (sensor_sample_fetch(dev) < 0) {
		printf("Sensor sample update error\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure) < 0) {
		printf("Cannot read LPS22HB pressure channel\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
		printf("Cannot read LPS22HB temperature channel\n");
		return;
	}

	++obs;
	printf("Observation:%u\n", obs);

	/* display pressure */
	printf("Pressure:%.1f kPa\n", sensor_value_to_double(&pressure));

	/* display temperature */
	printf("Temperature:%.1f C\n", sensor_value_to_double(&temp));

}

void main(void)
{
	const struct device *dev = device_get_binding(DT_LABEL(DT_INST(0, st_lps22hb_press)));

	if (dev == NULL) {
		printf("Could not get LPS22HB device\n");
		return;
	}

	while (true) {
		process_sample(dev);
		k_sleep(K_MSEC(2000));
	}
}
