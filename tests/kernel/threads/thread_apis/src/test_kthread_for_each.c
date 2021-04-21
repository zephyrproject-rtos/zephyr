/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>
#include <debug/stack.h>

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
					(k_thread_entry_t)thread_entry,
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
 * @ingroup kernel_thread_tests
 * @brief Test k_thread_foreach API
 *
 * @details Call k_thread_foreach() at the beginning of the test and
 * call it again after creating a thread, See k_thread_foreach()
 * iterates over the newly created thread and calls the user passed
 * callback function.
 *
 * @see k_thread_foreach()
 */
void test_k_thread_foreach(void)
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
			STACK_SIZE, (k_thread_entry_t)thread_entry, NULL,
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
 * @brief Test k_thread_foreach_unlock API
 *
 * @details Call k_thread_foreach_unlocked() at the beginning of the test and
 * call it again after creating a thread, See k_thread_foreach_unlocked()
 * iterates over the newly created thread and calls the user passed
 * callback function.
 * In contrast to k_thread_foreach(), k_thread_foreach_unlocked() allow
 * callback function created or abort threads
 *
 * @see k_thread_foreach_unlocked()
 * @ingroup kernel_thread_tests
 */
void test_k_thread_foreach_unlocked(void)
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
			STACK_SIZE, (k_thread_entry_t)thread_entry, NULL,
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
 * @brief Test k_thread_foreach API with null callback
 *
 * @details Call k_thread_foreach() with null callback will triger __ASSERT()
 * and this test thread will be aborted by z_fatal_error()
 * @see k_thread_foreach()
 * @ingroup kernel_thread_tests
 */

void test_k_thread_foreach_null_cb(void)
{
	k_thread_foreach(NULL, TEST_STRING);
}

/**
 * @brief Test k_thread_foreach_unlocked API with null callback
 *
 * @details Call k_thread_foreach_unlocked() with null callback will triger
 * __ASSERT() and this test thread will be aborted by z_fatal_error()
 *
 * @see k_thread_foreach_unlocked()
 * @ingroup kernel_thread_tests
 */

void test_k_thread_foreach_unlocked_null_cb(void)
{
	k_thread_foreach_unlocked(NULL, TEST_STRING_UNLOCKED);
}

/**
 * @brief Test k_thread_state_str API with null callback
 *
 * @details It's impossible to sched a thread step by step manually to
 * experence each state from _THREAD_PRESTART to _THREAD_DEAD. To cover each
 * line of function k_thread_state_str(), set thread_state of tdata1 and check
 * the string this function returns
 *
 * @see k_thread_state_str()
 * @ingroup kernel_thread_tests
 */
void test_k_thread_state_str(void)
{
	k_tid_t tid = &tdata1;

	tid->base.thread_state = 0;
	zassert_true(strcmp(k_thread_state_str(tid), "") == 0, NULL);

	tid->base.thread_state = _THREAD_DUMMY;
	zassert_true(strcmp(k_thread_state_str(tid), "dummy") == 0, NULL);

	tid->base.thread_state = _THREAD_PENDING;
	zassert_true(strcmp(k_thread_state_str(tid), "pending") == 0, NULL);

	tid->base.thread_state = _THREAD_PRESTART;
	zassert_true(strcmp(k_thread_state_str(tid), "prestart") == 0, NULL);

	tid->base.thread_state = _THREAD_DEAD;
	zassert_true(strcmp(k_thread_state_str(tid), "dead") == 0, NULL);

	tid->base.thread_state = _THREAD_SUSPENDED;
	zassert_true(strcmp(k_thread_state_str(tid), "suspended") == 0, NULL);

	tid->base.thread_state = _THREAD_ABORTING;
	zassert_true(strcmp(k_thread_state_str(tid), "aborting") == 0, NULL);

	tid->base.thread_state = _THREAD_QUEUED;
	zassert_true(strcmp(k_thread_state_str(tid), "queued") == 0, NULL);

	tid->base.thread_state = 0xFF;
	zassert_true(strcmp(k_thread_state_str(tid), "unknown") == 0, NULL);
}
