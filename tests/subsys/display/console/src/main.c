/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/display/display_console.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/ztest.h>

#include "fake_display.h"

/* Long enough for the work queue to draw what was written. */
#define SETTLE K_MSEC(CONFIG_DISPLAY_CONSOLE_FLUSH_DELAY_MS + 100)

static void write_str(const char *str)
{
	display_console_write((const uint8_t *)str, strlen(str));
}

/* Runs first: before a fatal error the work queue draws the output. */
ZTEST(display_console, test_1_deferred_draw)
{
	uint32_t before;

	zassert_true(display_console_ready(), "console not attached");
	k_sleep(SETTLE);

	before = fake_display_writes();
	write_str("deferred\n");
	zassert_equal(fake_display_writes(), before, "line drawn before the flush delay");

	k_sleep(SETTLE);
	zassert_true(fake_display_writes() > before, "line never drawn");
}

/* Runs last: the logging panic hook makes every completed line draw at once. */
ZTEST(display_console, test_2_panic_draw)
{
	uint32_t before;

	k_sleep(SETTLE);
	LOG_PANIC();

	before = fake_display_writes();
	write_str("partial");
	zassert_equal(fake_display_writes(), before, "incomplete line drawn");

	write_str(" line\n");
	zassert_true(fake_display_writes() > before, "line not drawn before the write returned");
}

ZTEST_SUITE(display_console, NULL, NULL, NULL, NULL, NULL);
