/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/zassert.h>

#include "harness.h"
#include "inline_fn.h"

static void expect_mymodule_level(void (*fn)(void), const char *needle)
{
	HARNESS_EXPECT(fn());

	switch (CONFIG_ASSERT_MODULE_MYMODULE_LEVEL) {
	case ZASSERT_OFF:
		zassert_false(harness_fired(), "OFF must be compiled out");
		zassert_false(harness_printed(), "OFF must not print");
		break;
	case ZASSERT_ON:
		zassert_true(harness_fired(), "ON did not fire");
		zassert_true(harness_contains("ASSERTION FAIL"), "location not printed");
		zassert_false(harness_contains("1 == 2"), "condition printed at ON");
		zassert_false(harness_contains(needle), "message printed below VERBOSE");
		break;
	default:
		zassert_true(harness_fired(), "VERBOSE did not fire");
		zassert_true(harness_contains("1 == 2"), "condition not printed at VERBOSE");
		zassert_true(harness_contains(needle), "message not printed at VERBOSE");
		break;
	}
}

ZTEST(zassert_inline, test_inline_block_module_follows_kconfig)
{
	expect_mymodule_level(inline_block_module_fn, "block module");
}

ZTEST(zassert_inline, test_inline_stateless_module_follows_kconfig)
{
	expect_mymodule_level(inline_stateless_module_fn, "stateless module");
}

ZTEST(zassert_inline, test_inline_forced_off_is_noop)
{
	HARNESS_EXPECT(inline_forced_off_fn());
	zassert_false(harness_fired(), "forced OFF fired");
	zassert_false(harness_printed(), "forced OFF printed");
}

ZTEST(zassert_inline, test_inline_forced_verbose)
{
	HARNESS_EXPECT(inline_forced_verbose_fn());

	if (IS_ENABLED(CONFIG_ASSERT)) {
		zassert_true(harness_fired(), "forced VERBOSE did not fire");
		zassert_true(harness_contains("forced verbose"), "forced VERBOSE message missing");
	} else {
		zassert_false(harness_fired(), "master switch off must silence ZASSERT_L");
		zassert_false(harness_printed(), "master switch off must not print");
	}
}

ZTEST_SUITE(zassert_inline, NULL, NULL, harness_reset, harness_reset, NULL);
