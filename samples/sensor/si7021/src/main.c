/*
 * Copyright (c) Nikolaus Huber, Uppsala University
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/sensor.h>
#include <stdio.h>


/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000


#define SI7021 DT_INST(0, silabs_si7021)
#define SI7021_LABEL DT_LABEL(SI7021)

void main(void)
{
	const struct device *dev = device_get_binding(SI7021_LABEL);

	if (dev == NULL) {
		printf("No device \"%s\" found; did initialization fail?\n", SI7021_LABEL);
		return;
	} else {
		printf("Found device \"%s\"\n", SI7021_LABEL);
	}

	for (;;) {
		struct sensor_value temp, rh;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &rh);

		printf("Temp: %f\n", sensor_value_to_double(&temp));
		printf("RH: %f\n", sensor_value_to_double(&rh));
		k_msleep(SLEEP_TIME_MS);
	}
}
