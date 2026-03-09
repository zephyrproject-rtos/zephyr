/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#define STRIP_NODE DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)
#define STRIP_NUM_COLORS DT_PROP_LEN(DT_ALIAS(led_strip), color_mapping)
#else
#error Unable to determine length of LED strip
#endif

#define UPDATE_DELAY K_MSEC(CONFIG_SAMPLE_LED_UPDATE_DELAY)
#define TYPE_DELAY   K_MSEC(CONFIG_SAMPLE_LED_TYPE_DELAY)

#define RGB(_r, _g, _b)  {.r = (_r), .g = (_g), .b = (_b)}
#define ARRAY_ITEM(i, _) i

static const struct led_rgb rgb_colors[] = {
	RGB(CONFIG_SAMPLE_LED_BRIGHTNESS, 0x00, 0x00), /* red */
	RGB(0x00, CONFIG_SAMPLE_LED_BRIGHTNESS, 0x00), /* green */
	RGB(0x00, 0x00, CONFIG_SAMPLE_LED_BRIGHTNESS), /* blue */
};

static struct led_rgb pixels[STRIP_NUM_PIXELS];

static uint8_t led_colors[STRIP_NUM_COLORS] = {LISTIFY(STRIP_NUM_COLORS, ARRAY_ITEM, (,))};

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

static void set_rbg_colors(void)
{
	int rc;

	for (size_t i = 0; i < ARRAY_SIZE(rgb_colors); ++i) {
		for (size_t cursor = 0; cursor < ARRAY_SIZE(pixels); cursor++) {
			memset(&pixels, 0x00, sizeof(pixels));
			memcpy(&pixels[cursor], &rgb_colors[i], sizeof(struct led_rgb));

			rc = led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
			if (rc) {
				LOG_ERR("couldn't update strip: %d", rc);
			}

			k_sleep(UPDATE_DELAY);
		}
	}
}

static void try_map_colors(uint8_t colors[STRIP_NUM_COLORS])
{
	const uint8_t *mapping;
	int len;
	uint8_t *color = colors;

	len = led_strip_color_mapping(strip, &mapping);
	if (len == -ENOTSUP) {
		LOG_INF("Map colors unsupported");
		return;
	} else if (len < 0) {
		LOG_ERR("Get mapping failed: %d", len);
		return;
	} else if (len != STRIP_NUM_COLORS) {
		LOG_ERR("Mapping and STRIP_NUM_COLORS mismatch");
		return;
	}

	for (size_t id = 0; id < LED_COLOR_ID_MAX; ++id) {
		for (uint8_t i = 0; i < len; ++i) {
			if (id == mapping[i]) {
				*color++ = i;
			}
		}
	}

	LOG_HEXDUMP_INF(colors, STRIP_NUM_COLORS, "Mapped colors");
}

static void set_led_colors(const uint8_t colors[STRIP_NUM_COLORS])
{
	uint8_t channels[STRIP_NUM_PIXELS * STRIP_NUM_COLORS] = {0};
	size_t channel;
	int rc;

	for (size_t color = 0; color < STRIP_NUM_COLORS; ++color) {
		channel = colors[color];

		for (size_t pixel = 0; pixel < STRIP_NUM_PIXELS; ++pixel) {
			channels[channel] = CONFIG_SAMPLE_LED_BRIGHTNESS;

			rc = led_strip_update_channels(strip, channels, sizeof(channels));
			if (rc == -ENOTSUP) {
				return;
			} else if (rc < 0) {
				LOG_ERR("Udate strip channel %u failed: %d", channel, rc);
				return;
			}

			channels[channel] = 0;

			channel += STRIP_NUM_COLORS;
			k_sleep(UPDATE_DELAY);
		}
	}
}

int main(void)
{
	if (device_is_ready(strip)) {
		LOG_INF("Found LED strip device %s", strip->name);
	} else {
		LOG_ERR("LED strip device %s is not ready", strip->name);
		return 0;
	}

	LOG_INF("Displaying pattern on strip");

	try_map_colors(led_colors);

	while (1) {
		set_rbg_colors();
		k_sleep(TYPE_DELAY);
		set_led_colors(led_colors);
		k_sleep(TYPE_DELAY);
	}

	return 0;
}
