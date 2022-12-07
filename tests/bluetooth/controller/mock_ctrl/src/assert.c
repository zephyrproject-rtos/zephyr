/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include "assert.h"

void assert_print(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintk(fmt, ap);
	va_end(ap);
}

void assert_post_action(const char *file, unsigned int line)
{
	/*
	 * Todo: implement FFF functionality, see the host unit tests for reference
	 */
	ztest_test_fail();
}
