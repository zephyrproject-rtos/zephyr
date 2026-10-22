/*
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ztest suite for the charlieplex LED matrix display driver. Runs on native_sim
 * (emulated GPIO + software counter) so it executes under twister / CI with no
 * hardware. Exercises the deterministic, hardware-independent surface of the
 * display API: capabilities, write->framebuffer mapping, blanking, brightness.
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

#define MATRIX_W   2
#define MATRIX_H   2
#define NUM_PIXELS (MATRIX_W * MATRIX_H)

static const struct device *disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

ZTEST(charlieplex, test_device_ready)
{
	zassert_true(device_is_ready(disp), "display device not ready");
}

ZTEST(charlieplex, test_capabilities)
{
	struct display_capabilities caps;

	display_get_capabilities(disp, &caps);
	zassert_equal(caps.x_resolution, MATRIX_W, "wrong x resolution");
	zassert_equal(caps.y_resolution, MATRIX_H, "wrong y resolution");
	zassert_true(caps.supported_pixel_formats & PIXEL_FORMAT_MONO01, "MONO01 not advertised");
	zassert_equal(caps.current_pixel_format, PIXEL_FORMAT_MONO01, "wrong current format");
}

/* A full-on MONO01 write should set every framebuffer cell to max brightness;
 * a full-off write should clear them. The driver's framebuffer is 1 byte/pixel.
 */
ZTEST(charlieplex, test_write_all_on_off)
{
	uint8_t *fb = display_get_framebuffer(disp);
	struct display_buffer_descriptor desc = {
		.buf_size = sizeof(uint8_t) * ((MATRIX_W + 7) / 8) * MATRIX_H,
		.width = MATRIX_W,
		.height = MATRIX_H,
		.pitch = ((MATRIX_W + 7) / 8) * 8,
		.frame_incomplete = false,
	};
	uint8_t on[((MATRIX_W + 7) / 8) * MATRIX_H];
	uint8_t off[((MATRIX_W + 7) / 8) * MATRIX_H];

	zassert_not_null(fb, "framebuffer is NULL");

	memset(on, 0xFF, sizeof(on));
	memset(off, 0x00, sizeof(off));

	zassert_ok(display_write(disp, 0, 0, &desc, on), "write on failed");
	for (int i = 0; i < NUM_PIXELS; i++) {
		zassert_true(fb[i] > 0, "pixel %d should be lit", i);
	}

	zassert_ok(display_write(disp, 0, 0, &desc, off), "write off failed");
	for (int i = 0; i < NUM_PIXELS; i++) {
		zassert_equal(fb[i], 0, "pixel %d should be dark", i);
	}
}

/* Writing a single set bit must light exactly the corresponding framebuffer
 * cell (row-major), proving the write x/y -> framebuffer index mapping.
 */
ZTEST(charlieplex, test_write_single_pixel)
{
	uint8_t *fb = display_get_framebuffer(disp);
	struct display_buffer_descriptor desc = {
		.buf_size = ((MATRIX_W + 7) / 8) * MATRIX_H,
		.width = MATRIX_W,
		.height = MATRIX_H,
		.pitch = ((MATRIX_W + 7) / 8) * 8,
		.frame_incomplete = false,
	};
	uint8_t clear[((MATRIX_W + 7) / 8) * MATRIX_H] = {0};
	/* Pixel (x=1,y=1) -> framebuffer index 3. MONO01 row-packed: row1 byte,
	 * bit 1 set.
	 */
	uint8_t one[((MATRIX_W + 7) / 8) * MATRIX_H] = {0};

	one[1 * ((MATRIX_W + 7) / 8) + 0] = BIT(1);

	zassert_ok(display_write(disp, 0, 0, &desc, clear), "clear failed");
	zassert_ok(display_write(disp, 0, 0, &desc, one), "write one failed");

	for (int i = 0; i < NUM_PIXELS; i++) {
		if (i == 3) {
			zassert_true(fb[i] > 0, "target pixel not lit");
		} else {
			zassert_equal(fb[i], 0, "pixel %d should be dark", i);
		}
	}
}

ZTEST(charlieplex, test_write_out_of_bounds)
{
	struct display_buffer_descriptor desc = {
		.buf_size = 4,
		.width = MATRIX_W,
		.height = MATRIX_H,
		.pitch = ((MATRIX_W + 7) / 8) * 8,
		.frame_incomplete = false,
	};
	uint8_t buf[4] = {0};

	/* origin beyond the panel must be rejected */
	zassert_not_ok(display_write(disp, MATRIX_W, 0, &desc, buf), "out-of-bounds x accepted");
	zassert_not_ok(display_write(disp, 0, MATRIX_H, &desc, buf), "out-of-bounds y accepted");
}

ZTEST(charlieplex, test_blanking)
{
	/* Blanking on/off should be supported and return success. */
	zassert_ok(display_blanking_on(disp), "blanking_on failed");
	zassert_ok(display_blanking_off(disp), "blanking_off failed");
}

ZTEST(charlieplex, test_pixel_format)
{
	zassert_ok(display_set_pixel_format(disp, PIXEL_FORMAT_MONO01),
		   "MONO01 should be accepted");
	zassert_not_ok(display_set_pixel_format(disp, PIXEL_FORMAT_RGB_888),
		       "RGB_888 should be rejected");
}

ZTEST_SUITE(charlieplex, NULL, NULL, NULL, NULL, NULL);
