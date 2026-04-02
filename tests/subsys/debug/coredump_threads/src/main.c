/*
 * Copyright Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define STACK_SIZE 256
#define THREAD_COUNT 7

static struct k_thread threads[THREAD_COUNT];
static uint32_t params[THREAD_COUNT];
static K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, THREAD_COUNT, STACK_SIZE);
static K_SEM_DEFINE(sem, 0, 1);

static void func0(uint32_t param)
{
	int ret = -EAGAIN;

	while (ret != 0) {
		ret = k_sem_take(&sem, K_NO_WAIT);
		k_sleep(K_MSEC(param));
	}

	k_panic();
}

static void test_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t *param = (uint32_t *)p1;

	func0(*param);
}

static void *coredump_threads_suite_setup(void)
{
	/* Spawn a few threads */
	for (int i = 0; i < THREAD_COUNT; i++) {
		params[i] = i;
		k_thread_create(
			&threads[i],
			thread_stacks[i],
			K_THREAD_STACK_SIZEOF(thread_stacks[i]),
			test_thread_entry,
			&params[i],
			NULL,
			NULL,
			(THREAD_COUNT - i), /* arbitrary priority */
			0,
			K_NO_WAIT);

		char thread_name[32];

		snprintf(thread_name, sizeof(thread_name), "thread%d", i);
		k_thread_name_set(&threads[i], thread_name);
	}

	return NULL;
}

ZTEST_SUITE(coredump_threads, NULL, coredump_threads_suite_setup, NULL, NULL, NULL);

ZTEST(coredump_threads, test_crash)
{
	/* Give semaphore allowing one of the waiting threads to continue and panic */
	k_sem_give(&sem);
	for (int i = 0; i < THREAD_COUNT; i++) {
		k_thread_join(&threads[i], K_FOREVER);
	}
}
