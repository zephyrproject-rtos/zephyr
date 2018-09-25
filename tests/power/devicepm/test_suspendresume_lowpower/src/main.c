/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_suspendresume_sysdevices(void);
extern void test_suspendresume_sysclock(void);
extern void test_suspendresume_rtc(void);
extern void test_suspendresume_uart(void);
extern void test_suspendresume_aonpt(void);
extern int wait_for_test_start(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_suspendresume_lowpower,
		ztest_unit_test(test_suspendresume_sysdevices),
		ztest_unit_test(test_suspendresume_sysclock),
		ztest_unit_test(test_suspendresume_rtc),
		ztest_unit_test(test_suspendresume_uart),
		ztest_unit_test(test_suspendresume_aonpt));
	ztest_run_test_suite(test_suspendresume_lowpower);
}
