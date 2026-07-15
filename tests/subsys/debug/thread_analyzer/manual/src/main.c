/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/debug/thread_analyzer.h>

#define EXTRA_THREAD_STACKSIZE 2048
#define EXTRA_THREAD_NAME      "extra_thread"

struct k_thread extra_thread;
K_THREAD_STACK_DEFINE(extra_stack, EXTRA_THREAD_STACKSIZE);

static void thread_entry(void *p1, void *p2, void *p3)
{
	while (true) {
		k_sleep(K_SECONDS(300));
	}
}

static void *setup_fn(void)
{
	k_thread_create(&extra_thread, extra_stack, EXTRA_THREAD_STACKSIZE, thread_entry, NULL,
			NULL, NULL, K_PRIO_PREEMPT(0), 0, K_MSEC(0));
	k_thread_name_set(&extra_thread, EXTRA_THREAD_NAME);

	return NULL;
}

void thread_analyzer_callback_fn(struct thread_analyzer_info *info, void *user_data)
{
	if (strcmp(info->name, EXTRA_THREAD_NAME) != 0) {
		return;
	}

	size_t *thread_count = user_data;
	(*thread_count)++;
}

ZTEST(thread_analyzer_manual, test_manual_run)
{
	size_t thread_count_ud = 0;

	thread_analyzer_run(thread_analyzer_callback_fn, 0, &thread_count_ud);

	zassert_equal(thread_count_ud, 1,
		      "Expected to find exactly one thread with name " EXTRA_THREAD_NAME);
}

ZTEST_SUITE(thread_analyzer_manual, NULL, setup_fn, NULL, NULL, NULL);
