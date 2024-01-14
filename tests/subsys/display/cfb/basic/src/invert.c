/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/display.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/display/cfb.h>

#include "testdata.h"
#include "utils.h"

LOG_MODULE_REGISTER(invert, CONFIG_DISPLAY_LOG_LEVEL);

static void *cfb_test_init(void)
{
	if (!display_initialized) {
		cfb_framebuffer_init(dev);
		display_initialized = true;
	}

	return NULL;
}

/**
 * Fill the buffer with 0 before running tests.
 */
static void cfb_test_before(void *text_fixture)
{
	display_get_capabilities(dev, &display_caps);

	struct display_buffer_descriptor desc = {
		.height = display_height,
		.pitch = display_width,
		.width = display_width,
		.buf_size = display_height * display_width / 8,
	};

	memset(read_buffer, 0, sizeof(read_buffer));
	display_write(dev, 0, 0, &desc, read_buffer);

	display_blanking_off(dev);

	cfb_framebuffer_clear(dev, false);
}

ZTEST(invert, test_invert)
{
	int err;
	bool result = false;

	zassert_ok(cfb_framebuffer_invert(dev));
	err = cfb_framebuffer_finalize(dev);

	if (err) {
		goto error;
	}

	result = verify_color_inside_rect(0, 0, 320, 240, 0xFFFFFF);
error:
	/* Need restore invert status */
	cfb_framebuffer_invert(dev);

	zassert_ok(err, "error");
	zassert_true(result, "check failed");
}

ZTEST(invert, test_invert_contents)
{
	int err;
	bool result = false;

	zassert_ok(cfb_invert_area(dev, 10, 10, 10, 10));
	zassert_ok(cfb_framebuffer_finalize(dev));
	zassert_true(verify_color_outside_rect(10, 10, 10, 10, 0));
	zassert_true(verify_color_inside_rect(10, 10, 10, 10, 0xFFFFFF));

	zassert_ok(cfb_framebuffer_invert(dev));
	err = cfb_framebuffer_finalize(dev);
	if (err) {
		goto error;
	}

	result = verify_color_outside_rect(10, 10, 10, 10, 0xFFFFFF);
	if (result) {
		result = verify_color_inside_rect(10, 10, 10, 10, 0x0);
	}

error:
	/* Need restore invert status */
	cfb_framebuffer_invert(dev);

	zassert_ok(err, "error");
	zassert_true(result, "check failed");
}

ZTEST_SUITE(invert, NULL, cfb_test_init, cfb_test_before, NULL, NULL);
