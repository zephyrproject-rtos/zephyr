/*
 * Copyright (c) 2019 Facebook
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Test the portable version of the math_extras.h functions */
#define PORTABLE_MISC_MATH_EXTRAS 1
#define VNAME(N) test_portable_##N
#include "tests.inc"

void test_portable_math_extras(void)
{
	/* clang-format off */
	ztest_test_suite(test_portable_math_extras,
		ztest_unit_test(test_portable_u32_add),
		ztest_unit_test(test_portable_u32_mul),
		ztest_unit_test(test_portable_u64_add),
		ztest_unit_test(test_portable_u64_mul),
		ztest_unit_test(test_portable_size_add),
		ztest_unit_test(test_portable_size_mul),
		ztest_unit_test(test_portable_u32_clz),
		ztest_unit_test(test_portable_u64_clz),
		ztest_unit_test(test_portable_u32_ctz),
		ztest_unit_test(test_portable_u64_ctz));
	ztest_run_test_suite(test_portable_math_extras);
	/* clang-format on */
}
