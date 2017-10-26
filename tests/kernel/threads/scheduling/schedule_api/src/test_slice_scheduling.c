/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#define STACK_SIZE (384 + CONFIG_TEST_EXTRA_STACKSIZE)
/* nrf 51 has lower ram, so creating less number of threads */
#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_SERIES_STM32F3X)
	#define NUM_THREAD 3
#else
	#define NUM_THREAD 10
#endif
#define BASE_PRIORITY 0
#define ITRERATION_COUNT 5
static K_THREAD_STACK_ARRAY_DEFINE(tstack, NUM_THREAD, STACK_SIZE);
/* slice size in millisecond*/
#define SLICE_SIZE 200
/* busy for more than one slice*/
#define BUSY_MS (SLICE_SIZE + 20)
static struct k_thread t[NUM_THREAD];

static K_SEM_DEFINE(sema1, 0, NUM_THREAD);
/*elapsed_slice taken by last thread*/
static s64_t elapsed_slice;

static int thread_idx;

static void thread_tslice(void *p1, void *p2, void *p3)
{
	/*Print New line for last thread*/
	int thread_parameter = ((int)p1 == (NUM_THREAD - 1)) ? '\n' :
								((int)p1 + 'A');

	while (1) {
		s64_t tdelta = k_uptime_delta(&elapsed_slice);

		TC_PRINT("%c", thread_parameter);
		/* Test Fails if thread exceed allocated time slice or
		 * Any thread is scheduled out of order.
		 */
		zassert_true(((tdelta <= SLICE_SIZE) &&
			      ((int)p1 == thread_idx)), NULL);
		thread_idx = (thread_idx+1) % (NUM_THREAD);
		u32_t t32 = k_uptime_get_32();

		/* Keep the current thread busy for more than one slice,
		 * even though,  when timeslice used up the next thread
		 * should be scheduled in.
		 */
		while (k_uptime_get_32() - t32 < BUSY_MS)
			;

		k_sem_give(&sema1);
	}

}

/*test cases*/
void test_slice_scheduling(void)
{
	u32_t t32;
	k_tid_t tid[NUM_THREAD];
	int old_prio = k_thread_priority_get(k_current_get());
	int count = 0;

	/*disable timeslice*/
	k_sched_time_slice_set(0, K_PRIO_PREEMPT(0));

	/* update priority for current thread*/
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(BASE_PRIORITY));

	/* create threads with equal preemptive priority*/
	for (int i = 0; i < NUM_THREAD; i++) {
		tid[i] = k_thread_create(&t[i], tstack[i], STACK_SIZE,
			 thread_tslice, (void *)(intptr_t) i, NULL, NULL,
				 K_PRIO_PREEMPT(BASE_PRIORITY), 0, 0);
	}

	/* enable time slice*/
	k_sched_time_slice_set(SLICE_SIZE, K_PRIO_PREEMPT(BASE_PRIORITY));

	while (count < ITRERATION_COUNT) {
		k_uptime_delta(&elapsed_slice);

		/* current thread (ztest native) consumed a half timeslice*/
		t32 = k_uptime_get_32();
		while (k_uptime_get_32() - t32 < SLICE_SIZE)
			;

		/* relinquish CPU and wait for each thread to complete*/
		for (int i = 0; i < NUM_THREAD; i++) {
			k_sem_take(&sema1, K_FOREVER);
		}
		count++;
	}


	/* test case teardown*/
	for (int i = 0; i < NUM_THREAD; i++) {
		k_thread_abort(tid[i]);
	}

	/* disable time slice*/
	k_sched_time_slice_set(0, K_PRIO_PREEMPT(0));

	k_thread_priority_set(k_current_get(), old_prio);
}
