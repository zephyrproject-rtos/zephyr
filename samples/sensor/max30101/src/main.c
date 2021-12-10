/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/sensor.h>
#include <stdio.h>

static struct sensor_value red;

#ifdef CONFIG_MAX30101_TRIGGER
static void trigger_handler_drdy(const struct device *dev, struct sensor_trigger *trig)
{
	if (sensor_sample_fetch(dev) < 0) {
		printf("Can't readout new sensor value\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_RED, &red) < 0) {
		printf("Can't get a new sensor value\n");
		return;
	}
	/* Print red PPG data */
	printf("RED=%d\r\n", red.val1);
}
#endif

void main(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(maxim_max30101);

	if (dev == NULL) {
		printf("Could not get max30101 device\n");
		return;
	}
	if (!device_is_ready(dev)) {
		printf("max30101 device %s is not ready\n", dev->name);
		return;
	}

#ifdef CONFIG_MAX30101_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	sensor_trigger_set(dev, &trig, trigger_handler_drdy);
#endif

	while (1) {
#ifndef CONFIG_MAX30101_TRIGGER
		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_RED, &red);

		/* Print red LED data*/
		printf("RED=%d\n", red.val1);

		k_sleep(K_MSEC(20));
#else
		k_sleep(K_FOREVER);
#endif
	}
}
