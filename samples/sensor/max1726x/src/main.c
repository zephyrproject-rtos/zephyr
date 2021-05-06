/*
 * Copyright (c) 2020 Matija Tudan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/sensor.h>

#define MAX1726X DT_INST(0, maxim_max1726x)

#if DT_NODE_HAS_STATUS(MAX1726X, okay)
#define MAX1726X_LABEL DT_LABEL(MAX1726X)
#else
#error Your devicetree has no enabled nodes with compatible "maxim,max1726x"
#define MAX1726X_LABEL "<none>"
#endif

void main(void)
{
	const struct device *dev = device_get_binding(MAX1726X_LABEL);

	if (dev == NULL) {
		printk("No device found...\n");
		return;
	}
	printk("Found device %s\n", dev->name);

	while (1) {
		struct sensor_value voltage, avg_current, temperature;
		float i_avg;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_VOLTAGE, &voltage);
		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_AVG_CURRENT,
						  &avg_current);
		sensor_channel_get(dev, SENSOR_CHAN_GAUGE_TEMP, &temperature);

		i_avg = avg_current.val1 + (avg_current.val2 / 1000000.0);

		printk("V: %d.%06d V; I: %f mA; T: %d.%06d °C\n",
		      voltage.val1, voltage.val2, i_avg,
		      temperature.val1, temperature.val2);

		k_sleep(K_MSEC(1000));
	}
}
