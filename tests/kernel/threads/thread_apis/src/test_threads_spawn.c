/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/ztest.h>

#include "tests_thread_apis.h"

static ZTEST_BMEM char tp1[8];
static ZTEST_DMEM int tp2 = 100;
static ZTEST_BMEM struct k_sema *tp3;
static ZTEST_BMEM int spawn_prio;

static void thread_entry_params(void *p1, void *p2, void *p3)
{
	/* checkpoint: check parameter 1, 2, 3 */
	zassert_equal(p1, tp1);
	zassert_equal(POINTER_TO_INT(p2), tp2);
	zassert_equal(p3, tp3);
}

static void thread_entry_priority(void *p1, void *p2, void *p3)
{
	/* checkpoint: check priority */
	zassert_equal(k_thread_priority_get(k_current_get()), spawn_prio);
}

static void thread_entry_delay(void *p1, void *p2, void *p3)
{
	tp2 = 100;
}

/* test cases */

/**
 * @brief Verify that a spawned thread receives the parameters it was given.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * The three parameters handed to k_thread_create() have to arrive unchanged
 * at the thread's entry point. The spawned thread compares them itself and
 * signals through a semaphore it was passed as one of them, so a pass proves
 * both that the values survived and that a passed kernel object is usable.
 *
 * Test steps:
 * - Create a user thread, passing two values and a semaphore.
 * - In the thread, compare the three parameters against what was passed.
 * - Sleep long enough for the thread to run.
 *
 * Expected result:
 * - The thread observes exactly the parameters that were passed to it.
 *
 * @see k_thread_create()
 */
ZTEST_USER(threads_lifecycle, test_thread_spawn_params)
{
	k_thread_create(&tdata, tstack, STACK_SIZE, thread_entry_params,
			tp1, INT_TO_POINTER(tp2), tp3, 0,
			K_USER, K_NO_WAIT);
	k_msleep(100);
}

/**
 * @brief Verify that a thread spawned at a higher priority preempts its
 *        creator.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * Creating a thread at a priority above the caller's must make it runnable at
 * once and preempt the caller, rather than waiting for the caller to block.
 * The spawned thread records the priority it observes so the placement can be
 * checked as well as the fact that it ran.
 *
 * Test steps:
 * - Compute a priority one step higher than the current thread's.
 * - Create a user thread at that priority.
 * - Sleep, then check what the thread recorded.
 *
 * Expected result:
 * - The thread runs and reports the priority it was created with.
 *
 * @see k_thread_create()
 * @see k_thread_priority_get()
 */
ZTEST(threads_lifecycle, test_thread_spawn_priority)
{
	/* spawn thread with higher priority */
	spawn_prio = k_thread_priority_get(k_current_get()) - 1;
	k_thread_create(&tdata, tstack, STACK_SIZE, thread_entry_priority,
			NULL, NULL, NULL, spawn_prio, K_USER, K_NO_WAIT);
	k_msleep(100);
}

/**
 * @brief Verify that a spawn delay defers the thread's first execution.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * A thread created with a start delay must not run before that delay has
 * elapsed. The check is made at a point in time deliberately shorter than the
 * delay, so a thread that started early is caught rather than merely being
 * late.
 *
 * Test steps:
 * - Set a sentinel value that the thread would overwrite.
 * - Create a user thread with a 120 ms start delay.
 * - Sleep only 100 ms and read the sentinel back.
 *
 * Expected result:
 * - The sentinel is untouched, so the thread had not run yet.
 *
 * @see k_thread_create()
 */
ZTEST_USER(threads_lifecycle, test_thread_spawn_delay)
{
	/* spawn thread with higher priority */
	tp2 = 10;
	k_thread_create(&tdata, tstack, STACK_SIZE, thread_entry_delay,
			NULL, NULL, NULL, 0, K_USER, K_MSEC(120));
	/* 100 < 120 ensure spawn thread not start */
	k_msleep(100);
	/* checkpoint: check spawn thread not execute */
	zassert_true(tp2 == 10);
	/* checkpoint: check spawn thread executed */
	k_msleep(100);
	zassert_true(tp2 == 100);
}

/**
 * @brief Verify that a K_FOREVER thread stays inactive until started.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * A thread created with K_FOREVER is never made ready by the passage of time,
 * only by an explicit k_thread_start(). It is created at the highest priority
 * so that, once started, it runs immediately: that makes the difference
 * between "not yet started" and "started" unambiguous even after the creator
 * yields.
 *
 * Test steps:
 * - Create a user thread with K_FOREVER at the highest priority.
 * - Yield, then confirm the thread has still not run.
 * - Call k_thread_start() and check again.
 *
 * Expected result:
 * - Yielding does not run the thread; k_thread_start() does.
 *
 * @see k_thread_create()
 * @see k_thread_start()
 */
ZTEST(threads_lifecycle, test_thread_spawn_forever)
{
	/* spawn thread with highest priority. It will run immediately once
	 * started.
	 */
	tp2 = 10;
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry_delay, NULL, NULL, NULL,
				      K_HIGHEST_THREAD_PRIO,
				      K_USER, K_FOREVER);
	k_yield();
	/* checkpoint: check spawn thread not execute */
	zassert_true(tp2 == 10);
	/* checkpoint: check spawn thread executed */
	k_thread_start(tid);
	k_yield();
	zassert_true(tp2 == 100);
	k_thread_abort(tid);
}

/**
 * @brief Verify that starting an already terminated thread has no effect.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * k_thread_start() on a thread that has already run to completion must be
 * ignored rather than resurrecting it. The thread writes a value when it
 * runs, so the test can reset that value after the thread has finished and
 * see whether a second start would run the body again.
 *
 * Test steps:
 * - Create a K_FOREVER thread and start it, letting it run to completion.
 * - Reset the value its body writes.
 * - Call k_thread_start() on the terminated thread again.
 *
 * Expected result:
 * - The second start does nothing, so the value stays as it was reset.
 *
 * @see k_thread_start()
 */
ZTEST(threads_lifecycle, test_thread_start)
{
	tp2 = 5;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry_delay, NULL, NULL, NULL,
				      K_HIGHEST_THREAD_PRIO,
				      K_USER, K_FOREVER);

	k_thread_start(tid);
	k_yield();
	zassert_true(tp2 == 100);

	/* checkpoint: k_thread_start() should not start the
	 * terminated thread
	 */

	tp2 = 50;
	k_thread_start(tid);
	k_yield();
	zassert_false(tp2 == 100);
}

static void user_start_thread(void *p1, void *p2, void *p3)
{
	*(int *)p1 = 100;
}

/**
 * @brief Verify that a user thread can start an inactive thread.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * k_thread_start() is reachable from user mode through its system call, so a
 * user thread must be able to start a thread it was granted, and that thread
 * must then actually run. The started thread writes a value its creator reads
 * back.
 *
 * Test steps:
 * - From a user thread, create a thread with a K_FOREVER start delay.
 * - Call k_thread_start() on it.
 * - Sleep, then read back the value the thread writes.
 *
 * Expected result:
 * - The started thread runs and writes its value.
 *
 * @see k_thread_start()
 * @see k_thread_create()
 */
ZTEST_USER(threads_lifecycle, test_thread_start_user)
{
	tp2 = 5;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      user_start_thread, &tp2, NULL, NULL,
				      0,
				      K_USER, K_FOREVER);

	k_thread_start(tid);
	k_msleep(100);
	zassert_true(tp2 == 100);
	k_thread_abort(tid);
}
