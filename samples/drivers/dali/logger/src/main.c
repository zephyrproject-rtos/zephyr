/*
 * Copyright (c) 2025 by Markus Becker <markushx@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dali.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(D, LOG_LEVEL_INF);

static void dali_rx_callback(const struct device *dev, struct dali_frame frame, void *user_data)
{
	uint8_t length = 0x80;

	switch (frame.event_type) {
	case DALI_FRAME_CORRUPT:
		break;
	case DALI_FRAME_BACKWARD:
		length = 8;
		break;
	case DALI_FRAME_GEAR:
		length = 16;
		break;
	case DALI_FRAME_DEVICE:
		length = 24;
		break;
	case DALI_FRAME_FIRMWARE:
		length = 32;
		break;
	case DALI_EVENT_BUS_FAILURE:
		length = 0x91;
		break;
	case DALI_EVENT_BUS_IDLE:
		length = 0x90;
		break;
	}
	LOG_INF("{%08llx:%02x %08x}", (unsigned long long)k_uptime_get(), length, frame.data);
}

#define DALI_NODE DT_NODELABEL(dali0)
#define LED0_NODE DT_ALIAS(led0)

int main(void)
{
	int ret;

	/* set up DALI device */
	const struct device *dali_dev = DEVICE_DT_GET(DALI_NODE);

	if (!device_is_ready(dali_dev)) {
		LOG_ERR("Failed to get DALI device.");
		return 0;
	}

	ret = dali_set_receive_callback(dali_dev, dali_rx_callback, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to set DALI receive callback (%d).", ret);
		return 0;
	}

	/* set up LED */
	const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("Failed to get LED device.");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure LED device (%d).", ret);
		return 0;
	}

	for (;;) {
		k_sleep(K_MSEC(1000));
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			LOG_ERR("Failed to toggle LED (%d).", ret);
			return 0;
		}
	}
	return 0;
}
