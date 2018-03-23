/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sensor.h>
#include <stdio.h>

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

void main(void)
{
	struct sensor_value accel[3];
	struct sensor_value magn[3];
	struct sensor_value temp;
	struct device *dev = device_get_binding(CONFIG_FXOS8700_NAME);

	if (dev == NULL) {
		printf("Could not get fxos8700 device\n");
		return;
	}

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	if (sensor_trigger_set(dev, &trig, trigger_handler)) {
		printf("Could not set trigger\n");
		return;
	}

	while (1) {
		k_sem_take(&sem, K_FOREVER);

		sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);
		sensor_channel_get(dev, SENSOR_CHAN_MAGN_XYZ, magn);
		sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &temp);

		/* Print accel x,y,z and mag x,y,z data */
		printf("AX=%10.6f AY=%10.6f AZ=%10.6f "
		       "MX=%10.6f MY=%10.6f MZ=%10.6f "
		       "T=%10.6f\n",
		       sensor_value_to_double(&accel[0]),
		       sensor_value_to_double(&accel[1]),
		       sensor_value_to_double(&accel[2]),
		       sensor_value_to_double(&magn[0]),
		       sensor_value_to_double(&magn[1]),
		       sensor_value_to_double(&magn[2]),
		       sensor_value_to_double(&temp));
	}
}
