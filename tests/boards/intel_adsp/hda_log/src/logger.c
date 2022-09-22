/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_backend_adsp_hda.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <intel_adsp_ipc.h>
#include "tests.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hda_test, LOG_LEVEL_DBG);

/* Define a prime length message string (13 bytes long when including the \0 terminator)
 *
 * This helps ensure most if not all messages are not going to land on a 128 byte boundary
 * which is important to test the padding and wrapping feature
 */
#ifdef CONFIG_LOG_PRINTK
#define FMT_STR "T:%02ld:%06d\n"
#else
#define FMT_STR "T:%02ld:%07d"
#endif

#define FMT_STR_LEN 13

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

#define NUM_THREADS CONFIG_HDA_LOG_TEST_THREADS

K_THREAD_STACK_ARRAY_DEFINE(stacks, NUM_THREADS, STACK_SIZE);
static struct k_thread threads[NUM_THREADS];
static k_tid_t tids[NUM_THREADS];

/* Now define the number of iterations such that we ensure a large number of wraps
 * on the HDA ring.
 */
#define TEST_ITERATIONS CONFIG_HDA_LOG_TEST_ITERATIONS
#define TIMER_TEST_ITERATIONS 32

static void thread_func(void *p1, void *p2, void *p3)
{
	intptr_t id = (intptr_t)p1;


	for (int i = 0; i < TEST_ITERATIONS; i++) {
		if (IS_ENABLED(CONFIG_LOG_PRINTK)) {
			printk(FMT_STR, id, i);
		} else {
			LOG_INF(FMT_STR, id, i);
		}
#ifdef CONFIG_HDA_LOG_TEST_PANIC
		if (i > TEST_ITERATIONS/2 && id == 0) {
			log_panic();
		}
#endif /* CONFIG_HDA_LOG_TEST_PANIC */
	}
}

#ifdef CONFIG_HDA_LOG_TEST_TIMER
K_SEM_DEFINE(timer_sem, 0, 1);
static uint32_t timer_cnt;
static void timer_isr(struct k_timer *tm)
{
	LOG_INF(FMT_STR, -1l, timer_cnt);
	timer_cnt++;

	if (timer_cnt >= TIMER_TEST_ITERATIONS) {
		k_sem_give(&timer_sem);
		k_timer_stop(tm);
	}
}

static struct k_timer timer;
#endif /* CONFIG_HDA_LOG_TEST_TIMER */


ZTEST(intel_adsp_hda_log, test_hda_logger)
{
	k_msleep(CONFIG_LOG_BACKEND_ADSP_HDA_FLUSH_TIME*2);

	TC_PRINT("Testing hda log backend, log buffer size %u, threads %u,"
		 " iterations %u\n", CONFIG_LOG_BACKEND_ADSP_HDA_SIZE,
		 NUM_THREADS, TEST_ITERATIONS);

	/* Wait a moment so the output isn't mangled with the above printk */
	k_msleep(CONFIG_LOG_BACKEND_ADSP_HDA_FLUSH_TIME*2);

	uint64_t start = k_cycle_get_64();

#ifdef CONFIG_HDA_LOG_TEST_TIMER
	/* Show logging in an ISR working */
	k_timer_init(&timer, timer_isr, NULL);
	k_timer_start(&timer, K_MSEC(100), K_MSEC(100));
#endif /* CONFIG_HDA_LOG_TEST_TIMER */

	for (intptr_t i = 0; i < NUM_THREADS; i++) {
		tids[i] = k_thread_create(&threads[i], stacks[i], STACK_SIZE,
				thread_func, (void *)i, NULL, NULL,
				k_thread_priority_get(k_current_get()) + i,
				0, K_MSEC(10));
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		k_thread_join(tids[i], K_FOREVER);
	}

#ifdef CONFIG_HDA_LOG_TEST_TIMER
	k_sem_take(&timer_sem, K_FOREVER);
#endif

	uint64_t end = k_cycle_get_64();
	uint64_t delta = end - start;
	uint64_t msgs = TEST_ITERATIONS*NUM_THREADS + TIMER_TEST_ITERATIONS;
	uint64_t bytes_sent = msgs*FMT_STR_LEN;
	double seconds = ((double)delta / (double)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	double microseconds = seconds*1000000.0;
	double bytes_per_second = ((double)bytes_sent/seconds);

	/* Wait to display stats and "Test Finished"
	 * for at least the flush time * 2
	 */
	k_msleep(CONFIG_LOG_BACKEND_ADSP_HDA_FLUSH_TIME*2);

	TC_PRINT("HDA Log sent %llu msgs totalling %llu bytes in %u microseconds,"
		 " %u bytes/sec\n", msgs, bytes_sent,
		 (uint32_t)microseconds, (uint32_t)bytes_per_second);
}

ZTEST(intel_adsp_hda_log, test_hda_logger_flush)
{
	/* Wait for the flush to happen first */
	k_msleep(CONFIG_LOG_BACKEND_ADSP_HDA_FLUSH_TIME*2);

	/* Test the flush timer works by writing a short string */
	LOG_INF("Timeout flush working if shown");

	/* In deferred mode we must tell the logger to flush this can be
	 * done by calling log_panic().
	 */
#ifdef CONFIG_LOG_MODE_DEFERRED
	log_panic();
#endif

	/* Wait again for the flush to happen, if working the message is shown */
	k_msleep(CONFIG_LOG_BACKEND_ADSP_HDA_FLUSH_TIME*2);
}


ZTEST_SUITE(intel_adsp_hda_log, NULL, NULL, NULL, NULL, NULL);
