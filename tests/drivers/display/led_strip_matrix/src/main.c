/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/ztest.h>

#define MATRIX_NODE DT_CHOSEN(zephyr_display)
#define STRIP_NODE DT_NODELABEL(fake_strip)
#define MATRIX_WIDTH DT_PROP(MATRIX_NODE, width)
#define MATRIX_HEIGHT DT_PROP(MATRIX_NODE, height)
#define NUM_PIXELS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define PIXEL_FORMAT DT_PROP(MATRIX_NODE, pixel_format)
#define BYTES_PER_PIXEL (PIXEL_FORMAT == PIXEL_FORMAT_ARGB_8888 ? 4U : 3U)

static const struct device *const display = DEVICE_DT_GET(MATRIX_NODE);

static struct {
	struct led_rgb pixels[NUM_PIXELS];
	size_t count;
	uint32_t calls;
	int result;
} strip_data;

static int strip_update_rgb(const struct device *dev, struct led_rgb *pixels, size_t count)
{
	ARG_UNUSED(dev);

	if (count > ARRAY_SIZE(strip_data.pixels)) {
		return -EINVAL;
	}

	memcpy(strip_data.pixels, pixels, count * sizeof(*pixels));
	strip_data.count = count;
	strip_data.calls++;
	return strip_data.result;
}

static size_t strip_length(const struct device *dev)
{
	ARG_UNUSED(dev);
	return DT_PROP(STRIP_NODE, chain_length);
}

static DEVICE_API(led_strip, strip_api) = {
	.update_rgb = strip_update_rgb,
	.length = strip_length,
};

DEVICE_DT_DEFINE(STRIP_NODE, NULL, NULL, NULL, NULL, POST_KERNEL,
		 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &strip_api);

/* Each RGB channel differs, and ARGB input alpha varies independently. */
static const struct led_rgb colors[NUM_PIXELS] = {
	{.r = 0x12, .g = 0x34, .b = 0x56},
	{.r = 0x9A, .g = 0xBC, .b = 0xDE},
	{.r = 0xF0, .g = 0x81, .b = 0x23},
	{.r = 0x45, .g = 0x67, .b = 0x89},
	{.r = 0xAB, .g = 0xCD, .b = 0xEF},
	{.r = 0x01, .g = 0x92, .b = 0xE3},
	{.r = 0xFE, .g = 0xDC, .b = 0xBA},
	{.r = 0x76, .g = 0x54, .b = 0x32},
};

static const uint32_t argb_colors[NUM_PIXELS] = {
	0x00123456U, 0x809ABCDEU, 0xFFF08123U, 0x11456789U,
	0x22ABCDEFU, 0x330192E3U, 0x44FEDCBAU, 0x55765432U,
};

struct pixel_buffer {
	uint32_t guard_before;
	union {
		uint32_t argb[NUM_PIXELS];
		uint8_t rgb[NUM_PIXELS * 3U];
	} pixels;
	uint32_t guard_after;
};

static const struct display_buffer_descriptor full_frame = {
	.buf_size = NUM_PIXELS * BYTES_PER_PIXEL,
	.width = MATRIX_WIDTH,
	.height = MATRIX_HEIGHT,
	.pitch = MATRIX_WIDTH,
};

static void set_pixel(struct pixel_buffer *buffer, size_t index, size_t color, bool opaque)
{
	if (PIXEL_FORMAT == PIXEL_FORMAT_ARGB_8888) {
		buffer->pixels.argb[index] = argb_colors[color];
		if (opaque) {
			buffer->pixels.argb[index] |= 0xFF000000U;
		}
	} else {
		buffer->pixels.rgb[index * 3U] = colors[color].r;
		buffer->pixels.rgb[index * 3U + 1U] = colors[color].g;
		buffer->pixels.rgb[index * 3U + 2U] = colors[color].b;
	}
}

static void fill_frame(struct pixel_buffer *buffer, bool opaque)
{
	memset(buffer, 0xA5, sizeof(*buffer));
	for (size_t i = 0; i < NUM_PIXELS; i++) {
		set_pixel(buffer, i, i, opaque);
	}
}

static void assert_strip(const struct led_rgb *expected)
{
	zassert_equal(strip_data.calls, 1U);
	zassert_equal(strip_data.count, NUM_PIXELS);
	for (size_t i = 0; i < NUM_PIXELS; i++) {
		zassert_equal(strip_data.pixels[i].r, expected[i].r, "red at pixel %zu", i);
		zassert_equal(strip_data.pixels[i].g, expected[i].g, "green at pixel %zu", i);
		zassert_equal(strip_data.pixels[i].b, expected[i].b, "blue at pixel %zu", i);
	}
}

static void before(void *fixture)
{
	uint32_t clear[NUM_PIXELS] = {0};

	ARG_UNUSED(fixture);
	memset(&strip_data, 0, sizeof(strip_data));
	zassert_true(device_is_ready(display));
	zassert_ok(display_write(display, 0, 0, &full_frame, clear));
	memset(&strip_data, 0, sizeof(strip_data));
}

ZTEST(led_strip_matrix, test_device_ready)
{
	zassert_true(device_is_ready(display));
	zassert_true(device_is_ready(DEVICE_DT_GET(STRIP_NODE)));
}

ZTEST(led_strip_matrix, test_capabilities)
{
	struct display_capabilities caps = {0};

	display_get_capabilities(display, &caps);
	zassert_equal(caps.x_resolution, MATRIX_WIDTH);
	zassert_equal(caps.y_resolution, MATRIX_HEIGHT);
	zassert_equal(caps.supported_pixel_formats, PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_ARGB_8888);
	zassert_equal(caps.current_pixel_format, PIXEL_FORMAT);
	zassert_equal(caps.screen_info, 0U);
}

ZTEST(led_strip_matrix, test_write_full)
{
	struct pixel_buffer input;

	fill_frame(&input, false);
	zassert_ok(display_write(display, 0, 0, &full_frame, &input.pixels));
	assert_strip(colors);
}

ZTEST(led_strip_matrix, test_write_padded)
{
	struct pixel_buffer input;
	struct led_rgb expected[NUM_PIXELS] = {0};
	const struct display_buffer_descriptor desc = {
		.buf_size = 3U * MATRIX_HEIGHT * BYTES_PER_PIXEL,
		.width = 2,
		.height = MATRIX_HEIGHT,
		.pitch = 3,
	};

	memset(&input, 0xA5, sizeof(input));
	for (size_t y = 0; y < desc.height; y++) {
		for (size_t x = 0; x < desc.width; x++) {
			size_t color = y * MATRIX_WIDTH + x;

			set_pixel(&input, y * desc.pitch + x, color, false);
			expected[color] = colors[color];
		}
	}
	zassert_ok(display_write(display, 0, 0, &desc, &input.pixels));
	assert_strip(expected);
}

ZTEST(led_strip_matrix, test_write_offset)
{
	struct pixel_buffer input;
	struct led_rgb expected[NUM_PIXELS] = {0};
	const struct display_buffer_descriptor desc = {
		.buf_size = 2U * BYTES_PER_PIXEL, .width = 2, .height = 1, .pitch = 2,
	};

	memset(&input, 0xA5, sizeof(input));
	set_pixel(&input, 0, 0, false);
	set_pixel(&input, 1, 1, false);
	expected[MATRIX_WIDTH + 1U] = colors[0];
	expected[MATRIX_WIDTH + 2U] = colors[1];
	zassert_ok(display_write(display, 1, 1, &desc, &input.pixels));
	assert_strip(expected);
}

ZTEST(led_strip_matrix, test_read_full)
{
	struct pixel_buffer input;
	struct pixel_buffer output;
	struct pixel_buffer expected;

	fill_frame(&input, false);
	fill_frame(&expected, true);
	memset(&output, 0xA5, sizeof(output));
	zassert_ok(display_write(display, 0, 0, &full_frame, &input.pixels));
	zassert_ok(display_read(display, 0, 0, &full_frame, &output.pixels));
	zassert_mem_equal(&output, &expected, sizeof(output));
}

ZTEST(led_strip_matrix, test_read_padded)
{
	struct pixel_buffer input;
	struct pixel_buffer output;
	struct pixel_buffer expected;
	const struct display_buffer_descriptor desc = {
		.buf_size = 3U * MATRIX_HEIGHT * BYTES_PER_PIXEL,
		.width = 2,
		.height = MATRIX_HEIGHT,
		.pitch = 3,
	};

	fill_frame(&input, false);
	memset(&output, 0xA5, sizeof(output));
	memset(&expected, 0xA5, sizeof(expected));
	for (size_t y = 0; y < desc.height; y++) {
		for (size_t x = 0; x < desc.width; x++) {
			set_pixel(&expected, y * desc.pitch + x, y * MATRIX_WIDTH + x, true);
		}
	}
	zassert_ok(display_write(display, 0, 0, &full_frame, &input.pixels));
	zassert_ok(display_read(display, 0, 0, &desc, &output.pixels));
	zassert_mem_equal(&output, &expected, sizeof(output));
}

ZTEST(led_strip_matrix, test_read_offset)
{
	struct pixel_buffer input;
	struct pixel_buffer output;
	struct pixel_buffer expected;
	const struct display_buffer_descriptor desc = {
		.buf_size = 2U * BYTES_PER_PIXEL, .width = 2, .height = 1, .pitch = 2,
	};

	fill_frame(&input, false);
	memset(&output, 0xA5, sizeof(output));
	memset(&expected, 0xA5, sizeof(expected));
	set_pixel(&expected, 0, MATRIX_WIDTH + 1U, true);
	set_pixel(&expected, 1, MATRIX_WIDTH + 2U, true);
	zassert_ok(display_write(display, 0, 0, &full_frame, &input.pixels));
	zassert_ok(display_read(display, 1, 1, &desc, &output.pixels));
	zassert_mem_equal(&output, &expected, sizeof(output));
}

ZTEST(led_strip_matrix, test_single_pixel)
{
	struct pixel_buffer input;
	struct pixel_buffer output;
	struct pixel_buffer expected;
	struct led_rgb expected_strip[NUM_PIXELS] = {0};
	const struct display_buffer_descriptor desc = {
		.buf_size = BYTES_PER_PIXEL, .width = 1, .height = 1, .pitch = 1,
	};

	memset(&input, 0xA5, sizeof(input));
	memset(&output, 0xA5, sizeof(output));
	memset(&expected, 0xA5, sizeof(expected));
	set_pixel(&input, 0, NUM_PIXELS - 1U, false);
	set_pixel(&expected, 0, NUM_PIXELS - 1U, true);
	expected_strip[NUM_PIXELS - 1U] = colors[NUM_PIXELS - 1U];
	zassert_ok(display_write(display, MATRIX_WIDTH - 1U, MATRIX_HEIGHT - 1U, &desc,
				 &input.pixels));
	assert_strip(expected_strip);
	zassert_ok(display_read(display, MATRIX_WIDTH - 1U, MATRIX_HEIGHT - 1U, &desc,
				&output.pixels));
	zassert_mem_equal(&output, &expected, sizeof(output));
}

ZTEST(led_strip_matrix, test_write_error)
{
	struct pixel_buffer input;

	fill_frame(&input, false);
	strip_data.result = -EIO;
	zassert_equal(display_write(display, 0, 0, &full_frame, &input.pixels), -EIO);
	assert_strip(colors);
}

ZTEST_SUITE(led_strip_matrix, NULL, NULL, before, NULL, NULL);
