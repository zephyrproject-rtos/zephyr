/*
 * Copyright (c) 2018, Yannis Damigos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>

static int32_t read_sensor(const struct device *sensor,
			   enum sensor_channel channel)
{
	struct sensor_value val[3];
	int32_t ret = 0;

	ret = sensor_sample_fetch(sensor);
	if (ret < 0 && ret != -EBADMSG) {
		printf("Sensor sample update error\n");
		goto end;
	}

	ret = sensor_channel_get(sensor, channel, val);
	if (ret < 0) {
		printf("Cannot read sensor channels\n");
		goto end;
	}

	printf("( x y z ) = ( %f  %f  %f )\n", sensor_value_to_double(&val[0]),
					       sensor_value_to_double(&val[1]),
					       sensor_value_to_double(&val[2]));

end:
	return ret;
}

void main(void)
{
	const struct device *accelerometer = device_get_binding(
						DT_LABEL(DT_INST(0, st_lis2dh)));
	const struct device *magnetometer = device_get_binding(
						DT_LABEL(DT_INST(0, st_lsm303dlhc_magn)));

	if (accelerometer == NULL) {
		printf("Could not get %s device\n",
				DT_LABEL(DT_INST(0, st_lis2dh)));
		return;
	}

	if (magnetometer == NULL) {
		printf("Could not get %s device\n",
				DT_LABEL(DT_INST(0, st_lsm303dlhc_magn)));
		return;
	}

	while (1) {
		printf("Magnetometer data:\n");
		if (read_sensor(magnetometer, SENSOR_CHAN_MAGN_XYZ) < 0) {
			printf("Failed to read magnetometer data\n");
		}

		printf("Accelerometer data:\n");
		if (read_sensor(accelerometer, SENSOR_CHAN_ACCEL_XYZ) < 0) {
			printf("Failed to read accelerometer data\n");
		}

		k_sleep(K_MSEC(2000));
	}
}
