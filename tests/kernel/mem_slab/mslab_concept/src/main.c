/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_mslab_alloc_wait_prio(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(mslab_concept,
			 ztest_unit_test(test_mslab_alloc_wait_prio));
	ztest_run_test_suite(mslab_concept);
}

