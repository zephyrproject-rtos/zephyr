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


#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME test
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

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

/**
 * @brief Verify immediate-mode logging is robust under thread preemption.
 *
 * @details
 * In immediate mode a log message is processed in the context that generated it.
 * This test creates several threads of different priorities that continuously
 * log text and hexdump data and then sleep, so that lower-priority threads are
 * frequently preempted mid-logging by higher-priority ones. It validates that
 * the subsystem handles such interleaved in-context processing without hitting
 * an assert or fault.
 *
 * Test steps:
 * - Create NUM_THREADS threads at staggered priorities, each logging in a loop.
 * - Let them run and preempt each other for a few seconds.
 * - Abort all threads.
 *
 * Expected result:
 * - The system survives frequent preemption during immediate logging with no
 *   assertion or fault.
 *
 * @see LOG_INF()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-8
 */
ZTEST(log_immediate, test_log_immediate_preemption)
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

ZTEST_SUITE(log_immediate, NULL, NULL, NULL, NULL, NULL);
