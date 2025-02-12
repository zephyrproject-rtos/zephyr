/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/pm/pm.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define NUM_THREAD 4
static K_THREAD_STACK_ARRAY_DEFINE(tstack, NUM_THREAD, STACK_SIZE);
static struct k_thread tdata[NUM_THREAD];

#define IDLE_THRESH 20

/*sleep duration tickless*/
#define SLEEP_TICKLESS	 k_ticks_to_ms_floor64(IDLE_THRESH)

/*sleep duration with tick*/
#define SLEEP_TICKFUL	 k_ticks_to_ms_floor64(IDLE_THRESH - 1)

/*slice size is set as half of the sleep duration*/
#define SLICE_SIZE	 k_ticks_to_ms_floor64(IDLE_THRESH >> 1)

/*maximum slice duration accepted by the test*/
#define SLICE_SIZE_LIMIT k_ticks_to_ms_floor64((IDLE_THRESH >> 1) + 1)

/*align to millisecond boundary*/
#define ALIGN_MS_BOUNDARY()		       \
	do {				       \
		uint32_t t = k_uptime_get_32();   \
		while (t == k_uptime_get_32()) \
			Z_SPIN_DELAY(50);       \
	} while (0)

K_SEM_DEFINE(sema, 0, NUM_THREAD);
static int64_t elapsed_slice;

static void thread_tslice(void *p1, void *p2, void *p3)
{
	int64_t t = k_uptime_delta(&elapsed_slice);

	TC_PRINT("elapsed slice %" PRId64 ", expected: <%" PRId64 ", %" PRId64 ">\n",
		t, SLICE_SIZE, SLICE_SIZE_LIMIT);

	/**TESTPOINT: verify slicing scheduler behaves as expected*/
	zassert_true(t >= SLICE_SIZE);
	/*less than one tick delay*/
	zassert_true(t <= SLICE_SIZE_LIMIT);

	/*keep the current thread busy for more than one slice*/
	k_busy_wait(1000 * SLEEP_TICKLESS);
	k_sem_give(&sema);
}
/**
 * @defgroup  kernel_tickless_tests Tickless
 * @ingroup all_tests
 * @{
 */


/**
 * @brief Verify system clock with and without tickless idle
 *
 * @details Check if system clock recovers and works as expected
 * when tickless idle is enabled and disabled.
 */
ZTEST(tickless_concept, test_tickless_sysclock)
{
	volatile uint32_t t0, t1;

	ALIGN_MS_BOUNDARY();
	t0 = k_uptime_get_32();
	k_msleep(SLEEP_TICKLESS);
	t1 = k_uptime_get_32();
	TC_PRINT("time %d, %d\n", t0, t1);
	/**TESTPOINT: verify system clock recovery after exiting tickless idle*/
	zassert_true((t1 - t0) >= SLEEP_TICKLESS);

	ALIGN_MS_BOUNDARY();
	t0 = k_uptime_get_32();
	k_sem_take(&sema, K_MSEC(SLEEP_TICKFUL));
	t1 = k_uptime_get_32();
	TC_PRINT("time %d, %d\n", t0, t1);
	/**TESTPOINT: verify system clock recovery after exiting tickful idle*/
	zassert_true((t1 - t0) >= SLEEP_TICKFUL);
}

/**
 * @brief Verify tickless functionality with time slice
 *
 * @details Create threads of equal priority and enable time
 * slice. Check if the threads execute more than a tick.
 */
ZTEST(tickless_concept, test_tickless_slice)
{
	k_tid_t tid[NUM_THREAD];

	k_sem_reset(&sema);
	/*enable time slice*/
	k_sched_time_slice_set(SLICE_SIZE, K_PRIO_PREEMPT(0));

	/*create delayed threads with equal preemptive priority*/
	for (int i = 0; i < NUM_THREAD; i++) {
		tid[i] = k_thread_create(&tdata[i], tstack[i], STACK_SIZE,
					 thread_tslice, NULL, NULL, NULL,
					 K_PRIO_PREEMPT(0), 0,
					 K_MSEC(SLICE_SIZE));
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

/**
 * @}
 */

ZTEST_SUITE(tickless_concept, NULL, NULL,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
