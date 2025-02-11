/*
 * Copyright (c) 2024 Google, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Custom header shell test suite
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>

static void *shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	/* Wait for the initialization of the shell dummy backend. */
	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");

	return NULL;
}

ZTEST_SUITE(sh, NULL, shell_setup, NULL, NULL, NULL);

ZTEST(sh, test_shell_fprintf)
{
	static const char expect[] = "[CUSTOM_PREFIX]testing 1 2 3";
	const struct shell *sh;
	const char *buf;
	size_t size;

	sh = shell_backend_dummy_get_ptr();
	zassert_not_null(sh, "Failed to get shell");

	/* Clear the output buffer */
	shell_backend_dummy_clear_output(sh);

	shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, "testing %d %s %c",
		      1, "2", '3');
	buf = shell_backend_dummy_get_output(sh, &size);
	zassert_true(size >= sizeof(expect), "Expected size > %u, got %d",
		     sizeof(expect), size);

	/*
	 * There are prompts and various ANSI characters in the output, so just
	 * check that the string is in there somewhere.
	 */
	zassert_true(strstr(buf, expect),
		     "Expected string to contain '%s', got '%s'", expect, buf);
}
