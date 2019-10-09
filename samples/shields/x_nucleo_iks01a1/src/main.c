/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <stdio.h>
#include <sys/util.h>

void main(void)
{
	struct sensor_value temp, hum, press;
	struct sensor_value magn_xyz[3], accel_xyz[3];
	struct device *hts221 = device_get_binding(DT_INST_0_ST_HTS221_LABEL);
	struct device *lis3mdl = device_get_binding(DT_INST_0_ST_LIS3MDL_MAGN_LABEL);
	struct device *lsm6ds0 = device_get_binding(DT_INST_0_ST_LSM6DS0_LABEL);
	struct device *lps25hb = device_get_binding(DT_INST_0_ST_LPS25HB_PRESS_LABEL);

	if (hts221 == NULL) {
		printf("Could not get HTS221 device\n");
		return;
	}
	if (lis3mdl == NULL) {
		printf("Could not get LIS3MDL device\n");
		return;
	}
	if (lsm6ds0 == NULL) {
		printf("Could not get LSM6DS0 device\n");
		return;
	}
	if (lps25hb == NULL) {
		printf("Could not get LPS25HB device\n");
		return;
	}

	while (1) {

		/* Get sensor samples */

		if (sensor_sample_fetch(hts221) < 0) {
			printf("HTS221 Sensor sample update error\n");
			return;
		}
		if (sensor_sample_fetch(lps25hb) < 0) {
			printf("LPS25HB Sensor sample update error\n");
			return;
		}
		if (sensor_sample_fetch(lis3mdl) < 0) {
			printf("LIS3MDL Sensor sample update error\n");
			return;
		}
		if (sensor_sample_fetch(lsm6ds0) < 0) {
			printf("LSM6DS0 Sensor sample update error\n");
			return;
		}

		/* Get sensor data */

		sensor_channel_get(hts221, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(hts221, SENSOR_CHAN_HUMIDITY, &hum);
		sensor_channel_get(lps25hb, SENSOR_CHAN_PRESS, &press);
		sensor_channel_get(lis3mdl, SENSOR_CHAN_MAGN_XYZ, magn_xyz);
		sensor_channel_get(lsm6ds0, SENSOR_CHAN_ACCEL_XYZ, accel_xyz);

		/* Display sensor data */

		/* Erase previous */
		printf("\0033\014");

		printf("X-NUCLEO-IKS01A1 sensor dashboard\n\n");

		/* temperature */
		printf("HTS221: Temperature: %.1f C\n",
		       sensor_value_to_double(&temp));

		/* humidity */
		printf("HTS221: Relative Humidity: %.1f%%\n",
		       sensor_value_to_double(&hum));

		/* pressure */
		printf("LPS25HB: Pressure:%.1f kpa\n",
		       sensor_value_to_double(&press));

		/* magneto data */
		printf(
		 "LIS3MDL: Magnetic field (gauss): x: %.1f, y: %.1f, z: %.1f\n",
		 sensor_value_to_double(&magn_xyz[0]),
		 sensor_value_to_double(&magn_xyz[1]),
		 sensor_value_to_double(&magn_xyz[2]));

		/* acceleration */
		printf(
		   "LSM6DS0: Acceleration (m.s-2): x: %.1f, y: %.1f, z: %.1f\n",
		   sensor_value_to_double(&accel_xyz[0]),
		   sensor_value_to_double(&accel_xyz[1]),
		   sensor_value_to_double(&accel_xyz[2]));

		k_sleep(K_MSEC(2000));
	}
}
