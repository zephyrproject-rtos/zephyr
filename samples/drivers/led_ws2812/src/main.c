/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr.h>
#include <drivers/led_strip.h>
#include <device.h>
#include <drivers/spi.h>
#include <sys/util.h>

/*
 * Number of RGB LEDs in the LED strip, adjust as needed.
 */
#if defined(CONFIG_WS2812_STRIP)
#define STRIP_NUM_LEDS 12
#define STRIP_DEV_NAME DT_INST_0_WORLDSEMI_WS2812_LABEL
#else
#define STRIP_NUM_LEDS 24
#define STRIP_DEV_NAME CONFIG_WS2812B_SW_NAME
#endif

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

void main(void)
{
	struct device *strip;
	size_t i, time;

	strip = device_get_binding(STRIP_DEV_NAME);
	if (strip) {
		LOG_INF("Found LED strip device %s", STRIP_DEV_NAME);
	} else {
		LOG_ERR("LED strip device %s not found", STRIP_DEV_NAME);
		return;
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
