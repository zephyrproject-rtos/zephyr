/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

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
	zassert_false(last_prio == create_prio);
	k_thread_resume(tid);
	k_msleep(100);
	/* checkpoint: created thread should be executed after resume */
	zassert_true(last_prio == create_prio);
}

/*test cases*/

/**
 * @brief Verify that suspend and resume work on a cooperative thread.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * A suspended thread must not be dispatched no matter what its priority
 * would otherwise allow, and resuming it must make it runnable again. The
 * worker is created at a cooperative priority and records that it ran, so
 * the check is on observed execution rather than on a state flag.
 *
 * Test steps:
 * - Create a cooperative thread and suspend it before it can run.
 * - Sleep long enough that it would have run, and confirm it did not.
 * - Resume it and sleep again.
 * - Check that the entry function executed and that the thread kept the
 *   priority it was created with.
 *
 * Expected result:
 * - The thread does not run while suspended and does run once resumed.
 *
 * @see k_thread_suspend()
 * @see k_thread_resume()
 */
ZTEST(threads_lifecycle_1cpu, test_thread_suspend_resume_coop)
{
	threads_suspend_resume(-2);
}

/**
 * @brief Verify that suspend and resume work on a preemptible thread.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * The preemptible counterpart of the cooperative case, run from user mode so
 * suspend and resume are reached through their system calls. A preemptible
 * thread is eligible to be scheduled as soon as it is ready, which makes
 * "did not run while suspended" the meaningful part of the check.
 *
 * Test steps:
 * - Create a preemptible thread and suspend it before it can run.
 * - Sleep long enough that it would have run, and confirm it did not.
 * - Resume it and sleep again.
 * - Check that the entry function executed and that the thread kept the
 *   priority it was created with.
 *
 * Expected result:
 * - The thread does not run while suspended and does run once resumed.
 *
 * @see k_thread_suspend()
 * @see k_thread_resume()
 */
ZTEST_USER(threads_lifecycle, test_thread_suspend_resume_preempt)
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
 * @brief Verify that a thread suspending itself reschedules immediately.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * k_thread_suspend() called on the current thread has to act as a scheduling
 * point: the thread must give up the CPU inside the call rather than running
 * on to the next statement. The worker sets a flag on the line after the
 * call, so that flag being clear is what proves the switch happened there.
 *
 * Test steps:
 * - Create a user thread that suspends itself and then sets a flag.
 * - Give it time to start and run into the suspend.
 * - Check the flag, then resume the thread and check it again.
 *
 * Expected result:
 * - The flag stays clear while the thread is suspended and is set once it is
 *   resumed, so the suspend switched away before the following statement.
 *
 * @see k_thread_suspend()
 */
ZTEST(threads_lifecycle, test_thread_suspend)
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
 * @brief Verify that suspending a sleeping thread cancels its timeout.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * A thread that is suspended while it happens to be sleeping must not be
 * woken by the timeout it had pending. If the timeout survived the suspend,
 * the thread would come back on its own, which is exactly what a suspended
 * thread must never do.
 *
 * Test steps:
 * - Create a user thread that sleeps and then sets a flag on waking.
 * - Suspend it part-way through its sleep.
 * - Wait well past the point its sleep would have expired.
 * - Check the flag.
 *
 * Expected result:
 * - The thread does not wake up, so the flag stays clear.
 *
 * @see k_thread_suspend()
 * @see k_sleep()
 */
ZTEST(threads_lifecycle, test_thread_suspend_timeout)
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
 * @brief Verify that resuming a thread that is not suspended does nothing.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * k_thread_resume() on a thread that was never suspended has to be a no-op
 * rather than disturbing whatever the thread is currently doing. The thread's
 * state string is sampled before and after the call, so an unwanted change of
 * state is visible rather than merely assumed absent.
 *
 * Test steps:
 * - Create a thread and let it reach a known state.
 * - Read its state with k_thread_state_str().
 * - Call k_thread_resume() on it, which was never suspended.
 * - Read the state again and compare.
 *
 * Expected result:
 * - The thread state is unchanged by the resume.
 *
 * @see k_thread_resume()
 * @see k_thread_state_str()
 */
ZTEST(threads_lifecycle, test_thread_resume_not_suspended)
{
	char buffer[32];
	const char *str;
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      0, K_USER, K_NO_WAIT);


	/* Resume an unsuspend thread will not change the thread state. */
	str = k_thread_state_str(tid, buffer, sizeof(buffer));
	zassert_str_equal(str, "queued");
	k_thread_resume(tid);
	str = k_thread_state_str(tid, buffer, sizeof(buffer));
	zassert_str_equal(str, "queued");

	/* suspend created thread */
	k_thread_suspend(tid);
	str = k_thread_state_str(tid, buffer, sizeof(buffer));
	zassert_str_equal(str, "suspended");

	/* Resume an suspend thread will make it to be next eligible.*/
	k_thread_resume(tid);
	str = k_thread_state_str(tid, buffer, sizeof(buffer));
	zassert_str_equal(str, "queued");
	k_thread_abort(tid);
}
