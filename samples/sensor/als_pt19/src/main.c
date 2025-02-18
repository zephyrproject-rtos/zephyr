/*
 * Copyright (c) 2025 Dipak Shetty <shetty.dipak@gmx.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(als_pt19_sample, LOG_LEVEL_DBG);

#define SLEEP_TIME K_MSEC(1000)

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(everlight_als_pt19);

	if (!device_is_ready(dev)) {
		LOG_ERR("sensor: device not ready.");
		return 0;
	}

	while (1) {
		int read;
		struct sensor_value lux;

		read = sensor_sample_fetch(dev);
		if (read) {
			LOG_ERR("sample fetch error %d", read);
			continue;
		}

		sensor_channel_get(dev, SENSOR_CHAN_LIGHT, &lux);

		LOG_INF("Illuminance reading: %.3f", sensor_value_to_double(&lux));

		k_sleep(SLEEP_TIME);
	}
	return 0;
}
