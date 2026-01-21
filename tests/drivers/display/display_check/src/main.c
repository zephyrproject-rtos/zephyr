/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2025 NXP
 *
 * Based on ST7789V sample:
 * Copyright (c) 2019 Marc Reilly
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/ztest.h>

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
			    struct display_buffer_descriptor *buf_desc);


#ifdef CONFIG_ARCH_POSIX
static void posix_exit_main(int exit_code)
{
	posix_exit(exit_code);
}
#endif

static void fill_buffer_argb8888(enum corner corner, uint8_t grey, uint8_t *buf,
				 struct display_buffer_descriptor *buf_desc)
{
	uint32_t color = 0;

	switch (corner) {
	case TOP_LEFT:
		color = 0xFFFF0000u;
		break;
	case TOP_RIGHT:
		color = 0xFF00FF00u;
		break;
	case BOTTOM_RIGHT:
		color = 0xFF0000FFu;
		break;
	case BOTTOM_LEFT:
		color = 0xFF000000u | grey << 16 | grey << 8 | grey;
		break;
	}

	for (size_t row = 0; row < buf_desc->height; row++) {
		for (size_t col = 0; col < buf_desc->width; col++) {
			size_t idx = row * buf_desc->pitch + col * 4;
			*((uint32_t *)(buf + idx)) = color;
		}
	}
}

static void fill_buffer_rgb888(enum corner corner, uint8_t grey, uint8_t *buf,
			       struct display_buffer_descriptor *buf_desc)
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
		for (size_t col = 0; col < buf_desc->width; col++) {
			size_t idx = row * buf_desc->pitch + col * 3;
			*(buf + idx + 0) = color >> 16;
			*(buf + idx + 1) = color >> 8;
			*(buf + idx + 2) = color >> 0;
		}
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

static void fill_buffer_rgb565x(enum corner corner, uint8_t grey, uint8_t *buf,
			       struct display_buffer_descriptor *buf_desc)
{
	uint16_t color = get_rgb565_color(corner, grey);

	for (size_t row = 0; row < buf_desc->height; row++) {
		for (size_t col = 0; col < buf_desc->width; col++) {
			size_t idx = row * buf_desc->pitch + col * 2;
			*(buf + idx + 0) = (color >> 8) & 0xFFu;
			*(buf + idx + 1) = (color >> 0) & 0xFFu;
		}
	}
}

static void fill_buffer_rgb565(enum corner corner, uint8_t grey, uint8_t *buf,
			       struct display_buffer_descriptor *buf_desc)
{
	uint16_t color = get_rgb565_color(corner, grey);

	for (size_t row = 0; row < buf_desc->height; row++) {
		for (size_t col = 0; col < buf_desc->width; col++) {
			size_t idx = row * buf_desc->pitch + col * 2;
			*(uint16_t *)(buf + idx) = color;
		}
	}
}

static void fill_buffer_mono(enum corner corner, uint8_t grey,
			     uint8_t black, uint8_t white,
			     uint8_t *buf,
			     struct display_buffer_descriptor *buf_desc)
{
	uint8_t color;

	switch (corner) {
	case BOTTOM_LEFT:
		color = (grey & 0x01u) ? white : black;
		break;
	default:
		color = black;
		break;
	}

	for (size_t row = 0; row < buf_desc->height; row++) {
		for (size_t col = 0; col < DIV_ROUND_UP(buf_desc->width, 8U); col++) {
			size_t idx = row * buf_desc->pitch + col;
			*(uint8_t *)(buf + idx) = color;
		}
	}
}

static inline void fill_buffer_l_8(enum corner corner, uint8_t grey, uint8_t *buf,
				   struct display_buffer_descriptor *buf_desc)
{
	for (size_t row = 0; row < buf_desc->height; row++) {
		for (size_t col = 0; col < buf_desc->width; col++) {
			size_t idx = row * buf_desc->pitch + col;
			*(uint8_t *)(buf + idx) = grey;
		}
	}
}

static void fill_buffer_al_88(enum corner corner, uint8_t grey, uint8_t *buf,
			      struct display_buffer_descriptor *buf_desc)
{
	uint16_t color;

	switch (corner) {
	case TOP_LEFT:
		color = 0xFF00u;
		break;
	case TOP_RIGHT:
		color = 0xFFFFu;
		break;
	case BOTTOM_RIGHT:
		color = 0xFF88u;
		break;
	case BOTTOM_LEFT:
		color = 0xFF00u | grey;
		break;
	default:
		color = 0;
		break;
	}

	for (size_t row = 0; row < buf_desc->height; row++) {
		for (size_t col = 0; col < buf_desc->width; col++) {
			size_t idx = row * buf_desc->pitch + col * 2;
			*((uint16_t *)(buf + idx)) = color;
		}
	}
}

static inline void fill_buffer_mono01(enum corner corner, uint8_t grey,
				      uint8_t *buf,
				      struct display_buffer_descriptor *buf_desc)
{
	fill_buffer_mono(corner, grey, 0x00u, 0xFFu, buf, buf_desc);
}

static inline void fill_buffer_mono10(enum corner corner, uint8_t grey,
				      uint8_t *buf,
				      struct display_buffer_descriptor *buf_desc)
{
	fill_buffer_mono(corner, grey, 0xFFu, 0x00u, buf, buf_desc);
}

int test_display(void)
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
	uint16_t rect_pitch;
	uint16_t full_screen_pitch;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device %s not found. Aborting test.",
			display_dev->name);
#ifdef CONFIG_ARCH_POSIX
		posix_exit_main(1);
#else
		return 0;
#endif
	}

	LOG_INF("Display test for %s", display_dev->name);
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

	if (capabilities.screen_info & SCREEN_INFO_X_ALIGNMENT_WIDTH) {
		rect_w = capabilities.x_resolution;
	}

	switch (capabilities.current_pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		bg_color = 0x00u;
		fill_buffer_fnc = fill_buffer_argb8888;
		rect_pitch = rect_w * 4;
		full_screen_pitch = capabilities.x_resolution * 4;
		break;
	case PIXEL_FORMAT_RGB_888:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_rgb888;
		rect_pitch = rect_w * 3;
		full_screen_pitch = capabilities.x_resolution * 3;
		break;
	case PIXEL_FORMAT_RGB_565:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_rgb565;
		rect_pitch = rect_w * 2;
		full_screen_pitch = capabilities.x_resolution * 2;
		break;
	case PIXEL_FORMAT_RGB_565X:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_rgb565x;
		rect_pitch = rect_w * 2;
		full_screen_pitch = capabilities.x_resolution * 2;
		break;
	case PIXEL_FORMAT_L_8:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_l_8;
		rect_pitch = rect_w;
		full_screen_pitch = capabilities.x_resolution;
		break;
	case PIXEL_FORMAT_AL_88:
		bg_color = 0x00u;
		fill_buffer_fnc = fill_buffer_al_88;
		rect_pitch = rect_w * 2;
		full_screen_pitch = capabilities.x_resolution * 2;
		break;
	case PIXEL_FORMAT_MONO01:
		bg_color = 0xFFu;
		fill_buffer_fnc = fill_buffer_mono01;
		rect_pitch = DIV_ROUND_UP(DIV_ROUND_UP(
			rect_w, NUM_BITS(uint8_t)), sizeof(uint8_t));
		full_screen_pitch = DIV_ROUND_UP(DIV_ROUND_UP(
			capabilities.x_resolution, NUM_BITS(uint8_t)), sizeof(uint8_t));
		break;
	case PIXEL_FORMAT_MONO10:
		bg_color = 0x00u;
		fill_buffer_fnc = fill_buffer_mono10;
		rect_pitch = DIV_ROUND_UP(DIV_ROUND_UP(
			rect_w, NUM_BITS(uint8_t)), sizeof(uint8_t));
		full_screen_pitch = DIV_ROUND_UP(DIV_ROUND_UP(
			capabilities.x_resolution, NUM_BITS(uint8_t)), sizeof(uint8_t));
		break;
	default:
		LOG_ERR("Unsupported pixel format. Aborting test.");
#ifdef CONFIG_ARCH_POSIX
		posix_exit_main(1);
#else
		return 0;
#endif
	}

	buf_size = MAX(rect_pitch * rect_h, full_screen_pitch * h_step);
	buf = k_malloc(buf_size);
	if (buf == NULL) {
		LOG_ERR("Could not allocate memory. Aborting test.");
#ifdef CONFIG_ARCH_POSIX
		posix_exit_main(1);
#else
		return 0;
#endif
	}

	(void)memset(buf, bg_color, buf_size);

	buf_desc.buf_size = buf_size;
	buf_desc.pitch = full_screen_pitch;
	buf_desc.width = capabilities.x_resolution;
	buf_desc.height = h_step;

	/*
	 * The following writes will only render parts of the image,
	 * so turn this option on.
	 * This allows double-buffered displays to hold the pixels
	 * back until the image is complete.
	 */
	buf_desc.frame_incomplete = true;

	for (int idx = 0; idx < capabilities.y_resolution; idx += h_step) {
		/*
		 * Tweaking the height value not to draw outside of the display.
		 * It is required when using a monochrome display whose vertical
		 * resolution can not be divided by 8.
		 */
		if ((capabilities.y_resolution - idx) < h_step) {
			buf_desc.height = (capabilities.y_resolution - idx);
		}
		display_write(display_dev, 0, idx, &buf_desc, buf);
	}

	buf_desc.pitch = rect_pitch;
	buf_desc.width = rect_w;
	buf_desc.height = rect_h;

	fill_buffer_fnc(TOP_LEFT, 0, buf, &buf_desc);
	x = 0;
	y = 0;
	display_write(display_dev, x, y, &buf_desc, buf);

	fill_buffer_fnc(TOP_RIGHT, 0, buf, &buf_desc);
	x = capabilities.x_resolution - rect_w;
	y = 0;
	display_write(display_dev, x, y, &buf_desc, buf);

	/*
	 * This is the last write of the frame, so turn this off.
	 * Double-buffered displays will now present the new image
	 * to the user.
	 */
	buf_desc.frame_incomplete = false;

	fill_buffer_fnc(BOTTOM_RIGHT, 0, buf, &buf_desc);
	x = capabilities.x_resolution - rect_w;
	y = capabilities.y_resolution - rect_h;
	display_write(display_dev, x, y, &buf_desc, buf);

	display_blanking_off(display_dev);

	grey_count = 50;
	x = 0;
	y = capabilities.y_resolution - rect_h;

	LOG_INF("Display starts");
	while (1) {
		fill_buffer_fnc(BOTTOM_LEFT, grey_count, buf, &buf_desc);
		display_write(display_dev, x, y, &buf_desc, buf);
		k_msleep(grey_scale_sleep);
	}

#ifdef CONFIG_ARCH_POSIX
	posix_exit_main(0);
#endif
	return 0;
}


ZTEST(display_test, test_display_by_capture)
{
	test_display();
}


ZTEST_SUITE(display_test, NULL, NULL, NULL, NULL, NULL);
