/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include "assert.h"

void assert_print(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintk(fmt, ap);
	va_end(ap);
}

bool mock_check_if_assert_expected(void)
{
	/* This will fail the test run if ztest_returns_value() hasn't been
	 * called (i.e. the test doesn't expect an assert).
	 */
	return ztest_get_return_value();
}

void assert_post_action(const char *file, unsigned int line)
{
	/* If this is an unexpected assert (i.e. not following expect_assert())
	 * calling mock_check_if_assert_expected() will terminate the test and
	 * mark it as failed.
	 */
	if (mock_check_if_assert_expected() == true) {
		printk("Assertion expected as part of a test case.\n");
		/* This will mark the test as passed and stop execution:
		 * Needed in the passing scenario to prevent undefined behavior after hitting the
		 * assert. In real builds (non-UT), the system will be halted by the assert.
		 */
		ztest_test_pass();
	}
}
