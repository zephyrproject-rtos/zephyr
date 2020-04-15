/*
 * Copyright (c) 2020 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>

void main(void)
{
	struct device *dev = device_get_binding(DT_LABEL(DT_INST(0,
	bosch_bma400)));
	struct sensor_value acc[3];
	struct sensor_value full_scale, sampling_freq, oversampling;

	/* Setting scale in G, due to loss of precision if the SI unit m/s^2
	 * is used
	 */
	full_scale.val1 = 8;            /* 8.0G */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;       /* 100.0 Hz */
	sampling_freq.val2 = 0;
	oversampling.val1 = 3;          /* 3.0 : Highest oversampling */
	oversampling.val2 = 0;

	printf("Device %p name is %s\n", dev, dev->config->name);

	if (dev) {
		sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_FULL_SCALE, &full_scale);
		sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_OVERSAMPLING, &oversampling);
		/* Set sampling frequency last as this also sets the
		 * appropriate power mode. If already sampling, change
		 * to 0.0Hz before changing other attributes
		 */
		sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);
	}

	while (dev) {
		k_sleep(K_MSEC(10)); /* 10ms period, 100Hz Sampling frequency */

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, acc);

		printf("X: %d.%06d; Y: %d.%06d; Z: %d.%06d;\n",
			acc[0].val1, acc[0].val2, acc[1].val1, acc[1].val2,
			acc[2].val1, acc[2].val2);
	}
}
