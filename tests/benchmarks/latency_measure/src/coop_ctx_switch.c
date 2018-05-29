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

#include "timestamp.h"
#include "utils.h"

#include <arch/cpu.h>

/* number of context switches */
#define NCTXSWITCH   10000
#ifndef STACKSIZE
#define STACKSIZE    512
#endif

/* stack used by the threads */
static K_THREAD_STACK_DEFINE(thread_one_stack, STACKSIZE);
static K_THREAD_STACK_DEFINE(thread_two_stack, STACKSIZE);
static struct k_thread thread_one_data;
static struct k_thread thread_two_data;

static u32_t timestamp;

/* context switches counter */
static volatile u32_t ctx_switch_counter;

/* context switch balancer. Incremented by one thread, decremented by another*/
static volatile int ctx_switch_balancer;

K_SEM_DEFINE(sync_sema, 0, 1);

/**
 *
 * thread_onne
 *
 * Fiber makes all the test preparations: registers the interrupt handler,
 * gets the first timestamp and invokes the software interrupt.
 *
 * @return N/A
 */
static void thread_one(void)
{
	k_sem_take(&sync_sema, K_FOREVER);
	timestamp = TIME_STAMP_DELTA_GET(0);
	while (ctx_switch_counter < NCTXSWITCH) {
		k_yield();
		ctx_switch_counter++;
		ctx_switch_balancer--;
	}
	timestamp = TIME_STAMP_DELTA_GET(timestamp);
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
	PRINT_FORMAT(" 6 - Measure average context switch time between threads (coop)");
	ctx_switch_counter = 0;
	ctx_switch_balancer = 0;

	bench_test_start();
	k_thread_create(&thread_one_data, thread_one_stack, STACKSIZE,
			(k_thread_entry_t) thread_one, NULL, NULL, NULL,
			6, 0, K_NO_WAIT);
	k_thread_create(&thread_two_data, thread_two_stack, STACKSIZE,
			(k_thread_entry_t) thread_two, NULL, NULL, NULL,
			6, 0, K_NO_WAIT);

	if (ctx_switch_balancer > 3 || ctx_switch_balancer < -3) {
		PRINT_FORMAT(" Balance is %d. FAILED", ctx_switch_balancer);
	} else if (bench_test_end() != 0) {
		error_count++;
		PRINT_OVERFLOW_ERROR();
	} else {
		PRINT_FORMAT(" Average context switch time is %u tcs = %u"
			     " nsec",
			     timestamp / ctx_switch_counter,
			     SYS_CLOCK_HW_CYCLES_TO_NS_AVG(timestamp,
							   ctx_switch_counter));
	}

	return 0;
}
