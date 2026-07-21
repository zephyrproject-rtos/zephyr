/*
 * Copyright 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_backend.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/shell/shell_uart.h>

ZTEST(shell_backend, test_backend_apis)
{
	const struct shell *sh_dummy = shell_backend_get_by_name("shell_dummy");
	const struct shell *sh_uart = shell_backend_get_by_name("shell_uart");

	zassert_equal(shell_backend_count_get(), 2, "Expecting 2, got %d",
		      shell_backend_count_get());

	zassert_equal_ptr(sh_dummy, shell_backend_dummy_get_ptr(),
			  "Unexpected shell_dummy backend");
	zassert_equal_ptr(sh_uart, shell_backend_uart_get_ptr(), "Unexpected shell_uart backend");
	zassert_equal_ptr(shell_backend_get_by_name("blah"), NULL, "Should be NULL if not found");

	zassert_equal_ptr(shell_backend_get(0), sh_dummy < sh_uart ? sh_dummy : sh_uart,
			  "Unexpected backend at index 0");
	zassert_equal_ptr(shell_backend_get(1), sh_dummy < sh_uart ? sh_uart : sh_dummy,
			  "Unexpected backend at index 1");
}

static void *shell_setup(void)
{
	/* Let the shell backend initialize. */
	k_usleep(10);
	return NULL;
}

ZTEST_SUITE(shell_backend, NULL, shell_setup, NULL, NULL, NULL);
