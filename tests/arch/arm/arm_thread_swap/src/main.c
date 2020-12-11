/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_arm_thread_swap(void);
extern void test_arm_syscalls(void);
extern void test_syscall_cpu_scrubs_regs(void);

void test_main(void)
{
	ztest_test_suite(arm_thread_swap,
		ztest_unit_test(test_arm_thread_swap));
	ztest_run_test_suite(arm_thread_swap);
#if defined(CONFIG_USERSPACE)
	ztest_test_suite(arm_syscalls,
		ztest_unit_test(test_arm_syscalls),
		ztest_user_unit_test(test_syscall_cpu_scrubs_regs));
	ztest_run_test_suite(arm_syscalls);
#endif
}
