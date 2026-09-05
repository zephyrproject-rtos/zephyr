/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/zassert.h>

#include "harness.h"

ZASSERT_MODULE(MYMODULE);

ZTEST(zassert_mymodule, test_passing_assert_is_noop)
{
	HARNESS_EXPECT(ZASSERT(1 == 1, "must not fire %d", harness_counting_arg()));
	zassert_false(harness_fired(), "passing ZASSERT fired");
	zassert_false(harness_printed(), "passing ZASSERT printed");
	zassert_equal(harness_arg_evals(), 0, "passing ZASSERT evaluated its message args");
}

ZTEST(zassert_mymodule, test_failing_assert_fires)
{
	HARNESS_EXPECT(ZASSERT(1 == 2, "boom"));

	switch (CONFIG_ASSERT_MODULE_MYMODULE_LEVEL) {
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

ZTEST(zassert_mymodule, test_failing_assert_message)
{
	HARNESS_EXPECT(ZASSERT(1 == 2, "boom %d", harness_counting_arg()));

	if (CONFIG_ASSERT_MODULE_MYMODULE_LEVEL == ZASSERT_LEVEL_VERBOSE) {
		zassert_true(harness_contains("boom 1"), "message not printed at VERBOSE");
		zassert_equal(harness_arg_evals(), 1, "message args not evaluated at VERBOSE");
	} else {
		zassert_false(harness_contains("boom"), "message printed below VERBOSE");
		zassert_equal(harness_arg_evals(), 0, "message args evaluated below VERBOSE");
	}
}

ZTEST(zassert_mymodule, test_failing_assert_no_message)
{
	HARNESS_EXPECT(ZASSERT(1 == 2));

	switch (CONFIG_ASSERT_MODULE_MYMODULE_LEVEL) {
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

ZTEST_SUITE(zassert_mymodule, NULL, NULL, harness_reset, harness_reset, NULL);
