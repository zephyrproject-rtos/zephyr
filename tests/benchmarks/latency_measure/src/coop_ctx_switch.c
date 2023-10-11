/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Measure context switch time between cooperative threads
 *
 * This file contains thread (coop) context switch time measurement.
 * The thread starts two cooperative thread. One thread waits on a semaphore. The other,
 * after starting, releases a semaphore which enable the first thread to run.
 * Each thread increases a common global counter and context switch back and
 * forth by yielding the cpu. When counter reaches the maximal value, threads
 * stop and the average time of context switch is displayed.
 */
#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include "utils.h"

/* number of context switches */
#define NCTXSWITCH 10000
#ifndef STACKSIZE
#define STACKSIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#endif

/* stack used by the threads */
static K_THREAD_STACK_DEFINE(thread_one_stack, STACKSIZE);
static K_THREAD_STACK_DEFINE(thread_two_stack, STACKSIZE);
static struct k_thread thread_one_data;
static struct k_thread thread_two_data;

static timing_t timestamp_start;
static timing_t timestamp_end;

/* context switches counter */
static volatile uint32_t ctx_switch_counter;

/* context switch balancer. Incremented by one thread, decremented by another*/
static volatile int ctx_switch_balancer;

K_SEM_DEFINE(sync_sema, 0, 1);

/**
 *
 * thread_one
 *
 * Fiber makes all the test preparations: registers the interrupt handler,
 * gets the first timestamp and invokes the software interrupt.
 *
 */
static void thread_one(void)
{
	k_sem_take(&sync_sema, K_FOREVER);

	timestamp_start = timing_counter_get();

	while (ctx_switch_counter < NCTXSWITCH) {
		k_yield();
		ctx_switch_counter++;
		ctx_switch_balancer--;
	}

	timestamp_end = timing_counter_get();
}

/**
 *
 * @brief Check the time when it gets executed after the semaphore
 *
 * Fiber starts, waits on semaphore. When the interrupt handler releases
 * the semaphore, thread  measures the time.
 *
 * @return 0 on success
 */
static void thread_two(void)
{
	k_sem_give(&sync_sema);
	while (ctx_switch_counter < NCTXSWITCH) {
		k_yield();
		ctx_switch_counter++;
		ctx_switch_balancer++;
	}
}

/**
 *
 * @brief The test main function
 *
 * @return 0 on success
 */
int coop_ctx_switch(void)
{
	ctx_switch_counter = 0U;
	ctx_switch_balancer = 0;
	char error_string[80];
	const char *notes = "";
	bool failed = false;
	int  end;

	timing_start();
	bench_test_start();

	k_thread_create(&thread_one_data, thread_one_stack, STACKSIZE,
			(k_thread_entry_t)thread_one, NULL, NULL, NULL,
			K_PRIO_COOP(6), 0, K_NO_WAIT);
	k_thread_create(&thread_two_data, thread_two_stack, STACKSIZE,
			(k_thread_entry_t)thread_two, NULL, NULL, NULL,
			K_PRIO_COOP(6), 0, K_NO_WAIT);

	end = bench_test_end();

	if (ctx_switch_balancer > 3 || ctx_switch_balancer < -3) {
		error_count++;
		snprintk(error_string, 78, " Balance is %d",
			 ctx_switch_balancer);
		notes = error_string;
		failed = true;
	} else if (end != 0) {
		error_count++;
		notes = TICK_OCCURRENCE_ERROR;
	}

	uint32_t diff;

	diff = timing_cycles_get(&timestamp_start, &timestamp_end);
	PRINT_STATS_AVG("Average context switch time between threads (coop)",
			diff, ctx_switch_counter, failed, notes);

	timing_stop();

	return 0;
}
