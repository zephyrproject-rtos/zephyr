/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_arm_runtime_nmi(void);

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(arm_runtime_nmi_fn,
		ztest_unit_test(test_arm_runtime_nmi));
	ztest_run_test_suite(arm_runtime_nmi_fn);
}
