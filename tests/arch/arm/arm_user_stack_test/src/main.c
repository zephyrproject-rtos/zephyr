/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_FPU_SHARING
#include <math.h>
#endif
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/syscall_list.h>

struct k_thread th0, th1;
K_THREAD_STACK_DEFINE(stk0, 2048);
K_THREAD_STACK_DEFINE(stk1, 2048);

ZTEST_BMEM int attack_stack[128];
ZTEST_BMEM uint64_t sys_ret; /* 64 syscalls take result address in r0 */

volatile int kernel_secret;
volatile int *const attack_sp = &attack_stack[128];
const int sysno = K_SYSCALL_K_UPTIME_TICKS;
k_tid_t low_tid, hi_tid;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	ztest_test_pass();
	k_thread_abort(low_tid);

	/* This check is to handle a case where low prio thread has started and
	 * resulted in a fault while changing the sp but
	 * the high prio thread is not created yet
	 */
	if (hi_tid) {
		k_thread_abort(hi_tid);
	}
}

void attack_entry(void)
{
	printf("Call %s from %s\n", __func__, k_is_user_context() ? "user" : "kernel");
	/* kernel_secret can only be updated in privilege mode so updating it here should result in
	 * a fault. If it doesn't we fail the test.
	 */
	kernel_secret = 1;

	printf("Changed the kernel_secret so marking test as failed\n");
	ztest_test_fail();

	k_thread_abort(low_tid);
	k_thread_abort(hi_tid);
}

void low_fn(void *arg1, void *arg2, void *arg3)
{
#ifdef CONFIG_FPU_SHARING
	double x = 1.2345;
	double y = 6.789;

	/* some random fp stuff so that an extended stack frame is saved on svc */
	zassert_equal(x, 1.2345);
	zassert_equal(y, 6.789);
#endif
	printf("Call %s from %s\n", __func__, k_is_user_context() ? "user" : "kernel");
	attack_stack[0] = 1;
	__asm__ volatile("mov sp, %0;"
			 "1:;"
			 "ldr r0, =sys_ret;"
			 "ldr r6, =sysno;"
			 "ldr r6, [r6];"
			 "svc 3;"
			 "b 1b;" ::"r"(attack_sp));
}

void hi_fn(void *arg1, void *arg2, void *arg3)
{
	printf("Call %s from %s\n", __func__, k_is_user_context() ? "user" : "kernel");
	while (1) {
		attack_sp[-2] = (int)attack_entry;
		k_msleep(1);
	}
}

ZTEST(arm_user_stack_test, test_arm_user_stack_corruption)
{
	low_tid = k_thread_create(&th0, stk0, K_THREAD_STACK_SIZEOF(stk0), low_fn, NULL, NULL, NULL,
				  2,
#ifdef CONFIG_FPU_SHARING
				  K_INHERIT_PERMS | K_USER | K_FP_REGS,
#else
				  K_INHERIT_PERMS | K_USER,
#endif
				  K_NO_WAIT);

	k_msleep(6); /* let low_fn start looping */
	hi_tid = k_thread_create(&th1, stk1, K_THREAD_STACK_SIZEOF(stk1), hi_fn, NULL, NULL, NULL,
				 1, K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_thread_join(&th0, K_FOREVER);
	k_thread_join(&th1, K_FOREVER);
}

ZTEST_SUITE(arm_user_stack_test, NULL, NULL, NULL, NULL, NULL);
