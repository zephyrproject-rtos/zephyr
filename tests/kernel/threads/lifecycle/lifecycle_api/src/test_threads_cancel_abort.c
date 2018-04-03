/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_threads_lifecycle
 * @{
 * @defgroup t_threads_cancel_abort test_threads_cancel_abort
 * @}
 */
#include <ztest.h>

#define STACK_SIZE (256 + CONFIG_TEST_EXTRA_STACKSIZE)
K_THREAD_STACK_EXTERN(tstack);
extern struct k_thread tdata;
static int execute_flag;

K_SEM_DEFINE(sync_sema, 0, 1);
#define BLOCK_SIZE 64

static void thread_entry(void *p1, void *p2, void *p3)
{
	execute_flag = 1;
	k_sleep(100);
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

void test_threads_abort_self(void)
{
	execute_flag = 0;
	k_thread_create(&tdata, tstack, STACK_SIZE, thread_entry_abort,
			NULL, NULL, NULL, 0, K_USER, 0);
	k_sleep(100);
	/**TESTPOINT: spawned thread executed but abort itself*/
	zassert_true(execute_flag == 1, NULL);
}

void test_threads_abort_others(void)
{
	execute_flag = 0;
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      0, K_USER, 0);

	k_thread_abort(tid);
	k_sleep(100);
	/**TESTPOINT: check not-started thread is aborted*/
	zassert_true(execute_flag == 0, NULL);

	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      thread_entry, NULL, NULL, NULL,
			      0, K_USER, 0);
	k_sleep(50);
	k_thread_abort(tid);
	/**TESTPOINT: check running thread is aborted*/
	zassert_true(execute_flag == 1, NULL);
	k_sleep(1000);
	zassert_true(execute_flag == 1, NULL);
}

/* Test should not crash if repeated aborts are called on a dead thread. */
void test_threads_abort_repeat(void)
{
	execute_flag = 0;
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      0, K_USER, 0);

	k_thread_abort(tid);
	k_sleep(100);
	k_thread_abort(tid);
	k_sleep(100);
	k_thread_abort(tid);
	/* If no fault occured till now. The test case passed. */
	ztest_test_pass();
}

/* Test to validate the call of abort handler specified by thread
 * when it is aborted
 */
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
	k_sleep(K_MSEC(2));
}

void test_abort_handler(void)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)uthread_entry, NULL, NULL, NULL,
			0, 0, 0);

	tdata.fn_abort = &abort_function;

	k_sleep(K_MSEC(1));

	abort_called = false;

	printk("Calling abort of child from parent\n");
	k_thread_abort(tid);

	zassert_true(abort_called == true, "Abort handler"
			" is not called");
}
