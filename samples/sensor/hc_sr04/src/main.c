/*
 * Copyright (c) 2026 Javad Rahimipetroudi <javad321javad@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hc_sr04_sample, CONFIG_LOG_DEFAULT_LEVEL);

#if DT_NODE_HAS_STATUS(DT_INST(0, hc_sr04), okay)
#define HC_SR04 DT_NODELABEL(hc_sr04)
#else
#error "Unsupported board:No HC-SR04 node found in the device tree"
#endif

#define SLEEP_MS 1000

int main(void)
{
	int ret = 0;

	const struct device *const hc_sr04_dev = DEVICE_DT_GET_ONE(hc_sr04);

	if (!device_is_ready(hc_sr04_dev)) {
		LOG_ERR("HC-SR04 module is not ready to use.");
		return 0;
	}

	struct sensor_value value = {0};

	while (1) {
		ret = sensor_sample_fetch(hc_sr04_dev);
		if (ret != 0) {
			LOG_ERR("Unable to fetch data from sensor (%d)", ret);
		}
		ret = sensor_channel_get(hc_sr04_dev, SENSOR_CHAN_DISTANCE, &value);

		if (ret != 0) {
			LOG_ERR("Unable to get channel data (%d)\n", ret);
		}

		LOG_INF("Distance: %lld cm", sensor_value_to_centi(&value));
		k_msleep(SLEEP_MS);
	}
	return 0;
}
