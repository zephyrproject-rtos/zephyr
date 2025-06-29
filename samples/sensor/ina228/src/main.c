/*
 * Copyright (c) 2025 Jonas Berg
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

int main(void)
{
	const struct device *const curr_sensor = DEVICE_DT_GET_ONE(ti_ina228);

	struct sensor_value bus_voltage = {0};
	struct sensor_value charge = {0};
	struct sensor_value current = {0};
	struct sensor_value energy = {0};
	struct sensor_value power = {0};
	struct sensor_value shunt_voltage = {0};
	struct sensor_value temperature = {0};
	int err;

	if (!device_is_ready(curr_sensor)) {
		printf("Sensor %s is not ready.\n", curr_sensor->name);
		return -1;
	}

	while (true) {
		err = sensor_sample_fetch(curr_sensor);
		if (err) {
			printf("Failed to fetch sensor data.\n");
			return -1;
		}

		if (sensor_channel_get(curr_sensor, SENSOR_CHAN_VOLTAGE, &bus_voltage)) {
			printf("Failed to get bus_voltage\n");
		}
		if (sensor_channel_get(curr_sensor, SENSOR_CHAN_CHARGE, &charge)) {
			printf("Failed to get charge\n");
		}
		if (sensor_channel_get(curr_sensor, SENSOR_CHAN_CURRENT, &current)) {
			printf("Failed to get current\n");
		}
		if (sensor_channel_get(curr_sensor, SENSOR_CHAN_ENERGY, &energy)) {
			printf("Failed to get energy\n");
		}
		if (sensor_channel_get(curr_sensor, SENSOR_CHAN_POWER, &power)) {
			printf("Failed to get power\n");
		}
		if (sensor_channel_get(curr_sensor, SENSOR_CHAN_VSHUNT, &shunt_voltage)) {
			printf("Failed to get shunt voltage\n");
		}
		if (sensor_channel_get(curr_sensor, SENSOR_CHAN_DIE_TEMP, &temperature)) {
			printf("Failed to get temperature\n");
		}

		printf("Bus %.3f V  Current %.3f A  Power %.3f W  Shunt %.6f V  "
		       "Charge %.6f C  Energy %.6f J  Temp %.1f deg C\n",
		       sensor_value_to_double(&bus_voltage), sensor_value_to_double(&current),
		       sensor_value_to_double(&power), sensor_value_to_double(&shunt_voltage),
		       sensor_value_to_double(&charge), sensor_value_to_double(&energy),
		       sensor_value_to_double(&temperature));

		k_msleep(3000);
	}

	return 0;
}
