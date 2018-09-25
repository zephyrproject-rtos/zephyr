/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

/*macros*/

/*external variables*/
extern void test_lowpower(void);
extern void (*hook_dev_state)(int);
extern void (*hook_dev_func)(void);

/*test cases*/
void test_suspendresume_sysdevices(void)
{
	/*system devices state set in "test_lowpower()"*/
	test_lowpower();
}

extern void test_sysclock_state(int);
extern void test_sysclock_func(void);
void test_suspendresume_sysclock(void)
{
	hook_dev_state = test_sysclock_state;
	hook_dev_func = test_sysclock_func;
	test_lowpower();
}

extern void test_rtc_state(int);
extern void test_rtc_func(void);
void test_suspendresume_rtc(void)
{
	hook_dev_state = test_rtc_state;
	hook_dev_func = test_rtc_func;
	test_lowpower();
}

extern void test_uart_state(int);
extern void test_uart_func(void);
void test_suspendresume_uart(void)
{
	hook_dev_state = test_uart_state;
	hook_dev_func = test_uart_func;
	test_lowpower();
}

extern void test_aonpt_state(int);
extern void test_aonpt_func(void);
void test_suspendresume_aonpt(void)
{
	hook_dev_state = test_aonpt_state;
	hook_dev_func = test_aonpt_func;
	test_lowpower();
}
