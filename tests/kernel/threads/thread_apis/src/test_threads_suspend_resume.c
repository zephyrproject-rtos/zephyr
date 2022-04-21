/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#include "tests_thread_apis.h"

static ZTEST_BMEM int last_prio;

static void thread_entry(void *p1, void *p2, void *p3)
{
	last_prio = k_thread_priority_get(k_current_get());
}

static void threads_suspend_resume(int prio)
{
	/* set current thread */
	last_prio = prio;
	k_thread_priority_set(k_current_get(), last_prio);

	/* create thread with lower priority */
	int create_prio = last_prio + 1;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      create_prio, K_USER, K_NO_WAIT);
	/* checkpoint: suspend current thread */
	k_thread_suspend(tid);
	k_msleep(100);
	/* checkpoint: created thread shouldn't be executed after suspend */
	zassert_false(last_prio == create_prio, NULL);
	k_thread_resume(tid);
	k_msleep(100);
	/* checkpoint: created thread should be executed after resume */
	zassert_true(last_prio == create_prio, NULL);
}

/*test cases*/

/**
 * @ingroup kernel_thread_tests
 * @brief Check the suspend and resume functionality in
 * a cooperative thread
 *
 * @details Create a thread with the priority lower than the current
 * thread which is cooperative and suspend it, make sure it doesn't
 * gets scheduled, and resume and check if the entry function is executed.
 *
 * @see k_thread_suspend(), k_thread_resume()
 */
void test_threads_suspend_resume_cooperative(void)
{
	threads_suspend_resume(-2);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Check the suspend and resume functionality in
 * preemptive thread
 *
 * @details Create a thread with the priority lower than the current
 * thread which is preemptive and suspend it, make sure it doesn't gets
 * scheduled, and resume and check if the entry function is executed.
 *
 * @see k_thread_suspend(), k_thread_resume()
 */
void test_threads_suspend_resume_preemptible(void)
{
	threads_suspend_resume(1);
}

static bool after_suspend;

void suspend_myself(void *arg0, void *arg1, void *arg2)
{
	ARG_UNUSED(arg0);
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	k_thread_suspend(k_current_get());
	after_suspend = true;
}

/**
 * @ingroup kernel_thread_tests
 *
 * @brief Check that k_thread_suspend() is a schedule point when
 * called on the current thread.
 */
void test_threads_suspend(void)
{
	after_suspend = false;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      suspend_myself, NULL, NULL, NULL,
				      0, K_USER, K_NO_WAIT);

	/* Give the thread a chance to start and verify that it
	 * stopped executing after suspending itself.
	 */
	k_msleep(100);
	zassert_false(after_suspend, "thread woke up unexpectedly");

	k_thread_abort(tid);
}

void sleep_suspended(void *arg0, void *arg1, void *arg2)
{
	ARG_UNUSED(arg0);
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	/* Sleep a half second, then set the flag after we wake up.
	 * If we are suspended, the wakeup should not occur
	 */
	k_msleep(100);
	after_suspend = true;
}

/**
 * @ingroup kernel_thread_tests
 * @brief Check that k_thread_suspend() cancels a preexisting thread timeout
 *
 * @details Suspended threads should not wake up unexpectedly if they
 * happened to have been sleeping when suspended.
 */
void test_threads_suspend_timeout(void)
{
	after_suspend = false;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      sleep_suspended, NULL, NULL, NULL,
				      0, K_USER, K_NO_WAIT);

	k_msleep(50);
	k_thread_suspend(tid);

	/* Give the timer long enough to expire, and verify that it
	 * has not (i.e. that the thread didn't wake up, because it
	 * has been suspended)
	 */
	k_msleep(200);
	zassert_false(after_suspend, "thread woke up unexpectedly");

	k_thread_abort(tid);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Check resume an unsuspend thread
 *
 * @details Use k_thread_state_str() to get thread state.
 * Resume an unsuspend thread will not change the thread state.
 */
void test_resume_unsuspend_thread(void)
{
	char buffer[32];
	const char *str;
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      0, K_USER, K_NO_WAIT);


	/* Resume an unsuspend thread will not change the thread state. */
	str = k_thread_state_str(tid, buffer, sizeof(buffer));
	zassert_true(strcmp(str, "queued") == 0, NULL);
	k_thread_resume(tid);
	str = k_thread_state_str(tid, buffer, sizeof(buffer));
	zassert_true(strcmp(str, "queued") == 0, NULL);

	/* suspend created thread */
	k_thread_suspend(tid);
	str = k_thread_state_str(tid, buffer, sizeof(buffer));
	zassert_true(strcmp(str, "suspended") == 0, NULL);

	/* Resume an suspend thread will make it to be next eligible.*/
	k_thread_resume(tid);
	str = k_thread_state_str(tid, buffer, sizeof(buffer));
	zassert_true(strcmp(str, "queued") == 0, NULL);
	k_thread_abort(tid);
}
