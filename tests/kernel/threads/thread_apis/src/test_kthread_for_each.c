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

static int tcount;
static bool thread_flag;

static void thread_entry(void *p1, void *p2, void *p3)
{
	k_sleep(SLEEP_MS);
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
	k_sleep(K_MSEC(1));

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

