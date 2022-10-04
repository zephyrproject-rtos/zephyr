/*
 * Copyright (c) 2019 Facebook
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Test the normal version of the math_extras.h functions */
#define VNAME(N) test_##N
#include "tests.inc"

extern void test_portable_math_extras(void);

void test_main(void)
{
	/* clang-format off */
	ztest_test_suite(test_math_extras,
		ztest_unit_test(test_u32_add),
		ztest_unit_test(test_u32_mul),
		ztest_unit_test(test_u64_add),
		ztest_unit_test(test_u64_mul),
		ztest_unit_test(test_size_add),
		ztest_unit_test(test_size_mul),
		ztest_unit_test(test_u32_clz),
		ztest_unit_test(test_u64_clz),
		ztest_unit_test(test_u32_ctz),
		ztest_unit_test(test_u64_ctz));
	ztest_run_test_suite(test_math_extras);
	/* clang-format on */

	test_portable_math_extras();
}
