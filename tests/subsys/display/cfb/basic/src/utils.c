/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils.h"

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cfb_test_draw_text_and_print_utils, CONFIG_CFB_LOG_LEVEL);

static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const uint32_t display_width = DT_PROP(DT_CHOSEN(zephyr_display), width);
static const uint32_t display_height = DT_PROP(DT_CHOSEN(zephyr_display), height);
uint8_t read_buffer[DT_PROP(DT_CHOSEN(zephyr_display), width) *
		    DT_PROP(DT_CHOSEN(zephyr_display), height) * 4];

inline uint32_t mono_pixel_order(uint32_t order)
{
	if (IS_ENABLED(CONFIG_SDL_DISPLAY_MONO_MSB_FIRST)) {
		return BIT(7 - order);
	} else {
		return BIT(order);
	}
}

uint32_t display_pixel(int x, int y)
{
	const uint8_t *ptr = read_buffer + (display_width * (y / 8) + x);
	struct display_capabilities display_caps;

	display_get_capabilities(dev, &display_caps);

	if (display_caps.current_pixel_format == PIXEL_FORMAT_MONO10) {
		return !(*ptr & mono_pixel_order(y % 8));
	}

	return !!(*ptr & mono_pixel_order(y % 8));
}

uint32_t image_pixel(const uint32_t *img, size_t width, int x, int y)
{
	const uint32_t *ptr = img + (width * y + x);

	return !!(*ptr & 0xFFFFFF);
}

bool verify_pixel(int x, int y, uint32_t color)
{
	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_height * display_width / 8,
	};

	zassert_ok(display_read(dev, 0, 0, &desc, read_buffer), "display_read failed");

	return ((!!display_pixel(x, y)) == (!!color));
}

bool verify_image(int cmp_x, int cmp_y, const uint32_t *img, size_t width, size_t height)
{
	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_height * display_width / 8,
	};

	zassert_ok(display_read(dev, 0, 0, &desc, read_buffer), "display_read failed");

	for (size_t y = 0; y < height; y++) {
		for (size_t x = 0; x < width; x++) {
			uint32_t disp_pix = display_pixel(cmp_x + x, cmp_y + y);
			uint32_t img_pix = image_pixel(img, width, x, y);

			if (disp_pix != img_pix) {
				LOG_INF("get_pixel(%d, %d) = %lu", x, y, disp_pix);
				LOG_INF("pixel_color(%d, %d) = %lu", x, y, img_pix);
				LOG_INF("disp@(0, %d) %p", y, read_buffer + (y * width / 8));
				LOG_HEXDUMP_INF(read_buffer + (y * width / 8), 64, "");
				LOG_INF("img@(0, %d) %p", y, (uint32_t *)img + (y * width));
				LOG_HEXDUMP_INF((uint32_t *)img + (y * width), 64, "");
				return false;
			}
		}
	}

	return true;
}

bool verify_color_inside_rect(int x, int y, size_t width, size_t height, uint32_t color)
{
	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_height * display_width / 8,
	};

	zassert_ok(display_read(dev, 0, 0, &desc, read_buffer), "display_read failed");

	for (size_t y_ = 0; y_ < height; y_++) {
		for (size_t x_ = 0; x_ < width; x_++) {
			uint32_t disp_pix = display_pixel(x + x_, y + y_);

			if (!!disp_pix != !!color) {
				return false;
			}
		}
	}

	return true;
}

bool verify_color_outside_rect(int x, int y, size_t width, size_t height, uint32_t color)
{
	bool ret = true;

	if (x > 0) {
		ret = verify_color_inside_rect(0, 0, x, y + height, color);
		if (!ret) {
			return false;
		}
	}

	if ((y + height) <= display_height) {
		ret = verify_color_inside_rect(0, y + height, x + width,
					       display_height - (y + height), color);
		if (!ret) {
			return false;
		}
	}

	if ((x + width) <= display_width) {
		ret = verify_color_inside_rect(x + width, y, display_width - (x + width),
					       display_height - y, color);
		if (!ret) {
			return false;
		}
	}

	if (y > 0) {
		ret = verify_color_inside_rect(x, 0, display_width - x, y, color);
		if (!ret) {
			return false;
		}
	}

	return true;
}

bool verify_image_and_bg(int x, int y, const uint32_t *img, size_t width, size_t height,
			 uint32_t color)
{
	bool ret = true;

	ret = verify_image(x, y, img, width, height);
	if (!ret) {
		return false;
	}

	ret = verify_color_outside_rect(x, y, width, height, color);

	return ret;
}

bool verify_pixel_and_bg(int x, int y, uint32_t pixcolor, uint32_t bgcolor)
{
	bool ret = true;

	ret = verify_pixel(x, y, pixcolor);
	if (!ret) {
		return false;
	}

	ret = verify_color_outside_rect(x, y, 1, 1, bgcolor);

	return ret;
}
