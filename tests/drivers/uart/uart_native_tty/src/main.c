/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Tests for the host side of the native_sim TTY UART driver. */

#include <stdio.h>
#include <string.h>

#include <zephyr/ztest.h>

#include <nsi_errno.h>

#include "pty_bottom.h"
#include "sysfs_bottom.h"
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

#define FTDI_FT232R   0x0403, 0x6001
#define FTDI_FT230X   0x0403, 0x6015
#define SILABS_CP210X 0x10c4, 0xea60

static char sysfs[256];
static char tty_class[300];
static char found[32];

static void sysfs_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(native_tty_test_sysfs_create(sysfs, sizeof(sysfs)));
	snprintf(tty_class, sizeof(tty_class), "%s/class", sysfs);
	memset(found, 0, sizeof(found));
}

static void sysfs_after(void *fixture)
{
	ARG_UNUSED(fixture);

	native_tty_test_sysfs_remove(sysfs);
}

static void add_tty(const char *name, int vendor, int product)
{
	zassert_ok(native_tty_test_sysfs_add_tty(sysfs, name, vendor, product));
}

ZTEST(uart_native_tty_usb, test_finds_match)
{
	static const uint32_t ids[] = {FTDI_FT232R};

	add_tty("ttyS0", -1, 0);
	add_tty("ttyUSB0", SILABS_CP210X);
	add_tty("ttyUSB1", FTDI_FT232R);

	zassert_ok(native_tty_find_usb_bottom(tty_class, ids, 1, found, sizeof(found)));
	zassert_str_equal(found, "/dev/ttyUSB1");
}

ZTEST(uart_native_tty_usb, test_pairs_in_order)
{
	static const uint32_t ids[] = {SILABS_CP210X, FTDI_FT232R};

	add_tty("ttyUSB0", FTDI_FT232R);
	add_tty("ttyUSB5", SILABS_CP210X);

	zassert_ok(native_tty_find_usb_bottom(tty_class, ids, 2, found, sizeof(found)));
	zassert_str_equal(found, "/dev/ttyUSB5");
}

ZTEST(uart_native_tty_usb, test_lowest_name_by_number)
{
	static const uint32_t ids[] = {FTDI_FT232R};

	add_tty("ttyUSB10", FTDI_FT232R);
	add_tty("ttyUSB2", FTDI_FT232R);

	zassert_ok(native_tty_find_usb_bottom(tty_class, ids, 1, found, sizeof(found)));
	zassert_str_equal(found, "/dev/ttyUSB2");
}

ZTEST(uart_native_tty_usb, test_product_must_match)
{
	static const uint32_t ids[] = {FTDI_FT232R};

	add_tty("ttyUSB0", FTDI_FT230X);

	zassert_equal(native_tty_find_usb_bottom(tty_class, ids, 1, found, sizeof(found)),
		      -NSI_ERRNO_MID_ENODEV);
}

ZTEST(uart_native_tty_usb, test_no_usb_tty)
{
	static const uint32_t ids[] = {FTDI_FT232R};

	add_tty("ttyS0", -1, 0);

	zassert_equal(native_tty_find_usb_bottom(tty_class, ids, 1, found, sizeof(found)),
		      -NSI_ERRNO_MID_ENODEV);
}

ZTEST(uart_native_tty_usb, test_path_too_long)
{
	static const uint32_t ids[] = {FTDI_FT232R};

	add_tty("ttyUSB0", FTDI_FT232R);

	zassert_equal(
		native_tty_find_usb_bottom(tty_class, ids, 1, found, sizeof("/dev/ttyUSB0") - 1),
		-NSI_ERRNO_MID_ENAMETOOLONG);
}

ZTEST(uart_native_tty_usb, test_missing_class)
{
	static const uint32_t ids[] = {FTDI_FT232R};

	strcat(tty_class, "/missing");
	zassert_equal(native_tty_find_usb_bottom(tty_class, ids, 1, found, sizeof(found)),
		      -NSI_ERRNO_MID_ENOENT);
}

ZTEST_SUITE(uart_native_tty_usb, NULL, NULL, sysfs_before, sysfs_after, NULL);
