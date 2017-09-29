/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_vector_1(void);
extern void test_vector_2(void);
extern void test_vector_3(void);
extern void test_vector_4(void);
extern void test_vector_5(void);
extern void test_vector_6(void);
extern void test_vector_7(void);
extern void test_vector_8(void);

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_ccm_fn,
		ztest_unit_test(test_vector_1),
		ztest_unit_test(test_vector_2),
		ztest_unit_test(test_vector_3),
		ztest_unit_test(test_vector_4),
		ztest_unit_test(test_vector_5),
		ztest_unit_test(test_vector_6),
		ztest_unit_test(test_vector_7),
		ztest_unit_test(test_vector_8));
	ztest_run_test_suite(test_ccm_fn);
}
