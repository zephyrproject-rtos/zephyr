/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_call_stacks_analyze_main(void);
extern void test_call_stacks_analyze_idle(void);
extern void test_call_stacks_analyze_workq(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_profiling_api,
			 ztest_unit_test(test_call_stacks_analyze_main),
			 ztest_unit_test(test_call_stacks_analyze_idle),
			 ztest_unit_test(test_call_stacks_analyze_workq));
	ztest_run_test_suite(test_profiling_api);
}
