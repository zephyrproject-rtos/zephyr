/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "tests_thread_apis.h"

static ZTEST_BMEM int execute_flag;

K_SEM_DEFINE(sync_sema, 0, 1);
#define BLOCK_SIZE 64

static void thread_entry(void *p1, void *p2, void *p3)
{
	execute_flag = 1;
	k_msleep(100);
	execute_flag = 2;
}

static void thread_entry_abort(void *p1, void *p2, void *p3)
{
	/**TESTPOINT: abort current thread*/
	execute_flag = 1;
	k_thread_abort(k_current_get());
	CODE_UNREACHABLE;
	/*unreachable*/
	execute_flag = 2;
	zassert_true(1 == 0);
}
/**
 * @ingroup kernel_thread_tests
 * @brief Validate k_thread_abort() when called by current thread
 *
 * @details Create a user thread and let the thread execute.
 * Then call k_thread_abort() and check if the thread is terminated.
 * Here the main thread is also a user thread.
 *
 * @see k_thread_abort()
 */
ZTEST_USER(threads_lifecycle, test_threads_abort_self)
{
	execute_flag = 0;
	k_thread_create(&tdata, tstack, STACK_SIZE, thread_entry_abort,
			NULL, NULL, NULL, 0, K_USER, K_NO_WAIT);
	k_msleep(100);
	/**TESTPOINT: spawned thread executed but abort itself*/
	zassert_true(execute_flag == 1);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Validate k_thread_abort() when called by other thread
 *
 * @details Create a user thread and abort the thread before its
 * execution. Create a another user thread and abort the thread
 * after it has started.
 *
 * @see k_thread_abort()
 */
ZTEST_USER(threads_lifecycle, test_threads_abort_others)
{
	execute_flag = 0;
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      0, K_USER, K_NO_WAIT);

	k_thread_abort(tid);
	k_msleep(100);
	/**TESTPOINT: check not-started thread is aborted*/
	zassert_true(execute_flag == 0);

	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      thread_entry, NULL, NULL, NULL,
			      0, K_USER, K_NO_WAIT);
	k_msleep(50);
	k_thread_abort(tid);
	/**TESTPOINT: check running thread is aborted*/
	zassert_true(execute_flag == 1);
	k_msleep(1000);
	zassert_true(execute_flag == 1);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test abort on a terminated thread
 *
 * @see k_thread_abort()
 */
ZTEST(threads_lifecycle_1cpu, test_threads_abort_repeat)
{
	execute_flag = 0;
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      0, K_USER, K_NO_WAIT);

	k_thread_abort(tid);
	k_msleep(100);
	k_thread_abort(tid);
	k_msleep(100);
	k_thread_abort(tid);
	/* If no fault occurred till now. The test case passed. */
	ztest_test_pass();
}

bool abort_called;
void *block;

static void delayed_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	execute_flag = 1;

	zassert_unreachable("Delayed thread shouldn't be executed");
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test abort on delayed thread before it has started
 * execution
 *
 * @see k_thread_abort()
 */
ZTEST(threads_lifecycle_1cpu, test_delayed_thread_abort)
{
	int current_prio = k_thread_priority_get(k_current_get());

	execute_flag = 0;
	/* Make current thread preemptive */
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(2));

	/* Create a preemptive thread of higher priority than
	 * current thread
	 */
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      delayed_thread_entry, NULL, NULL, NULL,
				      K_PRIO_PREEMPT(1), 0, K_MSEC(100));

	/* Give up CPU */
	k_msleep(50);

	/* Test point: check if thread delayed for 100ms has not started*/
	zassert_true(execute_flag == 0, "Delayed thread created is not"
		     " put to wait queue");

	k_thread_abort(tid);

	/* Test point: Test abort of thread before its execution*/
	zassert_false(execute_flag == 1, "Delayed thread is has executed"
		      " before cancellation");

	/* Restore the priority */
	k_thread_priority_set(k_current_get(), current_prio);
}

static volatile bool isr_finished;

static void offload_func(const void *param)
{
	struct k_thread *t = (struct k_thread *)param;

	k_thread_abort(t);

	/* Thread memory is unused now, validate that we can clobber it. */
	if (!IS_ENABLED(CONFIG_ARCH_POSIX)) {
		memset(t, 0, sizeof(*t));
	}

	/* k_thread_abort() in an isr shouldn't affect the ISR's execution */
	isr_finished = true;
}

static void entry_abort_isr(void *p1, void *p2, void *p3)
{
	/* Simulate taking an interrupt which kills this thread */
	irq_offload(offload_func, k_current_get());

	printk("shouldn't see this, thread should have been killed");
	ztest_test_fail();
}

extern struct k_sem offload_sem;

/**
 * @ingroup kernel_thread_tests
 *
 * @brief Show that threads can be aborted from interrupt context by itself
 *
 * @details Spwan a thread, then enter ISR context in child thread and abort
 * the child thread. Check if ISR completed and target thread was aborted.
 *
 * @see k_thread_abort()
 */
ZTEST(threads_lifecycle, test_abort_from_isr)
{
	isr_finished = false;
	k_thread_create(&tdata, tstack, STACK_SIZE, entry_abort_isr,
			NULL, NULL, NULL, 0, 0, K_NO_WAIT);


	k_thread_join(&tdata, K_FOREVER);
	zassert_true(isr_finished, "ISR did not complete");

	/* Thread struct was cleared after the abort, make sure it is
	 * still clear (i.e. that the arch layer didn't write to it
	 * during interrupt exit).  Doesn't work on posix, which needs
	 * the thread struct for its swap code.
	 */
	uint8_t *p = (uint8_t *)&tdata;

	if (!IS_ENABLED(CONFIG_ARCH_POSIX)) {
		for (int i = 0; i < sizeof(tdata); i++) {
			zassert_true(p[i] == 0, "Free memory write to aborted thread");
		}
	}

	/* Notice: Recover back the offload_sem: This is use for releasing
	 * offload_sem which might be held when thread aborts itself in ISR
	 * context, it will cause irq_offload cannot be used again.
	 */
	k_sem_give(&offload_sem);
}

/* use for sync thread start */
static struct k_sem sem_abort;

static void entry_aborted_thread(void *p1, void *p2, void *p3)
{
	k_sem_give(&sem_abort);

	/* wait for being aborted */
	while (1) {
		k_sleep(K_MSEC(1));
	}
	zassert_unreachable("should not reach here");
}

/**
 * @ingroup kernel_thread_tests
 *
 * @brief Show that threads can be aborted from interrupt context
 *
 * @details Spwan a thread, then enter ISR context in main thread and abort
 * the child thread. Check if ISR completed and target thread was aborted.
 *
 * @see k_thread_abort()
 */
ZTEST(threads_lifecycle, test_abort_from_isr_not_self)
{
	k_tid_t tid;

	isr_finished = false;
	k_sem_init(&sem_abort, 0, 1);

	tid = k_thread_create(&tdata, tstack, STACK_SIZE, entry_aborted_thread,
			NULL, NULL, NULL, 0, 0, K_NO_WAIT);

	/* wait for thread started */
	k_sem_take(&sem_abort, K_FOREVER);

	/* Simulate taking an interrupt which kills spwan thread */
	irq_offload(offload_func, (void *)tid);

	zassert_true(isr_finished, "ISR did not complete");
}
