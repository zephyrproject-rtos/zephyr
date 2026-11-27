/*
 * Copyright (c) 2026 Kristina Kisiuk, Aliensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define NUM_COLORS 3
#define RGB_LED    0

static const struct {
	const char *name;
	uint8_t rgb[NUM_COLORS];
} colors[] = {
	{"red", {0xff, 0x00, 0x00}},
	{"green", {0x00, 0xff, 0x00}},
	{"blue", {0x00, 0x00, 0xff}},
	{"white", {0xff, 0xff, 0xff}},
};

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(ti_lp5815);
	int ret;

	if (!device_is_ready(dev)) {
		LOG_ERR("LED device %s is not ready", dev->name);
		return 0;
	}

	ARRAY_FOR_EACH(colors, i) {
		LOG_INF("Color: %s", colors[i].name);

		ret = led_set_color(dev, RGB_LED, NUM_COLORS, colors[i].rgb);
		if (ret < 0) {
			LOG_ERR("Failed to set the color: %d", ret);
			return 0;
		}

		k_sleep(K_SECONDS(2));
	}

	LOG_INF("Blinking: 500 ms on, 500 ms off");

	ret = led_blink(dev, RGB_LED, 500, 500);
	if (ret < 0) {
		LOG_ERR("Failed to blink: %d", ret);
		return 0;
	}

	k_sleep(K_SECONDS(5));

	/* A blink with no on time ends the blinking and leaves the LED off */
	LOG_INF("Off");

	ret = led_blink(dev, RGB_LED, 0, 0);
	if (ret < 0) {
		LOG_ERR("Failed to switch the LED off: %d", ret);
	}

	return 0;
}
