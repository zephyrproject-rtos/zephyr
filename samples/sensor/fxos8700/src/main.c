/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sensor.h>
#include <stdio.h>

static void trigger_handler(struct device *dev, struct sensor_trigger *trigger)
{
	struct sensor_value accel[3];
	struct sensor_value magn[3];

	ARG_UNUSED(trigger);

	if (sensor_sample_fetch(dev)) {
		printf("sensor_sample_fetch failed\n");
		return;
	}

	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_ANY, accel);
	sensor_channel_get(dev, SENSOR_CHAN_MAGN_ANY, magn);

	/* Print accel x,y,z and mag x,y,z data */
	printf("AX=%10.6f AY=%10.6f AZ=%10.6f "
	       "MX=%10.6f MY=%10.6f MZ=%10.6f\n",
	       sensor_value_to_double(&accel[0]),
	       sensor_value_to_double(&accel[1]),
	       sensor_value_to_double(&accel[2]),
	       sensor_value_to_double(&magn[0]),
	       sensor_value_to_double(&magn[1]),
	       sensor_value_to_double(&magn[2]));
}

void main(void)
{
	struct device *dev = device_get_binding(CONFIG_FXOS8700_NAME);

	if (dev == NULL) {
		printf("Could not get fxos8700 device\n");
		return;
	}

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_ANY,
	};

	if (sensor_trigger_set(dev, &trig, trigger_handler)) {
		printf("Could not set trigger\n");
		return;
	}
}
