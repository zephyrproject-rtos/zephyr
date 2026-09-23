/*
 * Copyright (c) 2026 Simon Guinot <simon.guinot@sequanux.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_led_strip_cache

/**
 * @file
 * @brief LED driver bridging the LED and LED strip APIs.
 *
 * The led-strip-cache driver exposes a virtual LED device composed of one or
 * more LED strip devices. It bridges the LED and LED strip APIs and provides
 * a cache for all pixels of the underlying strips. It allows users to configure
 * individual LEDs via the ->set_color() method of the LED API.
 *
 * If multiple LED strips are specified, they are concatenated in phandle
 * order in the same LED device.
 *
 * led-strips = <&led_strip_0>, <&led_strip_1>;
 *
 *         strip 0 (M pixels)   strip 1 (N pixels)
 *        /                  \ /                 \
 * LEDs = [ 0    ...     M-1 ] [ M   ...   M+N-1 ]
 *
 * When an LED is updated, the corresponding pixel in the cache is modified
 * and the cached pixels are passed to the underlying LED strip device.
 *
 * The cache uses one struct led_rgb per LED, so its memory footprint is
 * three bytes per LED (four if CONFIG_LED_STRIP_RGB_SCRATCH is enabled).
 */

#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_strip_cache, CONFIG_LED_LOG_LEVEL);

struct led_strip_cache {
	const struct device *dev;
	struct led_rgb *pixels;
};

struct led_strip_cache_config {
	uint8_t num_strips;
	const struct led_strip_cache *strip;
};

static int led_strip_cache_set_color(const struct device *dev, uint32_t led,
				     uint8_t num_colors, const uint8_t *color)
{
	const struct led_strip_cache_config *config = dev->config;

	if (num_colors != 3) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < config->num_strips; i++) {
		const struct led_strip_cache *strip = &config->strip[i];
		size_t length = led_strip_length(strip->dev);
		struct led_rgb *pixels;

		if (led < length) {
			pixels = strip->pixels;
			pixels[led].r = color[0];
			pixels[led].g = color[1];
			pixels[led].b = color[2];

			return led_strip_update_rgb(strip->dev, pixels, led + 1);
		}
		led -= length;
	}

	return -EINVAL;
}

static int led_strip_cache_init(const struct device *dev)
{
	const struct led_strip_cache_config *config = dev->config;

	for (uint8_t i = 0; i < config->num_strips; i++) {
		const struct led_strip_cache *strip = &config->strip[i];

		if (!device_is_ready(strip->dev)) {
			LOG_ERR_DEVICE_NOT_READY(strip->dev);
			return -ENODEV;
		}
	}

	return 0;
}

static DEVICE_API(led, led_strip_cache_api) = {
	.set_color = led_strip_cache_set_color,
};

#define LED_STRIP_LENGTH(node_id, prop, idx)					\
	DT_PROP(DT_PHANDLE_BY_IDX(node_id, prop, idx), chain_length)

#define LED_STRIP_PIXELS(node_id, prop, idx)					\
	struct led_rgb pixels##node_id##idx[LED_STRIP_LENGTH(node_id, prop, idx)];

#define LED_STRIP(node_id, prop, idx)						\
	{									\
		.dev = DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),	\
		.pixels	= pixels##node_id##idx,					\
	}

#define LED_STRIP_CACHE_DEVICE(inst)						\
										\
	DT_INST_FOREACH_PROP_ELEM(inst, led_strips, LED_STRIP_PIXELS)		\
										\
	static const struct led_strip_cache led_strip_cache_##inst[] = {	\
		DT_INST_FOREACH_PROP_ELEM_SEP(					\
			inst, led_strips, LED_STRIP, (,))			\
	};									\
										\
	static const struct led_strip_cache_config				\
				led_strip_cache_config_##inst = {		\
		.num_strips = ARRAY_SIZE(led_strip_cache_##inst),		\
		.strip = led_strip_cache_##inst,				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, &led_strip_cache_init, NULL,		\
			      NULL, &led_strip_cache_config_##inst,		\
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,		\
			      &led_strip_cache_api);

DT_INST_FOREACH_STATUS_OKAY(LED_STRIP_CACHE_DEVICE)
