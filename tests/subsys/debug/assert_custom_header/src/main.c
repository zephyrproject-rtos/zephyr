/*
 * Copyright (c) 2025 Google, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Custom header assert test suite
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/sys/__assert.h>

static unsigned int last_assert_line;

void assert_post_action(const char *file, unsigned int line)
{
	last_assert_line = line;
}

ZTEST_SUITE(assert, NULL, NULL, NULL, NULL, NULL);

ZTEST(assert, test_assert_call)
{
	/* Because CONFIG_ASSERT_TEST is not enabled in this test, and we're using a
	 * custom assert macro, this should not crash and the test should proceed.
	 */
	__ASSERT(false, "This is a custom assert test");

	zassert_true(last_assert_line != 0, "Assert was called on line 0");
}
