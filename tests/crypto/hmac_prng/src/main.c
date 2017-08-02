/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_hmac_prng(void);

/**test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_hmac_prng_fn,
		ztest_unit_test(test_hmac_prng));
	ztest_run_test_suite(test_hmac_prng_fn);
}
