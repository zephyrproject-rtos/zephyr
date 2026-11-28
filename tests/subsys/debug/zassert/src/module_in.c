/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ZASSERT_IN() names its module at the call site. This file deliberately
 * selects no module at all, so every check here proves the level comes from the
 * named module rather than from anything in scope.
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/zassert.h>

#include "harness.h"
#include "module_in_macro.h"

ZTEST(zassert_module_in, test_passing_assert_is_noop)
{
	HARNESS_EXPECT(ZASSERT_IN(MYMODULE, 1 == 1, "must not fire %d", harness_counting_arg()));
	zassert_false(harness_fired(), "passing ZASSERT_IN fired");
	zassert_false(harness_printed(), "passing ZASSERT_IN printed");
	zassert_equal(harness_arg_evals(), 0, "passing ZASSERT_IN evaluated its message args");
}

ZTEST(zassert_module_in, test_follows_named_module)
{
	HARNESS_EXPECT(ZASSERT_IN(MYMODULE, 1 == 2, "boom"));

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
		zassert_false(harness_contains("boom"), "message printed at TERSE");
		break;
	case ZASSERT_LEVEL_NORMAL:
		zassert_true(harness_fired(), "NORMAL did not fire");
		zassert_true(harness_contains("ASSERTION FAIL @ "),
			"location not printed at NORMAL");
		zassert_false(harness_contains("1 == 2"), "condition printed at NORMAL");
		zassert_false(harness_contains("boom"), "message printed at NORMAL");
		break;
	default:
		zassert_true(harness_fired(), "VERBOSE did not fire");
		zassert_true(harness_contains("1 == 2"), "condition not printed at VERBOSE");
		zassert_true(harness_contains("boom"), "message not printed at VERBOSE");
		break;
	}
}

ZTEST(zassert_module_in, test_default_module_is_independent)
{
	HARNESS_EXPECT(ZASSERT_IN(DEFAULT, 1 == 2, "legacy"));

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
		zassert_false(harness_contains("legacy"), "message printed at NORMAL");
		break;
	default:
		zassert_true(harness_fired(), "VERBOSE did not fire");
		zassert_true(harness_contains("legacy"), "message not printed at VERBOSE");
		break;
	}
}

ZTEST(zassert_module_in, test_macro_from_header_keeps_its_module)
{
	HARNESS_EXPECT(CHECK_IN_MYMODULE(1 == 2, "from header"));

	if (CONFIG_ASSERT_MODULE_MYMODULE_LEVEL == ZASSERT_LEVEL_OFF) {
		zassert_false(harness_fired(), "OFF must be compiled out");
	} else {
		zassert_true(harness_fired(), "header macro did not fire");
	}

	if (CONFIG_ASSERT_MODULE_MYMODULE_LEVEL == ZASSERT_LEVEL_VERBOSE) {
		zassert_true(harness_contains("from header"), "message not printed at VERBOSE");
	} else {
		zassert_false(harness_contains("from header"), "message printed below VERBOSE");
	}
}

ZTEST(zassert_module_in, test_message_args_are_not_evaluated_below_verbose)
{
	HARNESS_EXPECT(ZASSERT_IN(MYMODULE, 1 == 2, "boom %d", harness_counting_arg()));

	if (CONFIG_ASSERT_MODULE_MYMODULE_LEVEL == ZASSERT_LEVEL_VERBOSE) {
		zassert_true(harness_contains("boom 1"), "message not printed at VERBOSE");
		zassert_equal(harness_arg_evals(), 1, "message args not evaluated at VERBOSE");
	} else {
		zassert_equal(harness_arg_evals(), 0, "message args evaluated below VERBOSE");
	}
}

ZTEST(zassert_module_in, test_no_message)
{
	HARNESS_EXPECT(ZASSERT_IN(MYMODULE, 1 == 2));

	if (CONFIG_ASSERT_MODULE_MYMODULE_LEVEL == ZASSERT_LEVEL_OFF) {
		zassert_false(harness_fired(), "OFF must be compiled out");
	} else {
		zassert_true(harness_fired(), "ZASSERT_IN without message did not fire");
	}

	if (CONFIG_ASSERT_MODULE_MYMODULE_LEVEL == ZASSERT_LEVEL_VERBOSE) {
		zassert_true(harness_contains("ASSERTION FAIL ["), "banner not printed at VERBOSE");
		zassert_true(harness_contains("1 == 2"), "condition not printed at VERBOSE");
	}
}

ZTEST_SUITE(zassert_module_in, NULL, NULL, harness_reset, harness_reset, NULL);
