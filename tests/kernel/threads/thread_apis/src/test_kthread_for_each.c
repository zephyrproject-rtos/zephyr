/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/debug/stack.h>

#include "tests_thread_apis.h"

#define SLEEP_MS 100
#define TEST_STRING "TEST"
#define TEST_STRING_UNLOCKED "TEST_UNLOCKED"

static int tcount;
static bool thread_flag;
static bool create_thread;
static k_tid_t in_callback_tid;

struct k_thread tdata1;
K_THREAD_STACK_DEFINE(tstack1, STACK_SIZE);

static void thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_msleep(SLEEP_MS);
}

static void thread_callback(const struct k_thread *thread, void *user_data)
{
	char *str = (char *)user_data;

	if (thread == &tdata) {
		TC_PRINT("%s: Newly added thread found\n", str);
		TC_PRINT("%s: tid: %p, prio: %d\n",
				str, thread, thread->base.prio);
		thread_flag = true;
	}
	tcount++;
}

static
void thread_callback_unlocked(const struct k_thread *thread, void *user_data)
{
	char *str = (char *)user_data;

	if (create_thread) {
		in_callback_tid = k_thread_create(&tdata1, tstack1,
					STACK_SIZE,
					thread_entry,
					NULL, NULL, NULL, K_PRIO_PREEMPT(0),
					0, K_NO_WAIT);
		create_thread = false;
	}

	if (thread == &tdata) {
		TC_PRINT("%s: Newly added thread found\n", str);
		TC_PRINT("%s: tid: %p, prio: %d\n",
				str, thread, thread->base.prio);
		thread_flag = true;
	}

	if (thread == &tdata1) {
		TC_PRINT("%s: Newly added thread in callback found\n", str);
		TC_PRINT("%s: tid: %p, prio: %d\n",
				str, thread, thread->base.prio);
		thread_flag = true;
		k_thread_abort(in_callback_tid);
	}
	tcount++;
}

/**
 * @brief Verify that k_thread_foreach() visits every thread, including new
 *        ones.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * The iterator has to walk the kernel's whole thread list and invoke the
 * caller's callback once per thread, picking up threads created since a
 * previous walk. Counting the callbacks before and after creating a thread
 * makes that growth observable. The iteration runs with the scheduler locked,
 * so the callback must not create or abort threads.
 *
 * Test steps:
 * - Call k_thread_foreach() and record how many threads were visited.
 * - Create an additional thread.
 * - Call k_thread_foreach() again and compare the new count.
 *
 * Expected result:
 * - The first walk visits at least one thread, and the second visits exactly
 *   one more than the first.
 *
 * @see k_thread_foreach()
 */
ZTEST(threads_lifecycle_1cpu, test_thread_foreach)
{
	int count;

	k_thread_foreach(thread_callback, TEST_STRING);

	/* Check thread_count non-zero, thread_flag
	 * and stack_flag are not set.
	 */
	zassert_true(tcount && !thread_flag,
				"thread_callback() not getting called");
	/* Save the initial thread count */
	count = tcount;

	/* Create new thread which should add a new entry to the thread list */
	k_tid_t tid = k_thread_create(&tdata, tstack,
			STACK_SIZE, thread_entry, NULL,
			NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	k_msleep(1);

	/* Call k_thread_foreach() and check
	 * thread_callback is getting called for
	 * the newly added thread.
	 */
	tcount = 0;
	k_thread_foreach(thread_callback, TEST_STRING);

	/* Check thread_count > temp, thread_flag and stack_flag are set */
	zassert_true((tcount > count) && thread_flag,
					"thread_callback() not getting called");
	k_thread_abort(tid);
}

/**
 * @brief Verify that k_thread_foreach_unlocked() iterates without holding the
 *        scheduler lock.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * The unlocked iterator visits the same threads as the locked one but leaves
 * the scheduler unlocked between callbacks, which is what makes it legal for
 * the callback to create or abort threads. The callback used here aborts a
 * thread while the walk is in progress, so a pass shows the iteration
 * tolerates the list changing underneath it.
 *
 * Test steps:
 * - Call k_thread_foreach_unlocked() and record how many threads were visited.
 * - Create an additional thread.
 * - Call it again with a callback that aborts a thread mid-iteration.
 * - Compare the counts.
 *
 * Expected result:
 * - The first walk visits at least one thread, the second visits exactly one
 *   more, and aborting from the callback does not disturb the walk.
 *
 * @see k_thread_foreach_unlocked()
 */
ZTEST(threads_lifecycle_1cpu, test_thread_foreach_unlocked)
{
	int count;

	thread_flag = false;
	tcount = 0;
	k_thread_foreach_unlocked(thread_callback_unlocked,
				  TEST_STRING_UNLOCKED);

	/* Check thread_count non-zero, thread_flag
	 * and stack_flag are not set.
	 */
	zassert_true(tcount && !thread_flag,
				"thread_callback() not getting called");
	/* Save the initial thread count */
	count = tcount;

	/* Create new thread which should add a new entry to the thread list */
	k_tid_t tid = k_thread_create(&tdata, tstack,
			STACK_SIZE, thread_entry, NULL,
			NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	k_msleep(1);

	/* Call k_thread_foreach() and check
	 * thread_callback is getting called for
	 * the newly added thread.
	 * meanwhile, a new thread is created in callback but
	 * it is not be counted in this iteration
	 */
	tcount = 0;
	create_thread = true;
	k_thread_foreach_unlocked(thread_callback_unlocked,
				  TEST_STRING_UNLOCKED);

	/* Check thread_count > temp, thread_flag and stack_flag are set */
	zassert_true((tcount > count) && thread_flag,
					"thread_callback() not getting called");

	/* thread_count increase again,
	 * as there is a thread is created in last iteration
	 */
	tcount = 0;
	k_thread_foreach_unlocked(thread_callback_unlocked,
				  TEST_STRING_UNLOCKED);
	zassert_true((tcount > count) && thread_flag,
					"thread_callback() not getting called");
	k_thread_abort(tid);
}

/**
 * @brief Verify that k_thread_foreach() rejects a NULL callback.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * There is nothing sensible to do with a NULL callback, so the iterator
 * asserts on it rather than walking the thread list and dereferencing it. The
 * assertion aborts this thread through the fatal error path, which the test
 * harness expects.
 *
 * Test steps:
 * - Call k_thread_foreach() with a NULL callback.
 *
 * Expected result:
 * - The call raises the expected fatal error and does not return.
 *
 * @see k_thread_foreach()
 */
ZTEST(threads_lifecycle_1cpu, test_thread_foreach_null_cb)
{
	k_thread_foreach(NULL, TEST_STRING);
}

/**
 * @brief Verify that k_thread_foreach_unlocked() rejects a NULL callback.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * The unlocked iterator has to reject a NULL callback exactly as the locked
 * one does, asserting instead of walking the thread list and dereferencing it.
 *
 * Test steps:
 * - Call k_thread_foreach_unlocked() with a NULL callback.
 *
 * Expected result:
 * - The call raises the expected fatal error and does not return.
 *
 * @see k_thread_foreach_unlocked()
 */
ZTEST(threads_lifecycle_1cpu, test_thread_foreach_unlocked_null_cb)
{
	k_thread_foreach_unlocked(NULL, TEST_STRING_UNLOCKED);
}

/**
 * @brief Verify that k_thread_state_str() names every thread state.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * k_thread_state_str() renders a thread's state bits as text for logs and
 * shell output, so every state and combination it knows about has to produce
 * the right string. Driving a real thread through each state in turn is not
 * possible, so the state field of a spare thread object is set directly and
 * the rendered string is compared against the expected name.
 *
 * Test steps:
 * - For each thread state, write it into a spare thread object's state field.
 * - Call k_thread_state_str() with a caller-provided buffer.
 * - Compare the returned string with the expected text.
 *
 * Expected result:
 * - Every state, including combinations and the empty state, renders as its
 *   documented name.
 *
 * @see k_thread_state_str()
 */
ZTEST(threads_lifecycle_1cpu, test_thread_state_str)
{
	char state_str[32];
	const char *str;
	k_tid_t tid = &tdata1;

	tid->base.thread_state = 0;
	str = k_thread_state_str(tid, state_str, sizeof(state_str));
	zassert_str_equal(str, "");

	tid->base.thread_state = _THREAD_DUMMY;

	str = k_thread_state_str(tid, NULL, sizeof(state_str));
	zassert_str_equal(str, "");

	str = k_thread_state_str(tid, state_str, 0);
	zassert_str_equal(str, "");

	str = k_thread_state_str(tid, state_str, sizeof(state_str));
	zassert_str_equal(str, "dummy");

	tid->base.thread_state = _THREAD_PENDING;
	str = k_thread_state_str(tid, state_str, sizeof(state_str));
	zassert_str_equal(str, "pending");

	tid->base.thread_state = _THREAD_DEAD;
	str = k_thread_state_str(tid, state_str, sizeof(state_str));
	zassert_str_equal(str, "dead");

	tid->base.thread_state = _THREAD_SLEEPING;
	str = k_thread_state_str(tid, state_str, sizeof(state_str));
	zassert_str_equal(str, "sleeping");

	tid->base.thread_state = _THREAD_SUSPENDED;
	str = k_thread_state_str(tid, state_str, sizeof(state_str));
	zassert_str_equal(str, "suspended");

	tid->base.thread_state = _THREAD_ABORTING;
	str = k_thread_state_str(tid, state_str, sizeof(state_str));
	zassert_str_equal(str, "aborting");

	tid->base.thread_state = _THREAD_QUEUED;
	str = k_thread_state_str(tid, state_str, sizeof(state_str));
	zassert_str_equal(str, "queued");

	tid->base.thread_state = _THREAD_PENDING | _THREAD_SUSPENDED;
	str = k_thread_state_str(tid, state_str, sizeof(state_str));
	zassert_str_equal(str, "pending+suspended");
}
