/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

BUILD_ASSERT(__cplusplus == 201703);

extern void test_array(void);
extern void test_make_unique(void);
extern void test_vector(void);

void test_main(void)
{
	TC_PRINT("version %u\n", (uint32_t)__cplusplus);
	ztest_test_suite(libcxx_tests,
			 ztest_unit_test(test_array),
			 ztest_unit_test(test_vector),
			 ztest_unit_test(test_make_unique)
		);

	ztest_run_test_suite(libcxx_tests);
}
