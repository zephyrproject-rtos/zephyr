/*
 * Copyright 2024 (c) TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/display.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(display_api, CONFIG_DISPLAY_LOG_LEVEL);

static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const uint32_t display_width = DT_PROP(DT_CHOSEN(zephyr_display), width);
static const uint32_t display_height = DT_PROP(DT_CHOSEN(zephyr_display), height);
#ifdef CONFIG_DISPLAY_BUFFER_USE_GENERIC_SECTION
Z_GENERIC_SECTION(CONFIG_DISPLAY_BUFFER_SECTION)
#endif
static uint8_t disp_buffer[DT_PROP(DT_CHOSEN(zephyr_display), width) *
			   DT_PROP(DT_CHOSEN(zephyr_display), height) * 4]
	__aligned(CONFIG_DISPLAY_BUFFER_ALIGNMENT);
static struct display_capabilities cfg;
static uint8_t bpp;
static bool is_tiled;

static inline uint8_t bytes_per_pixel(enum display_pixel_format pixel_format)
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

static void verify_bytes_of_area(uint8_t *data, int cmp_x, int cmp_y, size_t width, size_t height)
{
	struct display_buffer_descriptor desc = {
		.height = height,
		.pitch = width,
		.width = width,
		.buf_size = height * width * bpp,
	};

	int err = display_read(dev, cmp_x, cmp_y, &desc, disp_buffer);

	zassert_ok(err, "display_read failed");

	if (is_tiled) {
		zassert_mem_equal(data, disp_buffer, width * height / 8, NULL);
	} else {
		zassert_mem_equal(data, disp_buffer, width * height * bpp, NULL);
	}
}

static void verify_background_color(int x, int y, size_t width, size_t height, uint32_t color)
{
	size_t buf_size = is_tiled ? (height * width / 8) : (height * width * bpp);
	struct display_buffer_descriptor desc = {
		.height = height,
		.pitch = width,
		.width = width,
		.buf_size = buf_size,
	};
	uint32_t *buf32 = (void *)disp_buffer;
	uint16_t *buf16 = (void *)disp_buffer;
	uint8_t *buf8 = disp_buffer;
	int err;

	err = display_read(dev, x, y, &desc, disp_buffer);
	zassert_ok(err, "display_read failed");

	for (size_t i = 0; i < width * height; i++) {
		switch (bpp) {
		case 4:
			zassert_equal(buf32[i], color, "@%d", i);
			break;
		case 2:
			zassert_equal(buf16[i], (uint16_t)color, "@%d", i);
			break;
		case 1:
			if (is_tiled) {
				uint16_t x = i % (width);
				uint16_t line = (i - x) / width;
				uint16_t tile = line / 8;
				uint16_t y = line % 8;

				uint8_t *tptr = disp_buffer + (tile * width + x);

				zassert_equal(!!(*tptr & BIT(y)), !!(color), "@%d", i);
			} else {
				zassert_equal(buf8[i], (uint8_t)color, "@%d", i);
			}
			break;
		}
	}
}

/**
 * Fill the buffer with 0 before running tests.
 */
static void display_before(void *text_fixture)
{
	display_get_capabilities(dev, &cfg);
	bpp = bytes_per_pixel(cfg.current_pixel_format);
	is_tiled = ((bpp == 1) && (cfg.screen_info & SCREEN_INFO_MONO_VTILED));

	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_height * display_width * bpp,
	};

	memset(disp_buffer, 0, sizeof(disp_buffer));
	display_write(dev, 0, 0, &desc, disp_buffer);
}

/*
 * Verify that we can get a color of '0' from all pixels
 * when after zeroing the buffer.
 */
ZTEST(display_read_write, test_clear)
{
	verify_background_color(0, 0, display_width, display_height, 0);
}

/*
 * Write to the head of the buffer and check that
 * the same value can be read.
 */
ZTEST(display_read_write, test_write_to_buffer_head)
{
	uint8_t data[4] = {0xFA, 0xAF, 0x9F, 0xFA};
	uint8_t height = (is_tiled ? 8 : 1);
	uint16_t width = sizeof(data) / bpp;
	uint16_t buf_size = width * bpp;
	struct display_buffer_descriptor desc = {
		.height = height,
		.pitch = width,
		.width = width,
		.buf_size = buf_size,
	};

	/* write data to head of buffer */
	display_write(dev, 0, 0, &desc, data);

	/* check write data and read data are same */
	verify_bytes_of_area(data, 0, 0, width, height);

	/* check remaining region still black */
	verify_background_color(0, height, display_width, display_height - height, 0);
	verify_background_color(width, 0, display_width - width, display_height, 0);
}

/*
 * Write to the tail of the buffer and check that
 * the same value can be read.
 */
ZTEST(display_read_write, test_write_to_buffer_tail)
{
	uint8_t data[4] = {0xFA, 0xAF, 0x9F, 0xFA};
	uint16_t height = (is_tiled ? 8 : 1);
	uint16_t width = sizeof(data) / bpp;
	uint16_t buf_size = width * bpp;
	struct display_buffer_descriptor desc = {
		.height = height,
		.pitch = width,
		.width = width,
		.buf_size = buf_size,
	};
	struct display_buffer_descriptor desc_whole = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_height * display_width * bpp / height,
	};
	int err;

	/* write data to tail of buffer */
	display_write(dev, display_width - width, display_height - height, &desc, data);

	/* read entire displayed data */
	err = display_read(dev, 0, 0, &desc_whole, disp_buffer);
	zassert_ok(err, "display_read failed");

	/* check write data and read data are same */
	if (is_tiled) {
		zassert_mem_equal(data,
				  disp_buffer + (display_width * display_height / 8 - buf_size),
				  buf_size, NULL);
	} else {
		zassert_mem_equal(data,
				  disp_buffer + (display_width * display_height * bpp - buf_size),
				  buf_size, NULL);
	}

	/* check remaining region still black */
	verify_background_color(0, 0, display_width, display_height - height, 0);
	verify_background_color(0, display_height - height, display_width - width, height, 0);
}

/*
 * Verify that it will keep the drawn content even if display_read is executed
 */
ZTEST(display_read_write, test_read_does_not_clear_existing_buffer)
{
	uint8_t data[4] = {0xFA, 0xAF, 0x9F, 0xFA};
	uint8_t height = (is_tiled ? 8 : 1);
	uint16_t width = sizeof(data) / bpp;
	uint16_t buf_size = width * bpp;
	struct display_buffer_descriptor desc = {
		.height = height,
		.pitch = width,
		.width = width,
		.buf_size = buf_size,
	};
	struct display_buffer_descriptor desc_whole = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_height * display_width * bpp / height,
	};
	int err;

	/* write data to head of buffer */
	display_write(dev, 0, 0, &desc, data);

	/* check write data and read data are same */
	verify_bytes_of_area(data, 0, 0, width, height);

	/* check remaining region still black */
	verify_background_color(0, height, display_width, display_height - height, 0);
	verify_background_color(width, 0, display_width - width, display_height, 0);

	/* write data to tail of buffer */
	display_write(dev, display_width - width, display_height - height, &desc, data);

	/* read entire displayed data */
	err = display_read(dev, 0, 0, &desc_whole, disp_buffer);
	zassert_ok(err, "display_read failed");

	/* checking correctly write to the tail of buffer */
	if (is_tiled) {
		zassert_mem_equal(data,
				  disp_buffer + (display_width * display_height / 8 - buf_size),
				  buf_size, NULL);
	} else {
		zassert_mem_equal(data,
				  disp_buffer + (display_width * display_height * bpp - buf_size),
				  buf_size, NULL);
	}

	/* checking if the content written before reading is kept */
	verify_bytes_of_area(data, 0, 0, width, height);

	/* checking remaining region is still black */
	verify_background_color(width, 0, display_width - width, display_height - height, 0);
	verify_background_color(0, height, display_width - width, display_height - height, 0);
}

ZTEST_SUITE(display_read_write, NULL, NULL, display_before, NULL, NULL);
