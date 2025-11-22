/*
 * Copyright (c) 2020 George Gkinis
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/hx7xx.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#define CALIBRATION_WEIGHT 500.0

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

const struct device *hx711;

void set_rate(enum hx7xx_rate rate)
{
	static struct sensor_value rate_val;

	rate_val.val1 = rate;
	sensor_attr_set(hx711, SENSOR_CHAN_MASS, SENSOR_ATTR_SAMPLING_FREQUENCY, &rate_val);
}

void measure(void)
{
	static struct sensor_value weight;
	int ret;

	ret = sensor_sample_fetch(hx711);
	if (ret != 0) {
		LOG_ERR("Cannot take measurement: %d  ", ret);
	} else {
		sensor_channel_get(hx711, SENSOR_CHAN_MASS, &weight);
		LOG_INF("weight: %f", sensor_value_to_double(&weight));
	}
}

int main(void)
{
	hx711 = DEVICE_DT_GET(DT_ALIAS(hx711_sensor));

	__ASSERT(hx711 == NULL, "Failed to get device binding");

	if (!device_is_ready(hx711)) {
		LOG_ERR("Device is not ready");
		return -EBUSY;
	}

	LOG_INF("============= start =============");
	LOG_INF("device is %p, name is %s", hx711, hx711->name);

	LOG_INF("Calculating offset...");
	avia_hx7xx_tare(hx711, 15);

	LOG_INF("waiting for known weight of %f grams...", CALIBRATION_WEIGHT);

	for (int i = 5; i >= 0; i--) {
		LOG_INF(" %d..", i);
		k_msleep(1000);
	}

	LOG_INF("Calculating slope...");
	avia_hx7xx_calibrate(hx711, 15, CALIBRATION_WEIGHT);

	LOG_INF("== Test measure ==");
	while (true) {
		measure();
		k_msleep(1000);
	}

	return 0;
}
