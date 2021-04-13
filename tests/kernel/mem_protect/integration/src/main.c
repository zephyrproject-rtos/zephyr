/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>


extern void test_prod_consumer(void);


void test_main(void)
{
	ztest_test_suite(mem_protect_integration,
			 ztest_unit_test(test_prod_consumer));
	ztest_run_test_suite(mem_protect_integration);
}
