/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Tests for the host side of the native_sim TTY UART driver. */

#include <zephyr/ztest.h>

#include <nsi_errno.h>

#include "pty_bottom.h"
#include "uart_native_tty_bottom.h"

struct native_tty_test_pty {
	int controller;
	int terminal;
};

static struct native_tty_test_pty pty_open(void)
{
	char path[64];
	struct native_tty_test_pty pty;

	pty.controller = native_tty_test_open_pty(path, sizeof(path));
	zassert_true(pty.controller >= 0, "cannot open a pseudo terminal");
	pty.terminal = native_tty_open_tty_bottom(path);

	return pty;
}

ZTEST(uart_native_tty, test_poll_idle)
{
	struct native_tty_test_pty pty = pty_open();

	zassert_equal(native_tty_poll_bottom(pty.terminal), 0);

	native_tty_test_close(pty.terminal);
	native_tty_test_close(pty.controller);
}

ZTEST(uart_native_tty, test_poll_data)
{
	struct native_tty_test_pty pty = pty_open();

	/* The terminal side is in canonical mode: a line becomes readable as a whole. */
	zassert_ok(native_tty_test_write(pty.controller, "x\n", 2));
	native_tty_test_wait_readable(pty.terminal);
	zassert_equal(native_tty_poll_bottom(pty.terminal), 1);

	native_tty_test_close(pty.terminal);
	native_tty_test_close(pty.controller);
}

ZTEST(uart_native_tty, test_poll_hangup)
{
	struct native_tty_test_pty pty = pty_open();

	native_tty_test_close(pty.controller);
	zassert_equal(native_tty_poll_bottom(pty.terminal), -NSI_ERRNO_MID_EIO);

	native_tty_test_close(pty.terminal);
}

ZTEST(uart_native_tty, test_poll_closed)
{
	struct native_tty_test_pty pty = pty_open();

	native_tty_test_close(pty.terminal);
	zassert_equal(native_tty_poll_bottom(pty.terminal), -NSI_ERRNO_MID_EBADF);

	native_tty_test_close(pty.controller);
}

ZTEST_SUITE(uart_native_tty, NULL, NULL, NULL, NULL, NULL);
