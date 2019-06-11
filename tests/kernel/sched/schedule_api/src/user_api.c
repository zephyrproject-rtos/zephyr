/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_sched.h"

struct k_thread user_thread;
K_SEM_DEFINE(user_sem, 0, 1);

ZTEST_BMEM volatile bool thread_was_preempt;

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
			K_USER | K_INHERIT_PERMS, 0);

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
	k_thread_create(&user_thread, ustack, STACK_SIZE, preempt_test_thread,
			NULL, NULL, NULL,
			k_thread_priority_get(k_current_get()),
			K_USER | K_INHERIT_PERMS, 0);

	k_sem_take(&user_sem, K_FOREVER);
	zassert_false(thread_was_preempt, "unexpected return value");

	k_thread_create(&user_thread, ustack, STACK_SIZE, preempt_test_thread,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(1),
			K_USER | K_INHERIT_PERMS, 0);

	k_sem_take(&user_sem, K_FOREVER);
	zassert_true(thread_was_preempt, "unexpected return value");

}

