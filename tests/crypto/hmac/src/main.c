/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_hmac_1(void);
extern void test_hmac_2(void);
extern void test_hmac_3(void);
extern void test_hmac_4(void);
extern void test_hmac_5(void);
extern void test_hmac_6(void);
extern void test_hmac_7(void);

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_hmac_fn,
		ztest_unit_test(test_hmac_1),
		ztest_unit_test(test_hmac_2),
		ztest_unit_test(test_hmac_3),
		ztest_unit_test(test_hmac_4),
		ztest_unit_test(test_hmac_5),
		ztest_unit_test(test_hmac_6),
		ztest_unit_test(test_hmac_7));
	ztest_run_test_suite(test_hmac_fn);
}
