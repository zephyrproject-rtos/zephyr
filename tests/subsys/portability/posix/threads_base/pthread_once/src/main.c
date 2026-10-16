/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pthread.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

static pthread_once_t once_control = PTHREAD_ONCE_INIT;
static int attempt_count;
static int init_count;

static void init_routine(void)
{
	attempt_count++;
	if (attempt_count == 1) {
		/* First invocation exits to simulate failure/cancellation */
		pthread_exit(NULL);
	}
	init_count++;
}

static void *thread_func(void *p1)
{
	zassert_ok(pthread_once(&once_control, init_routine), "pthread_once failed");
	return NULL;
}

ZTEST(once, test_pthread_once_thread_safe)
{
	pthread_t thread1, thread2;

	init_count = 0;
	attempt_count = 0;

	/* Reset once_control just in case */
	pthread_once_t fresh_once = PTHREAD_ONCE_INIT;

	once_control = fresh_once;

	/* Spawn first thread which will exit during initialization */
	zassert_ok(pthread_create(&thread1, NULL, thread_func, NULL),
		   "attempt to create thread 1 failed");

	/* Wait for thread1 to finish (it will exit early) */
	zassert_ok(pthread_join(thread1, NULL), "failed to join thread 1");

	/* At this point, attempt_count should be 1 and init_count 0 */
	zassert_equal(attempt_count, 1, "attempt_count %d, expected 1", attempt_count);
	zassert_equal(init_count, 0, "init_count %d, expected 0", init_count);

	/* Spawn second thread which should successfully initialize */
	zassert_ok(pthread_create(&thread2, NULL, thread_func, NULL),
		   "attempt to create thread 2 failed");

	/* Wait for thread2 to finish */
	zassert_ok(pthread_join(thread2, NULL), "failed to join thread 2");

	/* At this point, attempt_count should be 2 and init_count 1 */
	zassert_equal(attempt_count, 2, "attempt_count %d, expected 2", attempt_count);
	zassert_equal(init_count, 1, "init_count %d, expected 1", init_count);
}

ZTEST_SUITE(once, NULL, NULL, NULL, NULL, NULL);
