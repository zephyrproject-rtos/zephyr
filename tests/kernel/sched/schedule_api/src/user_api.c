/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_sched.h"

struct k_thread user_thread;
K_SEM_DEFINE(user_sem, 0, 1);

ZTEST_BMEM volatile int thread_was_preempt;

K_THREAD_STACK_DEFINE(ustack, STACK_SIZE);

static void sleepy_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sleep(K_FOREVER);
	k_sem_give(&user_sem);
}

void test_user_k_wakeup(void)
{
	k_thread_create(&user_thread, ustack, STACK_SIZE, sleepy_thread,
			NULL, NULL, NULL,
			k_thread_priority_get(k_current_get()),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_yield(); /* Let thread run and start sleeping forever */
	k_wakeup(&user_thread);
	k_sem_take(&user_sem, K_FOREVER);
}

static void preempt_test_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	thread_was_preempt = k_is_preempt_thread();
	k_sem_give(&user_sem);
}

void test_user_k_is_preempt(void)
{
	/* thread_was_preempt is volatile, and static analysis doesn't
	 * like to see it being tested inside zassert_true, because
	 * the read is treated as a "side effect" of an assertion
	 * (e.g. a read is significant for things like volatile MMIO
	 * addresses, and assertions may or may not be compiled, even
	 * though here in a test they always will be and in any case
	 * the value is a static variable above marked volatile for
	 * threadsafety).  Read it into a local variable first to
	 * evade the warning.
	 */
	int twp;

	k_thread_create(&user_thread, ustack, STACK_SIZE, preempt_test_thread,
			NULL, NULL, NULL,
			k_thread_priority_get(k_current_get()),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_sem_take(&user_sem, K_FOREVER);

	twp = thread_was_preempt;
	zassert_false(twp, "unexpected return value");

	k_thread_create(&user_thread, ustack, STACK_SIZE, preempt_test_thread,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(1),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_sem_take(&user_sem, K_FOREVER);

	twp = thread_was_preempt;
	zassert_true(twp, "unexpected return value");
}

