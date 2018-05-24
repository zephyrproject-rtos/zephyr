/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <kernel.h>
#include <kernel_structs.h>

__kernel struct k_thread kthread_thread;

#define STACKSIZE 1024
K_THREAD_STACK_DEFINE(kthread_stack, STACKSIZE);
K_SEM_DEFINE(sync_sem, 0, 1);

static void thread_entry(void *p1, void *p2, void *p3)
{
	_thread_essential_set();

	if (_is_thread_essential()) {
		k_busy_wait(100);
	} else {
		zassert_unreachable("The thread is not set as essential");
	}

	_thread_essential_clear();
	zassert_false(_is_thread_essential(),
		      "Essential flag of the thread is not cleared");

	k_sem_give(&sync_sem);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test to validate essential flag set/clear
 *
 * @see #K_ESSENTIAL(x)
 */
void test_essential_thread_operation(void)
{
	k_tid_t tid = k_thread_create(&kthread_thread, kthread_stack,
				      STACKSIZE, (k_thread_entry_t)thread_entry, NULL,
				      NULL, NULL, K_PRIO_PREEMPT(0), 0, 0);

	k_sem_take(&sync_sem, K_FOREVER);
	k_thread_abort(tid);
}
