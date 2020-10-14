/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Thorvald Natvig
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/sensor.h>

#define SI7006 DT_INST(0, silabs_si7006)

#if DT_NODE_HAS_STATUS(SI7006, okay)
#define SI7006_LABEL DT_LABEL(SI7006)
#else
#error Your devicetree has no enabled nodes with compatible "silabs,si7006"
#define SI7006_LABEL "<none>"
#endif

void main(void)
{
	const struct device *dev = device_get_binding(SI7006_LABEL);

	if (dev == NULL) {
		printk("No device \"%s\" found; did initialization fail?\n",
		       SI7006_LABEL);
		return;
	} else {
		printk("Found device \"%s\"\n", SI7006_LABEL);
	}

	while (1) {
		int res;
		struct sensor_value temp, humidity;

		res = sensor_sample_fetch(dev);
		if (res != 0) {
			printk("Error: Failed to read sensor (%d)\n", res);
		} else {
			sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
			sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);

			printk("temp: %d.%06d; humidity: %d.%06d\n",
			       temp.val1, temp.val2,
			       humidity.val1, humidity.val2);
		}
		k_sleep(K_MSEC(1000));
	}
}
