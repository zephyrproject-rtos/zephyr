/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

/*
 * Number of RGB LEDs in the LED strip, adjust as needed.
 */
#define STRIP_NUM_LEDS 20

#define DELAY_TIME K_MSEC(40)

static const struct led_rgb colors[] = {
	{ .r = 0xff, .g = 0x00, .b = 0x00, }, /* red */
	{ .r = 0x00, .g = 0xff, .b = 0x00, }, /* green */
	{ .r = 0x00, .g = 0x00, .b = 0xff, }, /* blue */
};

static const struct led_rgb black = {
	.r = 0x00,
	.g = 0x00,
	.b = 0x00,
};

struct led_rgb strip_colors[STRIP_NUM_LEDS];

const struct led_rgb *color_at(size_t time, size_t i)
{
	size_t rgb_start = time % STRIP_NUM_LEDS;

	if (rgb_start <= i && i < rgb_start + ARRAY_SIZE(colors)) {
		return &colors[i - rgb_start];
	} else {
		return &black;
	}
}

#define DELAY_TIME K_MSEC(40)
void main(void)
{
	const struct device *strip;
	size_t i, time;

	strip = DEVICE_DT_GET_ANY(apa_apa102);
	if (!strip) {
		LOG_ERR("LED strip device not found");
		return;
	} else if (!device_is_ready(strip)) {
		LOG_ERR("LED strip device %s is not ready", strip->name);
		return;
	} else {
		LOG_INF("Found LED strip device %s", strip->name);
	}

	/*
	 * Display a pattern that "walks" the three primary colors
	 * down the strip until it reaches the end, then starts at the
	 * beginning. This has the effect of moving it around in a
	 * circle in the case of rings of pixels.
	 */
	LOG_INF("Displaying pattern on strip");
	time = 0;
	while (1) {
		for (i = 0; i < STRIP_NUM_LEDS; i++) {
			memcpy(&strip_colors[i], color_at(time, i),
			       sizeof(strip_colors[i]));
		}
		led_strip_update_rgb(strip, strip_colors, STRIP_NUM_LEDS);
		k_sleep(DELAY_TIME);
		time++;
	}
}
