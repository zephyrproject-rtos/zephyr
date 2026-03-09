/*
 * Copyright (c) 2026 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(main);

#include <zephyr/dt-bindings/led/led.h>
#include <zephyr/drivers/led_strip.h>

#include "common.h"

#define STRIP_NUM_PIXEL_CHANNELS DT_PROP_LEN(STRIP_NODE, color_mapping)
#define STRIP_NUM_CHANNELS       (STRIP_NUM_PIXELS * STRIP_NUM_PIXEL_CHANNELS)

/* Initialize unordered from 0 to (STRIP_NUM_PIXEL_CHANNELS - 1) */
#define ARRAY_ITEM(i, _) i
static uint8_t pixel_channels[STRIP_NUM_PIXEL_CHANNELS] = {
	LISTIFY(STRIP_NUM_PIXEL_CHANNELS, ARRAY_ITEM, (,))};

void try_map_channels(void)
{
	const uint8_t *mapping;
	int len;
	uint8_t *channel = pixel_channels;

	len = led_strip_color_mapping(strip, &mapping);
	if (len == -ENOTSUP) {
		LOG_INF("Map colors unsupported");
		return;
	} else if (len < 0) {
		LOG_ERR("Get mapping failed: %d", len);
		return;
	} else if (len != STRIP_NUM_PIXEL_CHANNELS) {
		LOG_ERR("Mapping and STRIP_NUM_PIXEL_CHANNELS mismatch");
		return;
	}

	for (size_t id = 0; id < LED_COLOR_ID_MAX; ++id) {
		for (uint8_t i = 0; i < len; ++i) {
			if (id == mapping[i]) {
				*channel++ = i;
			}
		}
	}

	LOG_HEXDUMP_INF(pixel_channels, STRIP_NUM_PIXEL_CHANNELS, "Mapped channels");
}

void set_channels(void)
{
	const size_t last_pixel_offset = STRIP_NUM_CHANNELS - STRIP_NUM_PIXEL_CHANNELS;
	uint8_t channels[STRIP_NUM_CHANNELS] = {0};
	size_t channel;
	int rc;

	for (size_t pixel_channel = 0; pixel_channel < STRIP_NUM_PIXEL_CHANNELS; ++pixel_channel) {
		channel = pixel_channels[pixel_channel] + last_pixel_offset;

		for (size_t pixel = 0; pixel < STRIP_NUM_PIXELS; ++pixel) {
			channels[channel] = CONFIG_SAMPLE_LED_BRIGHTNESS;

			rc = led_strip_update_channels(strip, channels, sizeof(channels));
			if (rc == -ENOTSUP) {
				return;
			} else if (rc < 0) {
				LOG_ERR("Update strip channel %u failed: %d", channel, rc);
				return;
			}

			channels[channel] = 0;

			channel -= STRIP_NUM_PIXEL_CHANNELS;
			k_sleep(UPDATE_DELAY);
		}
	}
}
