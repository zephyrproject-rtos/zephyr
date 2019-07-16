/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <display/cfb.h>
#include <sensor.h>
#include <version.h>
#include <logging/log.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "display.h"
#include "sensors.h"

LOG_MODULE_REGISTER(app_display, LOG_LEVEL_INF);

static u8_t screen_id = SCREEN_BOOT;
static struct device *epd_dev;
static char str_buf[280];

struct font_info {
	u8_t columns;
} fonts[] = {
	[FONT_BIG] =    { .columns = 12 },
	[FONT_MEDIUM] = { .columns = 16 },
	[FONT_SMALL] =  { .columns = 25 },
};


static size_t get_len(enum font_size font, const char *text)
{
	const char *space = NULL;
	s32_t i;

	for (i = 0; i <= fonts[font].columns; i++) {
		switch (text[i]) {
		case '\n':
		case '\0':
			return i;
		case ' ':
			space = &text[i];
			break;
		default:
			continue;
		}
	}

	/* If we got more characters than fits a line, and a space was
	 * encountered, fall back to the last space.
	 */
	if (space) {
		return space - text;
	}

	return fonts[font].columns;
}

static size_t print_line(enum font_size font_size, int row, const char *text,
			 size_t len, bool center)
{
	u8_t font_height, font_width;
	u8_t line[fonts[FONT_SMALL].columns + 1];
	int pad;

	cfb_framebuffer_set_font(epd_dev, font_size);

	len = MIN(len, fonts[font_size].columns);
	memcpy(line, text, len);
	line[len] = '\0';

	if (center) {
		pad = (fonts[font_size].columns - len) / 2;
	} else {
		pad = 0;
	}

	cfb_get_font_size(epd_dev, font_size, &font_width, &font_height);

	if (cfb_print(epd_dev, line, font_width * pad, font_height * row)) {
		LOG_ERR("Failed to print a string");
	}

	return len;
}

void board_show_text(enum font_size font, const char *text, bool center)
{
	int i;

	cfb_framebuffer_clear(epd_dev, false);

	for (i = 0; i < 3; i++) {
		size_t len;

		while (*text == ' ' || *text == '\n') {
			text++;
		}

		len = get_len(font, text);
		if (!len) {
			break;
		}

		text += print_line(font, i, text, len, center);
		if (!*text) {
			break;
		}
	}

	cfb_framebuffer_finalize(epd_dev);
}

static void show_boot_banner(void)
{
	snprintf(str_buf, sizeof(str_buf), "Booting Zephyr\n"
		"OS Version:\n%s",
		 KERNEL_VERSION_STRING);

	display_medium(str_buf, false);
}

static void show_sensors_data(void)
{
	static s32_t old_external_tmp = INVALID_SENSOR_VALUE;
	static s32_t old_tmp = INVALID_SENSOR_VALUE;
	static s32_t old_hum = INVALID_SENSOR_VALUE;

	s32_t external_tmp = sensors_get_temperature_external();
	s32_t temperature = sensors_get_temperature();
	s32_t humidity = sensors_get_humidity();

	u16_t len = 0U;
	u16_t len2 = 0U;

	if ((temperature == old_tmp) && (humidity == old_hum) &&
	    (old_external_tmp == external_tmp)) {
		return;
	}

	old_external_tmp = external_tmp;
	old_tmp = temperature;
	old_hum = humidity;

	if (temperature != INVALID_SENSOR_VALUE) {
		len = snprintf(str_buf, sizeof(str_buf),
				"Home: %3d.%1dC\n",
				temperature / 10, abs(temperature % 10));
	} else {
		len = snprintf(str_buf, sizeof(str_buf),
				"Home:    N/A\n");
	}

	if (humidity != INVALID_SENSOR_VALUE) {
		len2 = snprintf(str_buf + len, sizeof(str_buf) - len,
				"Home: %3d%% H\n",
				humidity);
	} else {
		len2 = snprintf(str_buf + len, sizeof(str_buf) - len,
				"Home:    N/A\n");
	}

	if (old_external_tmp != INVALID_SENSOR_VALUE) {
		len = snprintf(str_buf + len + len2,
				sizeof(str_buf) - len - len2,
				"Out: %c%3d.%1dC\n",
				external_tmp >= 0 ? ' ' : '-',
				abs(external_tmp / 10),
				abs(external_tmp % 10));
	} else {
		len = snprintf(str_buf + len + len2,
				sizeof(str_buf) - len - len2,
				"Out:     N/A");
	}

	display_big(str_buf, false);
}

int display_init(void)
{
	epd_dev = device_get_binding(DT_INST_0_SOLOMON_SSD16XXFB_LABEL);
	if (epd_dev == NULL) {
		LOG_ERR("SSD1673 device not found");
		return -ENODEV;
	}

	if (cfb_framebuffer_init(epd_dev)) {
		LOG_ERR("Framebuffer initialization failed");
		return -EIO;
	}

	cfb_framebuffer_clear(epd_dev, true);

	return 0;
}

int display_screen(enum screen_ids id)
{
	switch (id) {
	case SCREEN_BOOT:
		show_boot_banner();
		break;
	case SCREEN_SENSORS:
		show_sensors_data();
		break;
	default:
		LOG_ERR("fatal display error");
		return -EINVAL;
	}

	return 0;
}

enum screen_ids display_screen_get(void)
{
	return screen_id;
}
