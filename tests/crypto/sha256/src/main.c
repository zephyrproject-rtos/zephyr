/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_1(void);
extern void test_2(void);
extern void test_3(void);
extern void test_4(void);
extern void test_5(void);
extern void test_6(void);
extern void test_7(void);
extern void test_8(void);
extern void test_9(void);
extern void test_10(void);
extern void test_11(void);
extern void test_12(void);
extern void test_13_and_14(void);

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_sha256_fn,
		ztest_unit_test(test_1),
		ztest_unit_test(test_2),
		ztest_unit_test(test_3),
		ztest_unit_test(test_4),
		ztest_unit_test(test_5),
		ztest_unit_test(test_6),
		ztest_unit_test(test_7),
		ztest_unit_test(test_7),
		ztest_unit_test(test_9),
		ztest_unit_test(test_10),
		ztest_unit_test(test_11),
		ztest_unit_test(test_12),
		ztest_unit_test(test_13_and_14));
	ztest_run_test_suite(test_sha256_fn);
}
