/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * Based on ST7789V sample:
 * Copyright (c) 2019 Marc Reilly
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

#ifdef CONFIG_ARCH_POSIX
#include "posix_board_if.h"
#endif

enum corner {
	TOP_LEFT,
	TOP_RIGHT,
	BOTTOM_RIGHT,
	BOTTOM_LEFT
};

typedef void (*fill_buffer)(enum corner corner, uint8_t grey, uint8_t *buf,
			    size_t buf_size);


#ifdef CONFIG_ARCH_POSIX
static void posix_exit_main(int exit_code)
{
#if CONFIG_TEST
	if (exit_code == 0) {
		LOG_INF("PROJECT EXECUTION SUCCESSFUL");
	} else {
		LOG_INF("PROJECT EXECUTION FAILED");
	}
#endif
	posix_exit(exit_code);
}
#endif

static void fill_buffer_argb8888(enum corner corner, uint8_t grey, uint8_t *buf,
				 size_t buf_size)
{
	uint32_t color = 0;

	switch (corner) {
	case TOP_LEFT:
		color = 0x00FF0000u;
		break;
	case TOP_RIGHT:
		color = 0x0000FF00u;
		break;
	case BOTTOM_RIGHT:
		color = 0x000000FFu;
		break;
	case BOTTOM_LEFT:
		color = grey << 16 | grey << 8 | grey;
		break;
	}

	for (size_t idx = 0; idx < buf_size; idx += 4) {
		*((uint32_t *)(buf + idx)) = color;
	}
}

static void fill_buffer_rgb888(enum corner corner, uint8_t grey, uint8_t *buf,
			       size_t buf_size)
{
	uint32_t color = 0;

	switch (corner) {
	case TOP_LEFT:
		color = 0x00FF0000u;
		break;
	case TOP_RIGHT:
		color = 0x0000FF00u;
		break;
	case BOTTOM_RIGHT:
		color = 0x000000FFu;
		break;
	case BOTTOM_LEFT:
		color = grey << 16 | grey << 8 | grey;
		break;
	}

	for (size_t idx = 0; idx < buf_size; idx += 3) {
		*(buf + idx + 0) = color >> 16;
		*(buf + idx + 1) = color >> 8;
		*(buf + idx + 2) = color >> 0;
	}
}

static uint16_t get_rgb565_color(enum corner corner, uint8_t grey)
{
	uint16_t color = 0;
	uint16_t grey_5bit;

	switch (corner) {
	case TOP_LEFT:
		color = 0xF800u;
		break;
	case TOP_RIGHT:
		color = 0x07E0u;
		break;
	case BOTTOM_RIGHT:
		color = 0x001Fu;
		break;
	case BOTTOM_LEFT:
		grey_5bit = grey & 0x1Fu;
		/* shift the green an extra bit, it has 6 bits */
		color = grey_5bit << 11 | grey_5bit << (5 + 1) | grey_5bit;
		break;
	}
	return color;
}

static void fill_buffer_rgb565(enum corner corner, uint8_t grey, uint8_t *buf,
			       size_t buf_size)
{
	uint16_t color = get_rgb565_color(corner, grey);

	for (size_t idx = 0; idx < buf_size; idx += 2) {
		*(buf + idx + 0) = (color >> 8) & 0xFFu;
		*(buf + idx + 1) = (color >> 0) & 0xFFu;
	}
}

static void fill_buffer_bgr565(enum corner corner, uint8_t grey, uint8_t *buf,
			       size_t buf_size)
{
	uint16_t color = get_rgb565_color(corner, grey);

	for (size_t idx = 0; idx < buf_size; idx += 2) {
		*(uint16_t *)(buf + idx) = color;
	}
}

static void fill_buffer_mono(enum corner corner, uint8_t grey,
			     uint8_t black, uint8_t white,
			     uint8_t *buf, size_t buf_size)
{
	uint16_t color;

	switch (corner) {
	case BOTTOM_LEFT:
		color = (grey & 0x01u) ? white : black;
		break;
	default:
		color = black;
		break;
	}

	memset(buf, color, buf_size);
}

static inline void fill_buffer_mono01(enum corner corner, uint8_t grey,
				      uint8_t *buf, size_t buf_size)
{
	fill_buffer_mono(corner, grey, 0x00u, 0xFFu, buf, buf_size);
}

static inline void fill_buffer_mono10(enum corner corner, uint8_t grey,
				      uint8_t *buf, size_t buf_size)
{
	fill_buffer_mono(corner, grey, 0xFFu, 0x00u, buf, buf_size);
}

int main(void)
{
	size_t x;
	size_t y;
	size_t rect_w;
	size_t rect_h;
	size_t h_step;
	size_t scale;
	size_t grey_count;
	uint8_t bg_color;
	uint8_t *buf;
	int32_t grey_scale_sleep;
	const struct device *display_dev;
	struct display_capabilities capabilities;
	struct display_buffer_descriptor buf_desc;
	size_t buf_size = 0;
	fill_buffer fill_buffer_fnc = NULL;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device %s not found. Aborting sample.",
			display_dev->name);
#ifdef CONFIG_ARCH_POSIX
		posix_exit_main(1);
#else
		return 0;
#endif
	}

	LOG_INF("Display sample for %s", display_dev->name);
	display_get_capabilities(display_dev, &capabilities);

	if (capabilities.screen_info & SCREEN_INFO_MONO_VTILED) {
		rect_w = 16;
		rect_h = 8;
	} else {
		rect_w = 2;
		rect_h = 1;
	}

	if ((capabilities.x_resolution < 3 * rect_w) ||
	    (capabilities.y_resolution < 3 * rect_h) ||
	    (capabilities.x_resolution < 8 * rect_h)) {
		rect_w = capabilities.x_resolution * 40 / 100;
		rect_h = capabilities.y_resolution * 40 / 100;
		h_step = capabilities.y_resolution * 20 / 100;
		scale = 1;
	} else {
		h_step = rect_h;
		scale = (capabilities.x_resolution / 8) / rect_h;
	}

	rect_w *= scale;
	rect_h *= scale;

	if (capabilities.screen_info & SCREEN_INFO_EPD) {
		grey_scale_sleep = 10000;
	} else {
		grey_scale_sleep = 100;
	}

	buf_size = rect_w * rect_h;

	if (buf_size < (capabilities.x_resolution * h_step)) {
		buf_size = capabilities.x_resolution * h_step;
	}

	switch (capabilities.current_pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_argb8888;
		buf_size *= 4;
		break;
	case PIXEL_FORMAT_RGB_888:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_rgb888;
		buf_size *= 3;
		break;
	case PIXEL_FORMAT_RGB_565:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_rgb565;
		buf_size *= 2;
		break;
	case PIXEL_FORMAT_BGR_565:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_bgr565;
		buf_size *= 2;
		break;
	case PIXEL_FORMAT_MONO01:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_mono01;
		buf_size = DIV_ROUND_UP(DIV_ROUND_UP(
			buf_size, NUM_BITS(uint8_t)), sizeof(uint8_t));
		break;
	case PIXEL_FORMAT_MONO10:
		bg_color = 0x00u;
		fill_buffer_fnc = fill_buffer_mono10;
		buf_size = DIV_ROUND_UP(DIV_ROUND_UP(
			buf_size, NUM_BITS(uint8_t)), sizeof(uint8_t));
		break;
	default:
		LOG_ERR("Unsupported pixel format. Aborting sample.");
#ifdef CONFIG_ARCH_POSIX
		posix_exit_main(1);
#else
		return 0;
#endif
	}

	buf = k_malloc(buf_size);

	if (buf == NULL) {
		LOG_ERR("Could not allocate memory. Aborting sample.");
#ifdef CONFIG_ARCH_POSIX
		posix_exit_main(1);
#else
		return 0;
#endif
	}

	(void)memset(buf, bg_color, buf_size);

	buf_desc.buf_size = buf_size;
	buf_desc.pitch = capabilities.x_resolution;
	buf_desc.width = capabilities.x_resolution;
	buf_desc.height = h_step;

	for (int idx = 0; idx < capabilities.y_resolution; idx += h_step) {
		display_write(display_dev, 0, idx, &buf_desc, buf);
	}

	buf_desc.pitch = rect_w;
	buf_desc.width = rect_w;
	buf_desc.height = rect_h;

	fill_buffer_fnc(TOP_LEFT, 0, buf, buf_size);
	x = 0;
	y = 0;
	display_write(display_dev, x, y, &buf_desc, buf);

	fill_buffer_fnc(TOP_RIGHT, 0, buf, buf_size);
	x = capabilities.x_resolution - rect_w;
	y = 0;
	display_write(display_dev, x, y, &buf_desc, buf);

	fill_buffer_fnc(BOTTOM_RIGHT, 0, buf, buf_size);
	x = capabilities.x_resolution - rect_w;
	y = capabilities.y_resolution - rect_h;
	display_write(display_dev, x, y, &buf_desc, buf);

	display_blanking_off(display_dev);

	grey_count = 0;
	x = 0;
	y = capabilities.y_resolution - rect_h;

	while (1) {
		fill_buffer_fnc(BOTTOM_LEFT, grey_count, buf, buf_size);
		display_write(display_dev, x, y, &buf_desc, buf);
		++grey_count;
		k_msleep(grey_scale_sleep);
#if CONFIG_TEST
		if (grey_count >= 1024) {
			break;
		}
#endif
	}

#ifdef CONFIG_ARCH_POSIX
	posix_exit_main(0);
#endif
	return 0;
}
