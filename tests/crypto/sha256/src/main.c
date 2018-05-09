/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_sha256_1(void);
extern void test_sha256_2(void);
extern void test_sha256_3(void);
extern void test_sha256_4(void);
extern void test_sha256_5(void);
extern void test_sha256_6(void);
extern void test_sha256_7(void);
extern void test_sha256_8(void);
extern void test_sha256_9(void);
extern void test_sha256_10(void);
extern void test_sha256_11(void);
extern void test_sha256_12(void);
extern void test_sha256_13_and_14(void);

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_sha256_fn,
		ztest_unit_test(test_sha256_1),
		ztest_unit_test(test_sha256_2),
		ztest_unit_test(test_sha256_3),
		ztest_unit_test(test_sha256_4),
		ztest_unit_test(test_sha256_5),
		ztest_unit_test(test_sha256_6),
		ztest_unit_test(test_sha256_7),
		ztest_unit_test(test_sha256_9),
		ztest_unit_test(test_sha256_10),
		ztest_unit_test(test_sha256_11),
		ztest_unit_test(test_sha256_12),
		ztest_unit_test(test_sha256_13_and_14));
	ztest_run_test_suite(test_sha256_fn);
}
