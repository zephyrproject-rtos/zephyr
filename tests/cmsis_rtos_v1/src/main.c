/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_os.h>

extern void test_thread_prio(void);
extern void test_thread_apis(void);
extern void test_thread_instances(void);

void test_main(void)
{
	ztest_test_suite(test_cmsis_apis,
			ztest_unit_test(test_thread_apis),
			ztest_unit_test(test_thread_prio),
			ztest_unit_test(test_thread_instances));
	ztest_run_test_suite(test_cmsis_apis);
}
