/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sensor.h>
#include <stdio.h>

#ifdef CONFIG_LIS3MDL_TRIGGER_OWN_THREAD

#define DECIMATION_FACTOR	4

K_SEM_DEFINE(sem, 0, 1);	/* starts off "not available" */

static void trigger_handler(struct device *dev, struct sensor_trigger *trigger)
{
	static int decimator;
	ARG_UNUSED(trigger);

	/* Always fetch the sample to clear the data ready interrupt in the
	 * sensor.
	 */
	if (sensor_sample_fetch(dev)) {
		printf("sensor_sample_fetch failed\n");
		return;
	}

	/* Decimate the sensor data before printing to the console. There is
	 * not enough bandwidth on the UART at 115200 baud to print every
	 * sample.
	 */
	if (++decimator < DECIMATION_FACTOR) {
		return;
	}

	decimator = 0;

	k_sem_give(&sem);
}

#endif /* CONFIG_LIS3MDL_TRIGGER_OWN_THREAD */

void main(void)
{
	struct sensor_value magn[3];
	struct device *dev = device_get_binding("LIS3MDL");

	if (dev == NULL) {
		printf("Could not get lis3mdl device\n");
		return;
	}

#ifdef CONFIG_LIS3MDL_TRIGGER_OWN_THREAD
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	if (sensor_trigger_set(dev, &trig, trigger_handler)) {
		printf("Could not set trigger\n");
		return;
	}
#endif /* CONFIG_LIS3MDL_TRIGGER_OWN_THREAD */

	while (1) {
#ifdef CONFIG_LIS3MDL_TRIGGER_OWN_THREAD
		k_sem_take(&sem, K_FOREVER);
#endif /* CONFIG_LIS3MDL_TRIGGER_OWN_THREAD */

		sensor_sample_fetch(dev);

		sensor_channel_get(dev, SENSOR_CHAN_MAGN_XYZ, magn);

		/* Print mag x,y,z data */
		printf("MX=%10.6f MY=%10.6f MZ=%10.6f\n",
		       sensor_value_to_double(&magn[0]),
		       sensor_value_to_double(&magn[1]),
		       sensor_value_to_double(&magn[2]));

		k_sleep(1000);
	}

}
