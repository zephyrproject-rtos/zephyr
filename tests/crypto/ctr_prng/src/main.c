/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_ctr_prng_vector(void);
extern void test_reseed(void);
extern void test_uninstantiate(void);
extern void test_robustness(void);

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_ctr_prng_fn,
		ztest_unit_test(test_ctr_prng_vector),
		ztest_unit_test(test_reseed),
		ztest_unit_test(test_uninstantiate),
		ztest_unit_test(test_robustness));
	ztest_run_test_suite(test_ctr_prng_fn);
}
