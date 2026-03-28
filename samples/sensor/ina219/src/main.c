/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/drivers/sensor.h>


int main(void)
{
	const struct device *const ina = DEVICE_DT_GET_ONE(ti_ina219);
	struct sensor_value v_bus, power, current;
	int rc;

	if (!device_is_ready(ina)) {
		printf("Device %s is not ready.\n", ina->name);
		return 0;
	}

	while (true) {
		rc = sensor_sample_fetch(ina);
		if (rc) {
			printf("Could not fetch sensor data.\n");
			return 0;
		}

		sensor_channel_get(ina, SENSOR_CHAN_VOLTAGE, &v_bus);
		sensor_channel_get(ina, SENSOR_CHAN_POWER, &power);
		sensor_channel_get(ina, SENSOR_CHAN_CURRENT, &current);

		printf("Bus: %f [V] -- "
			"Power: %f [W] -- "
			"Current: %f [A]\n",
		       sensor_value_to_double(&v_bus),
		       sensor_value_to_double(&power),
		       sensor_value_to_double(&current));
		k_sleep(K_MSEC(2000));
	}
	return 0;
}
