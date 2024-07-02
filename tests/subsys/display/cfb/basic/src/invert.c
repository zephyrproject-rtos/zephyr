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

static struct cfb_display *disp;
static struct cfb_framebuffer *fb;

/**
 * Fill the buffer with 0 before running tests.
 */
static void cfb_test_before(void *text_fixture)
{
	disp = display_init();
	fb = cfb_display_get_framebuffer(disp);
}

static void cfb_test_after(void *test_fixture)
{
	display_deinit(disp);
}

ZTEST(invert, test_invert)
{
	zassert_ok(cfb_invert(fb));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_inside_rect(0, 0, 320, 240, 0xFFFFFF));
}

ZTEST(invert, test_invert_contents)
{
	zassert_ok(cfb_invert_area(fb, 10, 10, 10, 10));
	zassert_ok(cfb_finalize(fb));
	zassert_true(verify_color_outside_rect(10, 10, 10, 10, 0));
	zassert_true(verify_color_inside_rect(10, 10, 10, 10, 0xFFFFFF));

	zassert_ok(cfb_invert(fb));
	zassert_ok(cfb_finalize(fb));

	zassert_true(verify_color_outside_rect(10, 10, 10, 10, 0xFFFFFF));
}

ZTEST_SUITE(invert, NULL, NULL, cfb_test_before, cfb_test_after, NULL);
