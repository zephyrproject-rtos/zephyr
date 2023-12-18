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
			    size_t x, size_t w, struct display_buffer_descriptor *buf_desc);


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
				 size_t x, size_t w, struct display_buffer_descriptor *buf_desc)
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

	for (size_t row = 0; row < buf_desc->height; row++) {
		for (size_t col = x; col < x + w; col++) {
			*((uint32_t *)buf + col) = color;
		}
		buf += buf_desc->pitch * sizeof(uint32_t);
	}
}

static void fill_buffer_rgb888(enum corner corner, uint8_t grey, uint8_t *buf,
			       size_t x, size_t w, struct display_buffer_descriptor *buf_desc)
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

	for (size_t row = 0; row < buf_desc->height; row++) {
		for (size_t col = x; col < x + w; col++) {
			*(buf + 3 * col + 0) = color >> 16;
			*(buf + 3 * col + 1) = color >> 8;
			*(buf + 3 * col + 2) = color >> 0;
		}
		buf += buf_desc->pitch * 3;
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
			       size_t x, size_t w, struct display_buffer_descriptor *buf_desc)
{
	uint16_t color = get_rgb565_color(corner, grey);

	for (size_t row = 0; row < buf_desc->height; row++) {
		for (size_t col = x; col < x + w; col++) {
			*(buf + 2 * col + 0) = (color >> 8) & 0xFFu;
			*(buf + 2 * col + 1) = (color >> 0) & 0xFFu;
		}
		buf += buf_desc->pitch * 2;
	}
}

static void fill_buffer_bgr565(enum corner corner, uint8_t grey, uint8_t *buf,
			       size_t x, size_t w, struct display_buffer_descriptor *buf_desc)
{
	uint16_t color = get_rgb565_color(corner, grey);

	for (size_t row = 0; row < buf_desc->height; row++) {
		for (size_t col = x; col < x + w; col++) {
			*((uint16_t *)buf + col) = color;
		}

		buf += buf_desc->pitch * 2;
	}
}

static uint8_t get_rgbx111_color(enum corner corner, uint8_t grey)
{
	uint8_t color = 0;

	switch (corner) {
	case TOP_LEFT:
		color = 0b1000;
		break;
	case TOP_RIGHT:
		color = 0b0100;
		break;
	case BOTTOM_RIGHT:
		color = 0b0010;
		break;
	case BOTTOM_LEFT:
		/* 3bpp shifted to the left one dummy bit */
		color = (grey % 2 == 0 ? 0b1110 : 0b0000);
		break;
	}
	return color;
}

static void fill_buffer_rgbx111(enum corner corner, uint8_t grey,
				uint8_t *buf, size_t x, size_t w,
				struct display_buffer_descriptor *buf_desc)
{
	uint8_t color = get_rgbx111_color(corner, grey);

	for (size_t row = 0; row < buf_desc->height; row++) {
		for (size_t col = x; col < x + w; col += 2) {
			*(buf + (col / 2)) = (color << 4) | color;
		}
		buf += buf_desc->pitch / 2;
	}
}

static void fill_buffer_mono(enum corner corner, uint8_t grey, uint8_t *buf,
			     size_t x, size_t w,
			     struct display_buffer_descriptor *buf_desc)
{
	uint16_t color;

	switch (corner) {
	case BOTTOM_LEFT:
		color = (grey & 0x01u) ? 0xFFu : 0x00u;
		break;
	default:
		color = 0;
		break;
	}

	for (size_t row = 0; row < buf_desc->height; row++) {
		for (size_t col = x; col < x + w; col += 8) {
			*(buf + (col / 8)) = color;
		}
		buf += buf_desc->pitch / 8;
	}
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
	uint8_t *buf;
	int32_t grey_scale_sleep;
	const struct device *display_dev;
	struct display_capabilities capabilities;
	struct display_buffer_descriptor buf_desc;
	size_t buf_size = 0;
	fill_buffer fill_buffer_fnc = NULL;
	size_t xoffset;

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

	h_step = rect_h;
	scale = (capabilities.x_resolution / 8) / rect_h;

	rect_w *= scale;
	rect_h *= scale;

	if (capabilities.screen_info & SCREEN_INFO_EPD) {
		grey_scale_sleep = 10000;
	} else {
		grey_scale_sleep = 100;
	}

	if (capabilities.screen_info & SCREEN_INFO_X_ALIGNMENT_WIDTH) {
		buf_size = capabilities.x_resolution * rect_h;
	} else {
		buf_size = rect_w * rect_h;
	}

	if (buf_size < (capabilities.x_resolution * h_step)) {
		buf_size = capabilities.x_resolution * h_step;
	}

	switch (capabilities.current_pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		fill_buffer_fnc = fill_buffer_argb8888;
		buf_size *= 4;
		break;
	case PIXEL_FORMAT_RGB_888:
		fill_buffer_fnc = fill_buffer_rgb888;
		buf_size *= 3;
		break;
	case PIXEL_FORMAT_RGB_565:
		fill_buffer_fnc = fill_buffer_rgb565;
		buf_size *= 2;
		break;
	case PIXEL_FORMAT_BGR_565:
		fill_buffer_fnc = fill_buffer_bgr565;
		buf_size *= 2;
		break;
	case PIXEL_FORMAT_MONO01:
	case PIXEL_FORMAT_MONO10:
		fill_buffer_fnc = fill_buffer_mono;
		buf_size /= 8;
		break;
	case PIXEL_FORMAT_RGBx_111:
		fill_buffer_fnc = fill_buffer_rgbx111;
		buf_size /= 2;
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

	(void)memset(buf, 0xFFu, buf_size);

	buf_desc.buf_size = buf_size;
	buf_desc.pitch = capabilities.x_resolution;
	buf_desc.width = capabilities.x_resolution;
	buf_desc.height = h_step;

	for (int idx = 0; idx < capabilities.y_resolution; idx += h_step) {
		display_write(display_dev, 0, idx, &buf_desc, buf);
	}

	if (capabilities.screen_info & SCREEN_INFO_X_ALIGNMENT_WIDTH) {
		buf_desc.pitch = capabilities.x_resolution;
		buf_desc.width = capabilities.x_resolution;
		xoffset = capabilities.x_resolution - rect_w;
	} else {
		buf_desc.pitch = rect_w;
		buf_desc.width = rect_w;
		xoffset = 0;
	}
	buf_desc.height = rect_h;

	x = 0;
	y = 0;
	fill_buffer_fnc(TOP_LEFT, 0, buf, 0, rect_w, &buf_desc);
	display_write(display_dev, x, y, &buf_desc, buf);

	if (!(capabilities.screen_info & SCREEN_INFO_X_ALIGNMENT_WIDTH)) {
		x = capabilities.x_resolution - rect_w;
	}
	y = 0;
	fill_buffer_fnc(TOP_RIGHT, 0, buf, xoffset, rect_w, &buf_desc);
	display_write(display_dev, x, y, &buf_desc, buf);

	if (!(capabilities.screen_info & SCREEN_INFO_X_ALIGNMENT_WIDTH)) {
		x = capabilities.x_resolution - rect_w;
	} else {
		(void)memset(buf, 0xFFu, buf_size);
	}
	y = capabilities.y_resolution - rect_h;
	display_write(display_dev, x, y, &buf_desc, buf);

	display_blanking_off(display_dev);

	grey_count = 0;
	x = 0;
	y = capabilities.y_resolution - rect_h;

	while (1) {
		fill_buffer_fnc(BOTTOM_LEFT, grey_count, buf, 0, rect_w, &buf_desc);
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
