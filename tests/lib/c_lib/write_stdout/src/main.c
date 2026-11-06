/*
 * Copyright (c) 2026 Ziyou Wang
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/posix/unistd.h>

#define STDOUT_FD 1

static char recorded[32];
static size_t recorded_len;

static int record_hook(int c)
{
	if (recorded_len < sizeof(recorded)) {
		recorded[recorded_len++] = (char)c;
	}

	return c;
}

static void expect_write_passthrough(const char *msg, size_t len)
{
	recorded_len = 0;

	zassert_equal(write(STDOUT_FD, msg, len), len, "write() returned an unexpected value");
	zassert_equal(recorded_len, len, "unexpected byte count for %zu bytes: %zu", len,
		      recorded_len);
	zassert_mem_equal(recorded, msg, len, "bytes must be passed through untouched");
}

/*
 * Newline translation belongs to the console backend. The libc adapters must
 * pass the bytes through unchanged, otherwise a backend that already converts
 * LF to CRLF, such as the UART console, ends up emitting CR CR LF.
 *
 * The POSIX write() path is used because zephyr_write_stdout() is only declared
 * for newlib and arcmwdt, while write() reaches the same implementation through
 * zvfs for every libc that provides it.
 */
ZTEST(write_stdout, test_plain_text)
{
	expect_write_passthrough("abc", 3);
}

ZTEST(write_stdout, test_single_newline)
{
	expect_write_passthrough("\n", 1);
}

ZTEST(write_stdout, test_multiple_lines)
{
	expect_write_passthrough("A\nB\nC\n", 6);
}

ZTEST(write_stdout, test_no_trailing_newline)
{
	expect_write_passthrough("A\nB", 3);
}

ZTEST(write_stdout, test_existing_crlf)
{
	expect_write_passthrough("A\r\nB", 4);
}

static void write_stdout_before(void *fixture)
{
	ARG_UNUSED(fixture);

	__stdout_hook_install(record_hook);
}

ZTEST_SUITE(write_stdout, NULL, NULL, write_stdout_before, NULL, NULL);
