/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/device.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MAIN);

struct pms7003_readings {
	struct sensor_value pm_1_0_cf;
	struct sensor_value pm_2_5_cf;
	struct sensor_value pm_10_cf;
	struct sensor_value pm_1_0_atm;
	struct sensor_value pm_2_5_atm;
	struct sensor_value pm_10_0_atm;
	struct sensor_value pm_0_3_count;
	struct sensor_value pm_0_5_count;
	struct sensor_value pm_1_0_count;
	struct sensor_value pm_2_5_count;
	struct sensor_value pm_5_0_count;
	struct sensor_value pm_10_0_count;
};

int main()
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(pmsx003));

	if (!device_is_ready(dev)) {
		LOG_ERR("%s is not ready", dev->name);
		return 0;
	}

	while (1) {

		struct pms7003_readings readings;

		sensor_sample_fetch(dev);

		sensor_channel_get(dev, SENSOR_CHAN_PM_1_0_CF, &readings.pm_1_0_cf);
		sensor_channel_get(dev, SENSOR_CHAN_PM_2_5_CF, &readings.pm_2_5_cf);
		sensor_channel_get(dev, SENSOR_CHAN_PM_10_CF, &readings.pm_10_cf);
		sensor_channel_get(dev, SENSOR_CHAN_PM_1_0, &readings.pm_1_0_atm);
		sensor_channel_get(dev, SENSOR_CHAN_PM_2_5, &readings.pm_2_5_atm);
		sensor_channel_get(dev, SENSOR_CHAN_PM_10, &readings.pm_10_0_atm);
		sensor_channel_get(dev, SENSOR_CHAN_PM_0_3_COUNT, &readings.pm_0_3_count);
		sensor_channel_get(dev, SENSOR_CHAN_PM_0_5_COUNT, &readings.pm_0_5_count);
		sensor_channel_get(dev, SENSOR_CHAN_PM_1_0_COUNT, &readings.pm_1_0_count);
		sensor_channel_get(dev, SENSOR_CHAN_PM_2_5_COUNT, &readings.pm_2_5_count);
		sensor_channel_get(dev, SENSOR_CHAN_PM_5_0_COUNT, &readings.pm_5_0_count);
		sensor_channel_get(dev, SENSOR_CHAN_PM_10_0_COUNT, &readings.pm_10_0_count);

		LOG_INF("pm1.0_cf = %d.%02d µg/m³", readings.pm_1_0_cf.val1, readings.pm_1_0_cf.val2);
		LOG_INF("pm2.5_cf = %d.%02d µg/m³", readings.pm_2_5_cf.val1, readings.pm_2_5_cf.val2);
		LOG_INF("pm10_cf = %d.%02d µg/m³", readings.pm_10_cf.val1, readings.pm_10_cf.val2);
		LOG_INF("pm1.0_atm = %d.%02d µg/m³", readings.pm_1_0_atm.val1, readings.pm_1_0_atm.val2);
		LOG_INF("pm2.5_atm = %d.%02d µg/m³", readings.pm_2_5_atm.val1, readings.pm_2_5_atm.val2);
		LOG_INF("pm10_atm = %d.%02d µg/m³", readings.pm_10_0_atm.val1, readings.pm_10_0_atm.val2);
		LOG_INF("pm0.3_count = %d particles/0.1L", readings.pm_0_3_count.val1);
		LOG_INF("pm0.5_count = %d particles/0.1L", readings.pm_0_5_count.val1);
		LOG_INF("pm1.0_count = %d particles/0.1L", readings.pm_1_0_count.val1);
		LOG_INF("pm2.5_count = %d particles/0.1L", readings.pm_2_5_count.val1);
		LOG_INF("pm5.0_count = %d particles/0.1L", readings.pm_5_0_count.val1);
		LOG_INF("pm10_count = %d particles/0.1L", readings.pm_10_0_count.val1);

		k_sleep(K_SECONDS(10));
	}
}
