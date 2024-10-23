/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr_ws2812.h"
#include <string.h>
#include <stdarg.h>

/* Public APIs */

bool ws2812_led_init(struct ws2812_led_data *ws2812_led)
{
	bool result = true;

	do {
		if (!ws2812_led->led_len) {
			result = false;
			break;
		}
		/* check if ws2812 is ready */
		if (!device_is_ready(ws2812_led->led)) {
			result = false;
			break;
		}
		/* init all LEDs ready */
		memset(ws2812_led->pix, 0x00, sizeof(struct led_rgb) * ws2812_led->led_len);
		memcpy(ws2812_led->aux, ws2812_led->pix,
			sizeof(struct led_rgb) * ws2812_led->led_len);
		if (led_strip_update_rgb(ws2812_led->led,
			(struct led_rgb *) ws2812_led->aux, ws2812_led->led_len)) {
			result = false;
			break;
		}
		/* all done */
	} while (0);

	return result;
}

bool ws2812_led_set(struct ws2812_led_data *ws2812_led,
	size_t id, enum ws2812_led_state state, ...)
{
	bool result = false;

	if (id < ws2812_led->led_len * 3) {
		if (state == WS2812_LED_ON || state == WS2812_LED_OFF) {
			switch (id % 3) {
			case 0:
				ws2812_led->pix[id / 3].r = (state ? 0xff : 0);
				break;
			case 1:
				ws2812_led->pix[id / 3].g = (state ? 0xff : 0);
				break;
			case 2:
				ws2812_led->pix[id / 3].b = (state ? 0xff : 0);
				break;
			}
			memcpy(ws2812_led->aux, ws2812_led->pix,
				sizeof(struct led_rgb) * ws2812_led->led_len);
			if (!led_strip_update_rgb(ws2812_led->led,
				(struct led_rgb *) ws2812_led->aux, ws2812_led->led_len)) {
				result = true;
			}
		} else if (state == WS2812_LED_FIXED) {
			va_list argptr;

			va_start(argptr, state);
			uint32_t permille = va_arg(argptr, uint32_t);

			va_end(argptr);
			if (permille <= PERMILLE_MAX) {
				switch (id % 3) {
				case 0:
					ws2812_led->pix[id / 3].r =
						(permille * 0xff + PERMILLE_MAX / 2) / PERMILLE_MAX;
					break;
				case 1:
					ws2812_led->pix[id / 3].g =
						(permille * 0xff + PERMILLE_MAX / 2) / PERMILLE_MAX;
					break;
				case 2:
					ws2812_led->pix[id / 3].b =
						(permille * 0xff + PERMILLE_MAX / 2) / PERMILLE_MAX;
					break;
				}
				memcpy(ws2812_led->aux, ws2812_led->pix,
					sizeof(struct led_rgb) * ws2812_led->led_len);
				if (!led_strip_update_rgb(ws2812_led->led,
					(struct led_rgb *) ws2812_led->aux, ws2812_led->led_len)) {
					result = true;
				}
			}
		}
	}
	return result;
}
