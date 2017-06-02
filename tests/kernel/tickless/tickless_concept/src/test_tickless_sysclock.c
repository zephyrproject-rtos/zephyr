/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_tickless
 * @{
 * @defgroup t_tickless_concept test_tickless_concept
 * @brief TestPurpose: verify tickless idle concepts
 * @details
https://www.zephyrproject.org/doc/subsystems/power_management.html#tickless-idle
 * @}
 */

#include <ztest.h>
#include <power.h>

#define STACK_SIZE 512
#define NUM_THREAD 4
static K_THREAD_STACK_ARRAY_DEFINE(tstack, NUM_THREAD, STACK_SIZE);
static struct k_thread tdata[NUM_THREAD];
/*for those not supporting tickless idle*/
#ifndef CONFIG_TICKLESS_IDLE
#define CONFIG_TICKLESS_IDLE_THRESH 20
#endif
/*sleep duration tickless*/
#define SLEEP_TICKLESS (CONFIG_TICKLESS_IDLE_THRESH)
/*sleep duration with tick*/
#define SLEEP_TICKFUL (CONFIG_TICKLESS_IDLE_THRESH-1)
/*slice size is set as half of the sleep duration*/
#define SLICE_SIZE (CONFIG_TICKLESS_IDLE_THRESH >> 1)
/*millisecond per tick*/
#define MSEC_PER_TICK (sys_clock_us_per_tick / USEC_PER_MSEC)
/*align to millisecond boundary*/
#define ALIGN_MS_BOUNDARY() \
	do {\
		u32_t t = k_uptime_get_32();\
		while (t == k_uptime_get_32())\
			;\
	} while (0)
K_SEM_DEFINE(sema, 0, NUM_THREAD);
static s64_t elapsed_slice;

static void thread_tslice(void *p1, void *p2, void *p3)
{
	s64_t t = k_uptime_delta(&elapsed_slice);

	TC_PRINT("elapsed slice %lld\n", t);
	/**TESTPOINT: verify slicing scheduler behaves as expected*/
	zassert_true(t >= SLICE_SIZE, NULL);
	/*less than one tick delay*/
	zassert_true(t <= (SLICE_SIZE + MSEC_PER_TICK), NULL);

	u32_t t32 = k_uptime_get_32();

	/*keep the current thread busy for more than one slice*/
	while (k_uptime_get_32() - t32 < SLEEP_TICKLESS)
		;
	k_sem_give(&sema);
}

/*test cases*/
void test_tickless_sysclock(void)
{
	volatile u32_t t0, t1;

	ALIGN_MS_BOUNDARY();
	t0 = k_uptime_get_32();
	k_sleep(SLEEP_TICKLESS);
	t1 = k_uptime_get_32();
	TC_PRINT("time %d, %d\n", t0, t1);
	/**TESTPOINT: verify system clock recovery after exiting tickless idle*/
	zassert_true((t1 - t0) >= SLEEP_TICKLESS, NULL);

	ALIGN_MS_BOUNDARY();
	t0 = k_uptime_get_32();
	k_sem_take(&sema, SLEEP_TICKFUL);
	t1 = k_uptime_get_32();
	TC_PRINT("time %d, %d\n", t0, t1);
	/**TESTPOINT: verify system clock recovery after exiting tickful idle*/
	zassert_true((t1 - t0) >= SLEEP_TICKFUL, NULL);
}

void test_tickless_slice(void)
{
	k_tid_t tid[NUM_THREAD];

	k_sem_reset(&sema);
	/*enable time slice*/
	k_sched_time_slice_set(SLICE_SIZE, K_PRIO_PREEMPT(0));

	/*create delayed threads with equal preemptive priority*/
	for (int i = 0; i < NUM_THREAD; i++) {
		tid[i] = k_thread_create(&tdata[i], tstack[i], STACK_SIZE,
			thread_tslice, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), 0, SLICE_SIZE);
	}
	k_uptime_delta(&elapsed_slice);
	/*relinquish CPU and wait for each thread to complete*/
	for (int i = 0; i < NUM_THREAD; i++) {
		k_sem_take(&sema, K_FOREVER);
	}

	/*test case teardown*/
	for (int i = 0; i < NUM_THREAD; i++) {
		k_thread_abort(tid[i]);
	}
	/*disable time slice*/
	k_sched_time_slice_set(0, K_PRIO_PREEMPT(0));
}
