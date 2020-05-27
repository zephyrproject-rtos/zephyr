/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log immediate
 *
 */


#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>
#include <ztest.h>
#include <logging/log_backend.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>

#define LOG_MODULE_NAME test
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)

#define NUM_THREADS 5

K_THREAD_STACK_ARRAY_DEFINE(stacks, NUM_THREADS, STACK_SIZE);
static struct k_thread threads[NUM_THREADS];
static k_tid_t tids[NUM_THREADS];


/* Thread entry point, used for multiple threads. Thread is logging some data
 * (data length varies for each thread) and sleeps. Threads have different
 * priorities so on wakeup other thread will be preempted, interrupting logging.
 */
static void thread_func(void *p1, void *p2, void *p3)
{
	intptr_t id = (intptr_t)p1;
	int buf_len = 8*id + 8;
	uint8_t *buf = alloca(buf_len);

	while (1) {
		LOG_INF("test string printed %d %d %p", 1, 2, k_current_get());
		LOG_HEXDUMP_INF(buf, buf_len, "data:");
		k_msleep(20+id);
	}
}

/*
 * Test create number of threads with different priorities. Each thread logs
 * data and sleeps. This creates environment where multiple threads are
 * preempted during logging (in immediate mode). Test checks that system does
 * not hit any assert or other fault during frequent preemptions.
 */
static void test_log_immediate_preemption(void)
{
	if (!IS_ENABLED(CONFIG_LOG_IMMEDIATE_CLEAN_OUTPUT)) {
		LOG_INF("CONFIG_LOG_IMMEDIATE_CLEAN_OUTPUT not enabled."
			" Text output will be garbled.");
	}
	for (intptr_t i = 0; i < NUM_THREADS; i++) {
		tids[i] = k_thread_create(&threads[i], stacks[i], STACK_SIZE,
				thread_func, (void *)i, NULL, NULL,
				k_thread_priority_get(k_current_get()) + i,
				0, K_MSEC(10));
	}
	k_msleep(3000);

	for (int i = 0; i < NUM_THREADS; i++) {
		k_thread_abort(tids[i]);
	}
	zassert_true(true, "");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_log_immediate,
			 ztest_unit_test(test_log_immediate_preemption));
	ztest_run_test_suite(test_log_immediate);
}
