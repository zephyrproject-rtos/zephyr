/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

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
	/*unreachable*/
	execute_flag = 2;
	zassert_true(1 == 0, NULL);
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
void test_threads_abort_self(void)
{
	execute_flag = 0;
	k_thread_create(&tdata, tstack, STACK_SIZE, thread_entry_abort,
			NULL, NULL, NULL, 0, K_USER, K_NO_WAIT);
	k_msleep(100);
	/**TESTPOINT: spawned thread executed but abort itself*/
	zassert_true(execute_flag == 1, NULL);
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
void test_threads_abort_others(void)
{
	execute_flag = 0;
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      0, K_USER, K_NO_WAIT);

	k_thread_abort(tid);
	k_msleep(100);
	/**TESTPOINT: check not-started thread is aborted*/
	zassert_true(execute_flag == 0, NULL);

	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      thread_entry, NULL, NULL, NULL,
			      0, K_USER, K_NO_WAIT);
	k_msleep(50);
	k_thread_abort(tid);
	/**TESTPOINT: check running thread is aborted*/
	zassert_true(execute_flag == 1, NULL);
	k_msleep(1000);
	zassert_true(execute_flag == 1, NULL);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test abort on a terminated thread
 *
 * @see k_thread_abort()
 */
void test_threads_abort_repeat(void)
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

static void abort_function(void)
{
	printk("Child thread's abort handler called\n");
	abort_called = true;
	k_free(block);
}

static void uthread_entry(void)
{
	block = k_malloc(BLOCK_SIZE);
	zassert_true(block != NULL, NULL);
	printk("Child thread is running\n");
	k_msleep(2);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test to validate the call of abort handler
 * specified by thread when it is aborted
 *
 * @see k_thread_abort(), #k_thread.fn_abort
 */
void test_abort_handler(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      (k_thread_entry_t)uthread_entry, NULL, NULL, NULL,
				      0, 0, K_NO_WAIT);

	tdata.fn_abort = &abort_function;

	k_msleep(1);

	abort_called = false;

	printk("Calling abort of child from parent\n");
	k_thread_abort(tid);

	zassert_true(abort_called == true, "Abort handler"
		     " is not called");
}

static void delayed_thread_entry(void *p1, void *p2, void *p3)
{
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
void test_delayed_thread_abort(void)
{
	int current_prio = k_thread_priority_get(k_current_get());

	/* Make current thread preemptive */
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(2));

	/* Create a preemptive thread of higher priority than
	 * current thread
	 */
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      (k_thread_entry_t)delayed_thread_entry, NULL, NULL, NULL,
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

static void offload_func(void *param)
{
	struct k_thread *t = param;

	k_thread_abort(t);

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

/**
 * @ingroup kernel_thread_tests
 * @brief Show that threads can be aborted from interrupt context
 *
 * @see k_thread_abort()
 */
void test_abort_from_isr(void)
{
	isr_finished = false;
	k_thread_create(&tdata, tstack, STACK_SIZE, entry_abort_isr,
			NULL, NULL, NULL, 0, 0, K_NO_WAIT);


	k_thread_join(&tdata, K_FOREVER);
	zassert_true(isr_finished, "ISR did not complete");
}
