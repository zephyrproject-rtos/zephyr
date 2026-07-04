/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/__assert.h>

#include "harness.h"

ZTEST(zassert_deprecated, test_assert_pass_is_noop)
{
	HARNESS_EXPECT(__ASSERT(1 == 1, "no %d", harness_counting_arg()));
	zassert_false(harness_fired(), "passing __ASSERT fired");
	zassert_false(harness_printed(), "passing __ASSERT printed");
	zassert_equal(harness_arg_evals(), 0, "passing __ASSERT evaluated its message args");
}

ZTEST(zassert_deprecated, test_assert_fail_fires)
{
	HARNESS_EXPECT(__ASSERT(1 == 2, "boom"));

	switch (CONFIG_ASSERT_MODULE_DEFAULT_LEVEL) {
	case ZASSERT_LEVEL_OFF:
		zassert_false(harness_fired(), "OFF must be compiled out");
		zassert_false(harness_printed(), "OFF must not print");
		break;
	case ZASSERT_LEVEL_TERSE:
		zassert_true(harness_fired(), "TERSE did not fire");
		zassert_true(harness_contains("ASSERTION FAIL"), "banner not printed at TERSE");
		zassert_false(harness_contains(" @ "), "location printed at TERSE");
		zassert_false(harness_contains("1 == 2"), "condition printed at TERSE");
		break;
	case ZASSERT_LEVEL_NORMAL:
		zassert_true(harness_fired(), "NORMAL did not fire");
		zassert_true(harness_contains("ASSERTION FAIL @ "),
			     "location not printed at NORMAL");
		zassert_false(harness_contains("1 == 2"), "condition printed at NORMAL");
		break;
	default:
		zassert_true(harness_fired(), "VERBOSE did not fire");
		zassert_true(harness_contains("1 == 2"), "condition not printed at VERBOSE");
		break;
	}
}

ZTEST(zassert_deprecated, test_assert_fail_message_args)
{
	HARNESS_EXPECT(__ASSERT(1 == 2, "boom %d", harness_counting_arg()));

	if (CONFIG_ASSERT_MODULE_DEFAULT_LEVEL == ZASSERT_LEVEL_VERBOSE) {
		zassert_true(harness_contains("boom 1"), "message not printed at VERBOSE");
		zassert_equal(harness_arg_evals(), 1, "message args not evaluated at VERBOSE");
	} else {
		zassert_false(harness_contains("boom"), "message printed below VERBOSE");
		zassert_equal(harness_arg_evals(), 0, "message args evaluated below VERBOSE");
	}
}

ZTEST(zassert_deprecated, test_assert_no_msg_pass_is_noop)
{
	HARNESS_EXPECT(__ASSERT_NO_MSG(1 == 1));
	zassert_false(harness_fired(), "passing __ASSERT_NO_MSG fired");
	zassert_false(harness_printed(), "passing __ASSERT_NO_MSG printed");
}

ZTEST(zassert_deprecated, test_assert_no_msg_fail_fires)
{
	HARNESS_EXPECT(__ASSERT_NO_MSG(1 == 2));

	switch (CONFIG_ASSERT_MODULE_DEFAULT_LEVEL) {
	case ZASSERT_LEVEL_OFF:
		zassert_false(harness_fired(), "OFF must be compiled out");
		zassert_false(harness_printed(), "OFF must not print");
		break;
	case ZASSERT_LEVEL_TERSE:
		zassert_true(harness_fired(), "TERSE did not fire");
		zassert_true(harness_contains("ASSERTION FAIL"), "banner not printed at TERSE");
		zassert_false(harness_contains(" @ "), "location printed at TERSE");
		break;
	case ZASSERT_LEVEL_NORMAL:
		zassert_true(harness_fired(), "NORMAL did not fire");
		zassert_true(harness_contains("ASSERTION FAIL @ "),
			     "location not printed at NORMAL");
		break;
	default:
		zassert_true(harness_fired(), "VERBOSE did not fire");
		zassert_true(harness_contains("ASSERTION FAIL ["), "banner not printed at VERBOSE");
		zassert_true(harness_contains("1 == 2"), "condition not printed at VERBOSE");
		break;
	}
}

ZTEST_SUITE(zassert_deprecated, NULL, NULL, harness_reset, harness_reset, NULL);
