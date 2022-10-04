/*
 * Copyright (c) 2021 Jonathan Hahn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#define SAMPLING_INTERVAL_MS 10
#define DISPLAY_INTERVAL_MS 50

static int set_sampling_frequency(const struct device *sensor, double sampling_frequency)
{
	struct sensor_value setting;

	(void)sensor_value_from_double(&setting, sampling_frequency);

	return sensor_attr_set(sensor,
			       SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &setting);
}

static void fetch_and_display(const struct device *sensor)
{
	struct sensor_value gyro[3];

	int rc = sensor_sample_fetch(sensor);

	if (rc != 0) {
		printf("ERROR: Failed fetching gyro values: %d.\n", rc);
		return;
	}

	rc = sensor_channel_get(sensor,
				SENSOR_CHAN_GYRO_XYZ,
				gyro);

	if (rc != 0) {
		printf("ERROR: Failed getting gyro values: %d\n", rc);
		return;
	}

	printf("%u ms: x %f , y %f , z %f\n",
	       k_uptime_get_32(),
	       sensor_value_to_double(&gyro[0]),
	       sensor_value_to_double(&gyro[1]),
	       sensor_value_to_double(&gyro[2]));
}

void main(void)
{
	const double sampling_frequency = 1000.0 / SAMPLING_INTERVAL_MS;
	const struct device *sensor = DEVICE_DT_GET_ONE(st_i3g4250d);

	if (!device_is_ready(sensor)) {
		printf("Sensor %s is not ready\n", sensor->name);
		return;
	}

	printf("Set sensor sampling frequency to %f Hz.\n", sampling_frequency);
	set_sampling_frequency(sensor, sampling_frequency);

	printf("Start polling with an interval of %d ms\n", DISPLAY_INTERVAL_MS);
	while (true) {
		fetch_and_display(sensor);

		/* Wait some time before printing the next value */
		k_sleep(K_MSEC(DISPLAY_INTERVAL_MS));
	}
}
