/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

K_SEM_DEFINE(sem, 0, 1);	/* starts off "not available" */

static void trigger_handler(const struct device *dev,
			    const struct sensor_trigger *trigger)
{
	k_sem_give(&sem);
}

void main(void)
{
	struct sensor_value gyro[3];
	const struct device *dev = DEVICE_DT_GET_ANY(nxp_fxas21002);

	if (dev == NULL || !device_is_ready(dev)) {
		printf("Could not get fxas21002 device\n");
		return;
	}

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_GYRO_XYZ,
	};

	if (sensor_trigger_set(dev, &trig, trigger_handler)) {
		printf("Could not set trigger\n");
		return;
	}

	while (1) {
		k_sem_take(&sem, K_FOREVER);
		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);

		/* Print gyro x,y,z */
		printf("X=%10.3f Y=%10.3f Z=%10.3f\n",
		       sensor_value_to_double(&gyro[0]),
		       sensor_value_to_double(&gyro[1]),
		       sensor_value_to_double(&gyro[2]));
	}
}
