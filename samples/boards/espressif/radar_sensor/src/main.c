/*
 * Copyright (c) 2026 NotioNext LTD.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static void radar_trigger_handler(const struct device *dev,
				   const struct sensor_trigger *trig)
{
	struct sensor_value val;

	if (sensor_sample_fetch(dev) < 0) {
		LOG_ERR("Failed to fetch sample");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_PROX, &val) < 0) {
		LOG_ERR("Failed to get channel");
		return;
	}

	if (val.val1) {
		LOG_INF(">>> MOTION DETECTED!");
	} else {
		LOG_INF("<<< Area Clear");
	}
}

int main(void)
{
	const struct device *radar_dev = DEVICE_DT_GET(DT_NODELABEL(at581x));

	if (!device_is_ready(radar_dev)) {
		LOG_ERR("AT581X radar sensor not ready");
		return -1;
	}

	static const struct sensor_trigger trig = {
		.type = SENSOR_TRIG_MOTION,
		.chan = SENSOR_CHAN_PROX,
	};

	if (sensor_trigger_set(radar_dev, &trig, radar_trigger_handler) < 0) {
		LOG_ERR("Failed to set trigger");
		return -1;
	}

	LOG_INF("AT581X radar sensor initialized, waiting for motion...");

	return 0;
}