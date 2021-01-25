/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ksched.h>
#include <spinlock.h>
#include <wait_q.h>
#include <ztest.h>

struct sched_pending {
	_wait_q_t waitq;
	struct k_spinlock lock;
} pending;

#define STACK_SIZE	(1024 + CONFIG_TEST_EXTRA_STACKSIZE)

K_THREAD_STACK_DEFINE(test_stack, STACK_SIZE);

static struct k_thread tdata;

static atomic_t num;
static k_tid_t tid;

static void thread_handler1(void *p1, void *p2, void *p3)
{
	z_waitq_init(&pending.waitq);
	k_spinlock_key_t key = k_spin_lock(&pending.lock);

	z_pend_thread(_current, &pending.waitq, K_FOREVER);
	z_reschedule(&pending.lock, key);
}

static void thread_handler2(void *p1, void *p2, void *p3)
{
	z_waitq_init(&pending.waitq);

	unsigned int key = irq_lock();

	z_pend_curr_irqlock(key, &pending.waitq, K_FOREVER);
}

static void thread_handler3(void *p1, void *p2, void *p3)
{
	atomic_inc(&num);
}

/**
 * @brief Test kernel api z_pend_thread()  z_unpend_thread()
 * z_unpend_all() z_pend_curr_irqlock()
 *
 * @details Part1: Creat a child thread and when it's running,
 * use z_pend_thread() api to set it to pending state.
 * Verfiy the child thread state in main thread, it should be in pending.
 * Use z_unpend_thread() api to unpend the child thread and verify in
 * main thread. It should be in unpending state.
 * Part2: Creat a child thread and make it in pending state.
 * Use z_unpend_all() to unpend it then verify the state.
 * Part3: Test z_pend_curr_irqlock() and verify it's pending state.
 *
 * @ingroup kernel_sched_tests
 *
 * @see z_pend_thread() z_unpend_thread() z_unpend_all()
 * z_pend_curr_irqlock()
 */
void test_kernel_api_pend_unpend(void)
{
	/* Part1: test kernel api z_pend_thread() z_unpend_thread() */
	tid = k_thread_create(&tdata, test_stack, STACK_SIZE,
					thread_handler1, NULL, NULL, NULL,
					K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	/* relinquish CPU for above thread to start */
	k_msleep(1);
	zassert_true(z_is_thread_pending(&tdata), NULL);
	z_unpend_thread(tid);
	z_ready_thread(tid);
	zassert_false(z_is_thread_pending(&tdata), NULL);
	k_thread_abort(tid);

	/* Part2: test kernel api z_pend_thread() z_unpend_all() */
	tid = k_thread_create(&tdata, test_stack, STACK_SIZE,
					thread_handler1, NULL, NULL, NULL,
					K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	k_msleep(1);
	zassert_true(z_is_thread_pending(&tdata), NULL);
	z_unpend_all(&pending.waitq);
	zassert_false(z_is_thread_pending(&tdata), NULL);
	k_thread_abort(tid);

	/* Part3: test kernel api z_pend_curr_irqlock() */
	tid = k_thread_create(&tdata, test_stack, STACK_SIZE,
				thread_handler2, NULL, NULL, NULL,
				K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	k_msleep(1);
	zassert_true(z_is_thread_pending(&tdata), NULL);
	k_thread_abort(tid);
}

/**
 * @brief Test kernel api z_move_thread_to_end_of_prio_q()
 * z_remove_thread_from_ready_q()
 *
 * @details Firstly, creat a child thread and use
 * z_move_thread_to_end_of_prio_q()  api to append it to prio_q,
 * then verify then thread's callback has been executed.
 * Secondly, call z_remove_thread_from_ready_q() to remove a thread
 * that has been add to prio_q. The callback should not be executed.
 *
 * @ingroup kernel_sched_tests
 *
 * @see z_move_thread_to_end_of_prio_q() z_remove_thread_from_ready_q()
 */
void test_kernel_api_append_remove_queue(void)
{
	tid = k_thread_create(&tdata, test_stack, STACK_SIZE,
					thread_handler3, NULL, NULL, NULL,
					K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	/* test child thread will be implemented when append into prio_q */
	zassert_true(num == 0, NULL);
	z_move_thread_to_end_of_prio_q(&tdata);
	k_msleep(1);
	zassert_true(num == 1, NULL);

	/* test the child thread is not implemented */
	z_move_thread_to_end_of_prio_q(&tdata);
	z_remove_thread_from_ready_q(&tdata);
	zassert_true(num == 1, NULL);

	k_thread_abort(tid);
}
