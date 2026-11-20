/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/display/display_console.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/ztest.h>

#include "fake_display.h"

/* Long enough for the work queue to draw what was written. */
#define SETTLE K_MSEC(CONFIG_DISPLAY_CONSOLE_FLUSH_DELAY_MS + 100)

static void write_str(const char *str)
{
	display_console_write(str, strlen(str));
}

/* The bytes queued on the receive side of the console UART since the last call. */
static const char *read_rx(const struct device *uart)
{
	static char buf[8];
	size_t n = 0;
	unsigned char c;

	k_sleep(K_MSEC(20));
	while (n < sizeof(buf) - 1U && uart_poll_in(uart, &c) == 0) {
		buf[n++] = (char)c;
	}
	buf[n] = '\0';

	return buf;
}

/* Keyboard front end: key events become terminal bytes on the console UART RX side. */
ZTEST(display_console, test_0_keyboard)
{
	const struct device *uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	static const struct {
		uint16_t code;
		const char *bytes;
	} keys[] = {
		{INPUT_KEY_H, "h"},
		{INPUT_KEY_1, "1"},
		{INPUT_KEY_SPACE, " "},
		{INPUT_KEY_MINUS, "-"},
		{INPUT_KEY_ENTER, "\r"},
		{INPUT_KEY_BACKSPACE, "\b"},
		{INPUT_KEY_TAB, "\t"},
		{INPUT_KEY_ESC, "\x1b"},
		{INPUT_KEY_UP, "\x1b[A"},
		{INPUT_KEY_DOWN, "\x1b[B"},
		{INPUT_KEY_RIGHT, "\x1b[C"},
		{INPUT_KEY_LEFT, "\x1b[D"},
		{INPUT_KEY_HOME, "\x1b[H"},
		{INPUT_KEY_END, "\x1b[F"},
		{INPUT_KEY_DELETE, "\x1b[3~"},
		{INPUT_KEY_KP1, "1"},
		{INPUT_KEY_KPENTER, "\r"},
	};
	static const struct {
		uint16_t modifier;
		uint16_t code;
		const char *bytes;
	} chords[] = {
		{INPUT_KEY_LEFTSHIFT, INPUT_KEY_H, "H"},
		{INPUT_KEY_RIGHTSHIFT, INPUT_KEY_1, "!"},
		{INPUT_KEY_LEFTSHIFT, INPUT_KEY_MINUS, "_"},
		{INPUT_KEY_LEFTCTRL, INPUT_KEY_C, "\x03"},
		{INPUT_KEY_LEFTCTRL, INPUT_KEY_H, "\x08"},
	};

	zassert_true(device_is_ready(uart));

	for (size_t i = 0; i < ARRAY_SIZE(keys); i++) {
		zassert_ok(input_report_key(NULL, keys[i].code, 1, true, K_FOREVER));
		zassert_ok(input_report_key(NULL, keys[i].code, 0, true, K_FOREVER));
		zassert_str_equal(read_rx(uart), keys[i].bytes, "key %u", keys[i].code);
	}

	for (size_t i = 0; i < ARRAY_SIZE(chords); i++) {
		zassert_ok(input_report_key(NULL, chords[i].modifier, 1, true, K_FOREVER));
		zassert_ok(input_report_key(NULL, chords[i].code, 1, true, K_FOREVER));
		zassert_ok(input_report_key(NULL, chords[i].code, 0, true, K_FOREVER));
		zassert_ok(input_report_key(NULL, chords[i].modifier, 0, true, K_FOREVER));
		zassert_str_equal(read_rx(uart), chords[i].bytes, "chord %u+%u", chords[i].modifier,
				  chords[i].code);
	}

	/* A modifier alone and a release produce nothing. */
	zassert_ok(input_report_key(NULL, INPUT_KEY_LEFTSHIFT, 1, true, K_FOREVER));
	zassert_ok(input_report_key(NULL, INPUT_KEY_LEFTSHIFT, 0, true, K_FOREVER));
	zassert_str_equal(read_rx(uart), "");
}

/* Before a fatal error the work queue draws the output. */
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
