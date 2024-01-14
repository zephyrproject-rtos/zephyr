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

LOG_MODULE_REGISTER(clear, CONFIG_DISPLAY_LOG_LEVEL);

static void *cfb_test_init(void)
{
	if (!display_initialized) {
		cfb_framebuffer_init(dev);
		display_initialized = true;
	}

	return NULL;
}

/**
 * Fill the buffer with 0xAA before running tests.
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

	memset(read_buffer, 0xAA, sizeof(read_buffer));
	display_write(dev, 0, 0, &desc, read_buffer);

	display_blanking_off(dev);
}

ZTEST(clear, test_clear_false)
{
	zassert_ok(cfb_framebuffer_clear(dev, false));

	/* checking memory not updated */
	zassert_false(verify_color_inside_rect(0, 0, 320, 240, 0x0));
}

ZTEST(clear, test_clear_true)
{
	zassert_ok(cfb_framebuffer_clear(dev, true));

	zassert_true(verify_color_inside_rect(0, 0, 320, 240, 0x0));
}

ZTEST_SUITE(clear, NULL, cfb_test_init, cfb_test_before, NULL, NULL);
