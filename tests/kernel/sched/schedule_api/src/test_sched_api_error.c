/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_sched.h"

#define THREAD_TEST_PRIORITY 0
static bool after_test;
static struct k_thread tdata;

static void thread_resume_unsuspend(void *p1, void *p2, void *p3)
{
	k_thread_resume(k_current_get());
	after_test = true;
}

/**
 * @brief Test k_thread_resume() API
 *
 * @details resume a thread which is not suspended will directly return
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_thread_resume()
 */
void test_k_thread_resume_unsuspend(void)
{
	after_test = false;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)thread_resume_unsuspend,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			0, K_NO_WAIT);

	zassert_false(after_test, "child thread didn't return");

	k_thread_join(tid, K_FOREVER);
}
