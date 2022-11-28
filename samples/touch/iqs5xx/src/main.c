/*
 * Copyright (C) 2022 Ryan Walker <info@interruptlabs.ca>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/touch.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <esp_event.h>
#include <esp_timer.h>

#define SLEEP_TIME_MS 10

LOG_MODULE_REGISTER(iqs5xx_sample, LOG_LEVEL_DBG);

void main(void)
{
	const struct device *trackpad = DEVICE_DT_GET(DT_NODELABEL(iqs5xx));

	if (!device_is_ready(trackpad)) {
		LOG_ERR("Trackpad Not Ready!");
		return;
	}

	while (true) {
		if (num_fingers_get(trackpad)) {
			int16_t x_pos = x_pos_get_abs(trackpad);
			int16_t y_pos = y_pos_get_abs(trackpad);

			LOG_INF("x: %d, y: %d", x_pos, y_pos);
		}
		k_msleep(SLEEP_TIME_MS);
	}
}
