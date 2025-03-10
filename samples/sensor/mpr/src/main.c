/*
 * Copyright (c) 2020 Sven Herrmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(honeywell_mpr);
	int rc;

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return 0;
	}

	while (1) {
		struct sensor_value pressure;
		int8_t shift;
		q31_t pressure_value;
		struct sensor_chan_spec channels[1] = {
			{.chan_type = SENSOR_CHAN_PRESS, .chan_idx = 0},
		};

		rc = sensor_read_and_decode(dev, channels, ARRAY_SIZE(channels), &shift,
					    &pressure_value, 1);
		if (rc != 0) {
			printf("sensor_read_and_decode error: %d\n", rc);
			break;
		}
		printf("pressure (q31): %" PRIq(6) " kPa\n", PRIq_arg(pressure_value, 6, shift));

		q31_to_sensor_value(pressure_value, shift, &pressure);
		printf("pressure (sensor_value): %u.%u kPa\n", pressure.val1, pressure.val2);

		k_sleep(K_SECONDS(1));
	}
	return 0;
}
