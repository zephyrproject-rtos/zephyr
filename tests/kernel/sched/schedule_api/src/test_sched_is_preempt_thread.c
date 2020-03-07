/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>
#include "test_sched.h"

/*local variables*/
static struct k_thread tdata;
static struct k_sem end_sema;

static void tIsr(void *data)
{
	/** TESTPOINT: The code is running at ISR.*/
	zassert_false(k_is_preempt_thread(), NULL);
}

static void tpreempt_ctx(void *p1, void *p2, void *p3)
{
	/** TESTPOINT: The thread's priority is in the preemptible range.*/
	zassert_true(k_is_preempt_thread(), NULL);
	k_sched_lock();
	/** TESTPOINT: The thread has locked the scheduler.*/
	zassert_false(k_is_preempt_thread(), NULL);
	k_sched_unlock();
	/** TESTPOINT: The thread has not locked the scheduler.*/
	zassert_true(k_is_preempt_thread(), NULL);
	k_thread_priority_set(k_current_get(), K_PRIO_COOP(1));
	/** TESTPOINT: The thread's priority is in the cooperative range.*/
	zassert_false(k_is_preempt_thread(), NULL);
	k_sem_give(&end_sema);
}

static void tcoop_ctx(void *p1, void *p2, void *p3)
{
	/** TESTPOINT: The thread's priority is in the cooperative range.*/
	zassert_false(k_is_preempt_thread(), NULL);
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(1));
	/** TESTPOINT: The thread's priority is in the preemptible range.*/
	zassert_true(k_is_preempt_thread(), NULL);
	k_sched_lock();
	/** TESTPOINT: The thread has locked the scheduler.*/
	zassert_false(k_is_preempt_thread(), NULL);
	k_sched_unlock();
	/** TESTPOINT: The thread has not locked the scheduler.*/
	zassert_true(k_is_preempt_thread(), NULL);
	k_sem_give(&end_sema);
}

/*test cases*/

/**
 * @brief Validate the correctness of k_is_preempt_thread()
 *
 * @details Create a preemptive thread, lock the scheduler
 * and call k_is_preempt_thread(). Unlock the scheduler and
 * call k_is_preempt_thread() again. Create a cooperative
 * thread and lock the scheduler k_is_preempt_thread() and
 * unlock the scheduler and call k_is_preempt_thread().
 *
 * @see k_is_preempt_thread()
 *
 * @ingroup kernel_sched_tests
 */
void test_sched_is_preempt_thread(void)
{
	k_sem_init(&end_sema, 0, 1);

	/*create preempt thread*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tpreempt_ctx, NULL, NULL, NULL,
				      K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);

	/*create coop thread*/
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      tcoop_ctx, NULL, NULL, NULL,
			      K_PRIO_COOP(1), 0, K_NO_WAIT);
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);

	/*invoke isr*/
	irq_offload(tIsr, NULL);
}
