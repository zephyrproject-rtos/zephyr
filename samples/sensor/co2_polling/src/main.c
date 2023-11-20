/*
 * Copyright (c) 2023, Vitrolife A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MAIN);

int main(void)
{
	struct sensor_value value;
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(co2));

	if (!device_is_ready(dev)) {
		LOG_ERR("%s is not ready", dev->name);
		return 0;
	}

	while (1) {
		if (sensor_sample_fetch(dev) == 0 &&
		    sensor_channel_get(dev, SENSOR_CHAN_CO2, &value) == 0) {
			LOG_INF("CO2 %d ppm", value.val1);
		}

		k_msleep(1000);
	}

	return 0;
}
