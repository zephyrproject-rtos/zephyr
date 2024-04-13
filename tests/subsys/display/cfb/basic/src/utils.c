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
#include <zephyr/display/cfb.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(cfb_test_draw_text_and_print_utils, CONFIG_CFB_LOG_LEVEL);

const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
const uint32_t display_width = DT_PROP(DT_CHOSEN(zephyr_display), width);
const uint32_t display_height = DT_PROP(DT_CHOSEN(zephyr_display), height);
uint8_t read_buffer[DT_PROP(DT_CHOSEN(zephyr_display), width) *
		    DT_PROP(DT_CHOSEN(zephyr_display), height) * 4];
#if CONFIG_TEST_TRANSFER_BUF_SIZE != 0
uint8_t transfer_buffer[DT_PROP(DT_CHOSEN(zephyr_display), width) *
			DT_PROP(DT_CHOSEN(zephyr_display), height) * 4];
static struct cfb_display disp;
#endif

uint8_t bytes_per_pixel(enum display_pixel_format pixel_format)
{
	switch (pixel_format) {
	case PIXEL_FORMAT_ARGB_8888:
		return 4;
	case PIXEL_FORMAT_RGB_888:
		return 3;
	case PIXEL_FORMAT_RGB_565:
	case PIXEL_FORMAT_BGR_565:
		return 2;
	case PIXEL_FORMAT_MONO01:
	case PIXEL_FORMAT_MONO10:
	default:
		return 1;
	}

	return 0;
}

inline uint32_t mono_pixel_order(uint32_t order)
{
	if (IS_ENABLED(CONFIG_SDL_DISPLAY_MONO_MSB_FIRST)) {
		return BIT(7 - order);
	} else {
		return BIT(order);
	}
}

struct cfb_display *display_init(void)
{
#if CONFIG_TEST_TRANSFER_BUF_SIZE != 0
	struct cfb_display_init_param param = {
		.dev = dev,
		.transfer_buf = transfer_buffer,
		.transfer_buf_size = sizeof(transfer_buffer),
	};
#else
	struct cfb_display *pdisp;
#endif

	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_buf_size(dev),
	};

	memset(read_buffer, 0, sizeof(read_buffer));
	zassert_ok(display_write(dev, 0, 0, &desc, read_buffer));

	zassert_ok(display_blanking_off(dev));

#if CONFIG_TEST_TRANSFER_BUF_SIZE != 0
	zassert_ok(cfb_display_init(&disp, &param));
	return &disp;
#else
	pdisp = cfb_display_alloc(dev);
	zassert_not_null(pdisp);
	return pdisp;
#endif
}

void display_deinit(struct cfb_display *disp)
{
#if CONFIG_TEST_TRANSFER_BUF_SIZE != 0
	cfb_display_deinit(disp);
#else
	cfb_display_free(disp);
#endif
}

static inline uint32_t pixel_per_tile(enum display_pixel_format pixel_format)
{
	if (((pixel_format == PIXEL_FORMAT_MONO01) || (pixel_format == PIXEL_FORMAT_MONO10))) {
		return 8;
	}

	return 1;
}

uint32_t display_buf_size(const struct device *dev)
{
	struct display_capabilities caps;

	display_get_capabilities(dev, &caps);

	return display_width * display_height * bytes_per_pixel(caps.current_pixel_format) /
	       pixel_per_tile(caps.current_pixel_format);
}

uint32_t display_pixel(int x, int y)
{
	uint8_t *ptr;
	struct display_capabilities caps;

	display_get_capabilities(dev, &caps);
	ptr = read_buffer +
	      (((display_width * (y / pixel_per_tile(caps.current_pixel_format))) + x) *
	       bytes_per_pixel(caps.current_pixel_format));

	if (caps.current_pixel_format == PIXEL_FORMAT_MONO01) {
		return (*ptr & mono_pixel_order(y % 8)) ? COLOR_WHITE : 0;
	} else if (caps.current_pixel_format == PIXEL_FORMAT_MONO10) {
		return (*ptr & mono_pixel_order(y % 8)) ? 0 : COLOR_WHITE;
	} else if (caps.current_pixel_format == PIXEL_FORMAT_ARGB_8888) {
		return (*(uint32_t *)ptr) & 0x00FFFFFF;
	} else if (caps.current_pixel_format == PIXEL_FORMAT_RGB_888) {
		return (*ptr << 16) | (*(ptr + 1) << 8) | *(ptr + 2);
	} else if (caps.current_pixel_format == PIXEL_FORMAT_RGB_565) {
		uint16_t c = sys_be16_to_cpu(*(uint16_t *)ptr);

		return ((c & 0xF800) << 8) | ((c & 0x7E0) << 5) | ((c & 0x1F) << 3);
	} else if (caps.current_pixel_format == PIXEL_FORMAT_BGR_565) {
		uint16_t c = *(uint16_t *)ptr;

		return ((c & 0xF800) << 8) | ((c & 0x7E0) << 5) | ((c & 0x1F) << 3);
	} else {
		return 0xFFFFFFFF;
	}
}

uint32_t image_pixel(const uint32_t *img, size_t width, int x, int y)
{
	const uint32_t *ptr = img + (width * y + x);

	return *ptr;
}

bool compare_pixel(enum display_pixel_format pixel_format, uint32_t disp_pix, uint32_t img_pix)
{
	if ((pixel_format == PIXEL_FORMAT_RGB_565) || (pixel_format == PIXEL_FORMAT_BGR_565)) {
		return ((disp_pix & 0xF8FCF8) == (img_pix & 0xF8FCF8));
	}

	return ((disp_pix & 0x00FFFFFF) == (img_pix & 0x00FFFFFF));
}

bool verify_pixel(int x, int y, uint32_t color)
{
	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_buf_size(dev),
	};
	struct display_capabilities caps;

	display_get_capabilities(dev, &caps);

	zassert_ok(display_read(dev, 0, 0, &desc, read_buffer), "display_read failed");

	return compare_pixel(caps.current_pixel_format, display_pixel(x, y), color);
}

bool verify_image(int cmp_x, int cmp_y, const uint32_t *img, size_t width, size_t height)
{
	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_buf_size(dev),
	};
	struct display_capabilities caps;

	display_get_capabilities(dev, &caps);

	zassert_ok(display_read(dev, 0, 0, &desc, read_buffer), "display_read failed");

	for (size_t y = 0; y < height; y++) {
		for (size_t x = 0; x < width; x++) {
			uint32_t disp_pix = display_pixel(cmp_x + x, cmp_y + y);
			uint32_t img_pix = image_pixel(img, width, x, y);

			if (!compare_pixel(caps.current_pixel_format, disp_pix, img_pix)) {
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
		.buf_size = display_buf_size(dev),
	};
	struct display_capabilities caps;

	display_get_capabilities(dev, &caps);

	zassert_ok(display_read(dev, 0, 0, &desc, read_buffer), "display_read failed");

	for (size_t y_ = 0; y_ < height; y_++) {
		for (size_t x_ = 0; x_ < width; x_++) {
			uint32_t disp_pix = display_pixel(x + x_, y + y_);

			if (!compare_pixel(caps.current_pixel_format, disp_pix, color)) {
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
