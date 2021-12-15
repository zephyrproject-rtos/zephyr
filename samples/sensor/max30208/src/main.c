/*
 * Copyright (c) 2021, Silvano Cortesi
 * Copyright (c) 2021, ETH Zürich
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/sensor/max30208.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <zephyr.h>

static struct sensor_value temperature;

#ifdef CONFIG_MAX30208_TRIGGER
static void trigger_handler_drdy(const struct device *dev, struct sensor_trigger *trig)
{
	if (max30208_readout_sample(dev) < 0) {
		printf("Can't readout a new sensor value\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature) < 0) {
		printf("Can't get a new sensor value\n");
		return;
	}
	/* Print temperature data*/
	printf("INTERRUPT-TEMP=%d.%03d\r\n", temperature.val1, temperature.val2 / 1000);
}
#endif

void main(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(maxim_max30208);

	if (dev == NULL) {
		printf("Could not get MAX30208 device\r\n");
		return;
	}

	if (!device_is_ready(dev)) {
		printf("MAX30208 device %s is not ready\r\n", dev->name);
		return;
	}

#ifdef CONFIG_MAX30208_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	sensor_trigger_set(dev, &trig, trigger_handler_drdy);
#endif

	while (1) {
		k_sleep(K_MSEC(1000));

		if (sensor_sample_fetch(dev) < 0) {
			printf("Can't readout a new sensor value\n");
			continue;
		}

#ifndef CONFIG_MAX30208_TRIGGER
		if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature) < 0) {
			printf("Can't get a new sensor value\n");
			continue;
		}
		/* Print temperature data*/
		printf("POLLED-TEMP=%d.%d\r\n", temperature.val1, temperature.val2 / 1000);
#endif
	}
}
