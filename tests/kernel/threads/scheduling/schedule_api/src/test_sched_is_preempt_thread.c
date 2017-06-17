/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_sched_api
 * @{
 * @defgroup t_sched_is_preempt_thread test_sched_is_preempt_thread
 * @brief TestPurpose: verify context type is preempt thread
 * - API coverage
 *   -# k_is_preempt_thread
 * @}
 */

#include <ztest.h>
#include <irq_offload.h>

/*macro definition*/
#define STACK_SIZE 512

/*local variables*/
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
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
void test_sched_is_preempt_thread(void)
{
	k_sem_init(&end_sema, 0, 1);

	/*create preempt thread*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
		tpreempt_ctx, NULL, NULL, NULL,
		K_PRIO_PREEMPT(1), 0, 0);
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);

	/*create coop thread*/
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
		tcoop_ctx, NULL, NULL, NULL,
		K_PRIO_COOP(1), 0, 0);
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);

	/*invoke isr*/
	irq_offload(tIsr, NULL);
}
