/*
 * Copyright (c) 2021 Leonard Pollak
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stdio.h>
#include <drivers/sensor/ina219.h>


#if !DT_HAS_COMPAT_STATUS_OKAY(ti_ina219)
#error "No ti,ina219 compatible node found in the device tree"
#endif

void main(void)
{
	const struct device *ina = DEVICE_DT_GET_ANY(ti_ina219);
	struct sensor_value v_shunt, v_bus, power, current;

	if (!device_is_ready(ina)) {
		printf("Device %s is not ready.\n", ina->name);
		return;
	}

	while (true) {

		if (sensor_sample_fetch(ina)) {
			printf("Could not fetch sensor data.\n");
			return;
		}

		sensor_channel_get(ina, SENSOR_CHAN_INA219_V_SHUNT, &v_shunt);
		sensor_channel_get(ina, SENSOR_CHAN_INA219_BUS_VOLTAGE, &v_bus);
		sensor_channel_get(ina, SENSOR_CHAN_INA219_BUS_POWER, &power);
		sensor_channel_get(ina, SENSOR_CHAN_INA219_BUS_CURRENT, &current);

		printf("Shunt: %f [V] -- "
			"Bus: %f [V] -- "
			"Power: %f [W] -- "
			"Current: %f [A]\n",
		       sensor_value_to_double(&v_shunt),
		       sensor_value_to_double(&v_bus),
		       sensor_value_to_double(&power),
		       sensor_value_to_double(&current));
		k_sleep(K_MSEC(2000));
	}
}
