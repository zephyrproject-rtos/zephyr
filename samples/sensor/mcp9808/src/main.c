/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>

#ifdef CONFIG_MCP9808_TRIGGER
static void trigger_handler(struct device *dev, struct sensor_trigger *trig)
{
	struct sensor_value temp;

	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);

	printf("trigger fired, temp %d.%06d\n", temp.val1, temp.val2);
}
#endif

void main(void)
{
	struct device *dev = device_get_binding("MCP9808");

	if (dev == NULL) {
		printf("device not found.  aborting test.\n");
		return;
	}

#ifdef DEBUG
	printf("dev %p\n", dev);
	printf("dev %p name %s\n", dev, dev->config->name);
#endif

#ifdef CONFIG_MCP9808_TRIGGER
	struct sensor_value val;
	struct sensor_trigger trig;

	val.val1 = 26;
	val.val2 = 0;

	sensor_attr_set(dev, SENSOR_CHAN_AMBIENT_TEMP,
			SENSOR_ATTR_UPPER_THRESH, &val);

	trig.type = SENSOR_TRIG_THRESHOLD;
	trig.chan = SENSOR_CHAN_AMBIENT_TEMP;

	if (sensor_trigger_set(dev, &trig, trigger_handler)) {
		printf("Could not set trigger.  aborting test.\n");
		return;
	}
#endif

	while (1) {
		struct sensor_value temp;
		int rc;

		rc = sensor_sample_fetch(dev);
		if (rc != 0) {
			printf("sensor_sample_fetch error: %d\n", rc);
			break;
		}

		rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		if (rc != 0) {
			printf("sensor_channel_get error: %d\n", rc);
			break;
		}

		printf("temp: %d.%06d\n", temp.val1, temp.val2);

		k_sleep(K_MSEC(2000));
	}
}
