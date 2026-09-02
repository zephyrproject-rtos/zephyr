/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/irq_offload.h>

#include "tests_thread_apis.h"

static int thread2_data;

K_SEM_DEFINE(sem_thread2, 0, 1);
K_SEM_DEFINE(sem_thread1, 0, 1);

/** Arguments passed to the ISR that changes a thread's priority. */
struct isr_arg {
	k_tid_t thread;  /**< Thread whose priority is changed. */
	int     prio;    /**< Priority to set from the ISR. */
};

static struct isr_arg prio_args;

/**
 * @brief Test changing a thread's priority from an ISR
 */
static void test_isr(const void *arg)
{
	const struct isr_arg *data = arg;

	k_thread_priority_set(data->thread, data->prio);
}

/**
 * @brief Test thread behavior when its priority is changed
 */
void thread2_set_prio_test(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* lower thread2 priority by 5 */
	k_sem_take(&sem_thread2, K_FOREVER);
	thread2_data = k_thread_priority_get(k_current_get());
	k_sem_give(&sem_thread1);

	/* raise thread2 priority by 10 */
	k_sem_take(&sem_thread2, K_FOREVER);
	thread2_data = k_thread_priority_get(k_current_get());
	k_sem_give(&sem_thread1);

	/* restore thread2 priority */
	k_sem_take(&sem_thread2, K_FOREVER);
	thread2_data = k_thread_priority_get(k_current_get());
	k_sem_give(&sem_thread1);
}

/**
 * @brief Verify that a thread's priority can be changed and read back.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * k_thread_priority_set() must take effect on both the calling thread and
 * another thread, and the new value has to be what k_thread_priority_get()
 * subsequently reports. The second thread reads its own priority back and
 * hands the result to the test through a semaphore, so what is checked is the
 * priority the scheduler actually recorded rather than the value that was
 * requested.
 *
 * Test steps:
 * - Lower the current thread's own priority and read it back.
 * - Create a second thread and change its priority from the test thread.
 * - Have the second thread read its own priority and report it.
 * - Compare the reported priority against the requested one and join.
 *
 * Expected result:
 * - Both threads report exactly the priorities that were set for them.
 *
 * @see k_thread_priority_set()
 * @see k_thread_priority_get()
 */
ZTEST(threads_lifecycle, test_thread_priority_set)
{
	int rv;
	int prio = k_thread_priority_get(k_current_get());

	/* Lower the priority of the current thread (thread1) */
	k_thread_priority_set(k_current_get(), prio + 2);
	rv = k_thread_priority_get(k_current_get());
	zassert_equal(rv, prio + 2,
			"Expected priority to be changed to %d, not %d\n",
			prio + 2, rv);

	/* Raise the priority of the current thread (thread1) */
	k_thread_priority_set(k_current_get(), prio - 2);
	rv = k_thread_priority_get(k_current_get());
	zassert_equal(rv, prio - 2,
			"Expected priority to be changed to %d, not %d\n",
			prio - 2, rv);

	/* Restore the priority of the current thread (thread1) */
	k_thread_priority_set(k_current_get(), prio);
	rv = k_thread_priority_get(k_current_get());
	zassert_equal(rv, prio,
			"Expected priority to be changed to %d, not %d\n",
			prio, rv);

	/* create thread with lower priority */
	int thread2_prio = prio + 1;

	k_tid_t thread2_id = k_thread_create(&tdata, tstack, STACK_SIZE,
			thread2_set_prio_test,
			NULL, NULL, NULL, thread2_prio, 0,
			K_NO_WAIT);

	/* Lower the priority of thread2 */
	k_thread_priority_set(thread2_id, thread2_prio + 2);
	k_sem_give(&sem_thread2);
	k_sem_take(&sem_thread1, K_FOREVER);
	zassert_equal(thread2_data, thread2_prio + 2,
			"Expected priority to be changed to %d, not %d\n",
			thread2_prio + 2, thread2_data);

	/* Raise the priority of thread2 */
	k_thread_priority_set(thread2_id, thread2_prio - 2);
	k_sem_give(&sem_thread2);
	k_sem_take(&sem_thread1, K_FOREVER);
	zassert_equal(thread2_data, thread2_prio - 2,
			"Expected priority to be changed to %d, not %d\n",
			thread2_prio - 2, thread2_data);

	/* Restore the priority of thread2 */
	k_thread_priority_set(thread2_id, thread2_prio);
	k_sem_give(&sem_thread2);
	k_sem_take(&sem_thread1, K_FOREVER);
	zassert_equal(thread2_data, thread2_prio,
			"Expected priority to be changed to %d, not %d\n",
			thread2_prio, thread2_data);

	k_thread_join(thread2_id, K_FOREVER);
}

/**
 * @brief Verify that a thread's priority can be changed from an ISR.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * k_thread_priority_set() is callable from interrupt context, where it must
 * apply the same change without needing to reschedule inside the ISR. The
 * same priority changes as the thread-context case are therefore driven
 * through an offloaded interrupt, and the results are read back afterwards
 * from thread context.
 *
 * Test steps:
 * - Lower the current thread's own priority from an ISR via irq_offload().
 * - Read the priority back and compare it with the requested value.
 * - Change a second thread's priority from an ISR the same way.
 * - Have the second thread report its own priority and compare.
 *
 * Expected result:
 * - Both priority changes made from interrupt context take effect and are
 *   reported back.
 *
 * @see k_thread_priority_set()
 * @see k_thread_priority_get()
 * @see irq_offload()
 */
ZTEST(threads_lifecycle, test_thread_priority_set_from_isr)
{
	int rv;
	int prio = k_thread_priority_get(k_current_get());

	/* Lower the priority of the current thread (thread1) */
	prio_args.thread = k_current_get();
	prio_args.prio = prio + 2;
	irq_offload(test_isr, &prio_args);
	rv = k_thread_priority_get(k_current_get());
	zassert_equal(rv, prio + 2,
			"Expected priority to be changed to %d, not %d\n",
			prio + 2, rv);

	/* Raise the priority of the current thread (thread1) */
	prio_args.prio = prio - 2;
	irq_offload(test_isr, &prio_args);
	rv = k_thread_priority_get(k_current_get());
	zassert_equal(rv, prio - 2,
			"Expected priority to be changed to %d, not %d\n",
			prio - 2, rv);

	/* Restore the priority of the current thread (thread1) */
	prio_args.prio = prio;
	irq_offload(test_isr, &prio_args);
	rv = k_thread_priority_get(k_current_get());
	zassert_equal(rv, prio,
			"Expected priority to be changed to %d, not %d\n",
			prio, rv);

	/* create thread with lower priority */
	int thread2_prio = prio + 1;

	k_tid_t thread2_id = k_thread_create(&tdata, tstack, STACK_SIZE,
			thread2_set_prio_test,
			NULL, NULL, NULL, thread2_prio, 0,
			K_NO_WAIT);

	/* Lower the priority of thread2 */
	prio_args.thread = thread2_id;
	prio_args.prio = thread2_prio + 2;
	irq_offload(test_isr, &prio_args);
	k_sem_give(&sem_thread2);
	k_sem_take(&sem_thread1, K_FOREVER);
	zassert_equal(thread2_data, thread2_prio + 2,
			"Expected priority to be changed to %d, not %d\n",
			thread2_prio + 2, thread2_data);

	/* Raise the priority of thread2 */
	prio_args.prio = thread2_prio - 2;
	irq_offload(test_isr, &prio_args);
	k_sem_give(&sem_thread2);
	k_sem_take(&sem_thread1, K_FOREVER);
	zassert_equal(thread2_data, thread2_prio - 2,
			"Expected priority to be changed to %d, not %d\n",
			thread2_prio - 2, thread2_data);

	/* Restore the priority of thread2 */
	prio_args.prio = thread2_prio;
	irq_offload(test_isr, &prio_args);
	k_sem_give(&sem_thread2);
	k_sem_take(&sem_thread1, K_FOREVER);
	zassert_equal(thread2_data, thread2_prio,
			"Expected priority to be changed to %d, not %d\n",
			thread2_prio, thread2_data);
	k_thread_join(thread2_id, K_FOREVER);
}
