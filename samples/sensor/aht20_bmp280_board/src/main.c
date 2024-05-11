/*
 * Copyright (c) 2023 Russ Webber
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app);

void main(void)
{
	int ret;
	const struct device *ht_sensor;
	const struct device *pressure_sensor;

	LOG_INF("ATH20+BMP280 board sample started");

	ht_sensor = DEVICE_DT_GET(DT_NODELABEL(ht_sensor));
	if (!device_is_ready(ht_sensor)) {
		LOG_ERR("Humidity and Temp sensor %s not ready", ht_sensor->name);
		return;
	}

	pressure_sensor = DEVICE_DT_GET(DT_NODELABEL(pressure_sensor));
	if (!device_is_ready(pressure_sensor)) {
		LOG_ERR("Pressure sensor %s not ready", pressure_sensor->name);
		return;
	}

	while (1) {
		struct sensor_value val;

		ret = sensor_sample_fetch(ht_sensor);
		if (ret < 0) {
			LOG_ERR("%s - could not fetch sample (%d)", ht_sensor->name, ret);
			return;
		}

		ret = sensor_sample_fetch(pressure_sensor);
		if (ret < 0) {
			LOG_ERR("%s - could not fetch sample (%d)", pressure_sensor->name, ret);
			return;
		}

		ret = sensor_channel_get(ht_sensor, SENSOR_CHAN_HUMIDITY, &val);
		if (ret < 0) {
			LOG_ERR("Could not get SENSOR_CHAN_HUMIDITY channel (%d)", ret);
		} else {
			LOG_INF("%s: Relative Humidity (%%): %d.%06d", ht_sensor->name, val.val1, val.val2);
		}

		ret = sensor_channel_get(ht_sensor, SENSOR_CHAN_AMBIENT_TEMP, &val);
		if (ret < 0) {
			LOG_ERR("Could not get SENSOR_CHAN_AMBIENT_TEMP channel (%d)", ret);
		} else {
			LOG_INF("%s: Temperature (C): %d.%06d", ht_sensor->name, val.val1, val.val2);
		}

		ret = sensor_channel_get(pressure_sensor, SENSOR_CHAN_PRESS, &val);
		if (ret < 0) {
			LOG_ERR("Could not get SENSOR_CHAN_PRESS channel (%d)", ret);
		} else {
			LOG_INF("%s: Pressure (kPa): %d.%06d", pressure_sensor->name, val.val1, val.val2);
		}

		ret = sensor_channel_get(pressure_sensor, SENSOR_CHAN_AMBIENT_TEMP, &val);
		if (ret < 0) {
			LOG_ERR("Could not get SENSOR_CHAN_AMBIENT_TEMP channel (%d)", ret);
		} else {
			LOG_INF("%s: Temperature (C): %d.%06d", pressure_sensor->name, val.val1, val.val2);
		}

		k_msleep(2000);
	}
}

