/*
 * Copyright (c) 2017, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sensor.h>
#include <stdio.h>

void main(void)
{
	struct sensor_value gyro[3];
	struct sensor_value accel[3];
	struct device *dev = device_get_binding(CONFIG_LSM6DSL_DEV_NAME);

	if (dev == NULL) {
		printf("Could not get LSM6DSL device\n");
		return;
	}

	while (1) {

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyro);
		sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel);

		/* Print gyro x,y,z */
		printf("Angular rate in millidegrees per sec:\n");
		printf("X=%10.1f Y=%10.1f Z=%10.1f\n\n",
		       sensor_value_to_double(&gyro[0]),
		       sensor_value_to_double(&gyro[1]),
		       sensor_value_to_double(&gyro[2]));
		/* Print accel x,y,z */
		printf("Linear acceleration in mg:\n");
		printf("X=%10.1f Y=%10.1f Z=%10.1f\n\n",
		       sensor_value_to_double(&accel[0]),
		       sensor_value_to_double(&accel[1]),
		       sensor_value_to_double(&accel[2]));

		k_sleep(2000);
		}
}
