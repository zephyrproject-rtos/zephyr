/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/zassert.h>

#include "harness.h"

ZASSERT_GROUP(DEFAULT);

ZTEST(zassert_default, test_passing_assert_is_noop)
{
	HARNESS_EXPECT(ZASSERT(1 == 1, "must not fire %d", harness_counting_arg()));
	zassert_false(harness_fired(), "passing ZASSERT fired");
	zassert_false(harness_printed(), "passing ZASSERT printed");
	zassert_equal(harness_arg_evals(), 0, "passing ZASSERT evaluated its message args");
}

ZTEST(zassert_default, test_failing_assert_fires)
{
	HARNESS_EXPECT(ZASSERT(1 == 2, "boom"));

	if (CONFIG_DEFAULT_ZASSERT_LEVEL == ZASSERT_OFF) {
		zassert_false(harness_fired(), "OFF must be compiled out");
		zassert_false(harness_printed(), "OFF must not print");
	} else {
		zassert_true(harness_fired(), "assert did not fire");
		zassert_true(harness_contains("ASSERTION FAIL"), "location not printed");
	}
}

ZTEST(zassert_default, test_failing_assert_message)
{
	HARNESS_EXPECT(ZASSERT(1 == 2, "boom %d", harness_counting_arg()));

	if (CONFIG_DEFAULT_ZASSERT_LEVEL == ZASSERT_VERBOSE) {
		zassert_true(harness_contains("boom 1"), "message not printed at VERBOSE");
		zassert_equal(harness_arg_evals(), 1, "message args not evaluated at VERBOSE");
	} else {
		zassert_false(harness_contains("boom"), "message printed below VERBOSE");
		zassert_equal(harness_arg_evals(), 0, "message args evaluated below VERBOSE");
	}
}

ZTEST(zassert_default, test_failing_assert_no_message)
{
	HARNESS_EXPECT(ZASSERT(1 == 2));

	if (CONFIG_DEFAULT_ZASSERT_LEVEL == ZASSERT_OFF) {
		zassert_false(harness_fired(), "OFF must be compiled out");
		zassert_false(harness_printed(), "OFF must not print");
	} else {
		zassert_true(harness_fired(), "no-message assert did not fire");
		zassert_true(harness_contains("ASSERTION FAIL"), "location not printed");
	}
}

ZTEST_SUITE(zassert_default, NULL, NULL, harness_reset, harness_reset, NULL);
