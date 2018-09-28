/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#define STACK_SIZE (384 + CONFIG_TEST_EXTRA_STACKSIZE)
/* nrf 51 has lower ram, so creating less number of threads */
#if CONFIG_SRAM_SIZE <= 24
	#define NUM_THREAD 2
#elif (CONFIG_SRAM_SIZE <= 32) \
	|| defined(CONFIG_SOC_EMSK_EM7D)
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

	s64_t expected_slice_min = __ticks_to_ms(_ms_to_ticks(SLICE_SIZE));
	s64_t expected_slice_max = __ticks_to_ms(_ms_to_ticks(SLICE_SIZE) + 1);

	while (1) {
		s64_t tdelta = k_uptime_delta(&elapsed_slice);
		TC_PRINT("%c", thread_parameter);
		/* Test Fails if thread exceed allocated time slice or
		 * Any thread is scheduled out of order.
		 */
		zassert_true(((tdelta >= expected_slice_min) &&
			      (tdelta <= expected_slice_max) &&
			      ((int)p1 == thread_idx)), NULL);
		thread_idx = (thread_idx + 1) % (NUM_THREAD);
		u32_t t32 = k_uptime_get_32();

		/* Keep the current thread busy for more than one slice,
		 * even though, when timeslice used up the next thread
		 * should be scheduled in.
		 */
		while (k_uptime_get_32() - t32 < BUSY_MS) {
#if defined(CONFIG_ARCH_POSIX)
			posix_halt_cpu(); /*sleep until next irq*/
#else
			;
#endif
		}

		k_sem_give(&sema1);
	}

}

/*test cases*/

/**
 * @brief Check the behavior of preemptive threads when the
 * time slice is disabled and enabled
 *
 * @details Create multiple preemptive threads with same priorities
 * priorities and few with same priorities and enable the time slice.
 * Ensure that each thread is given the time slice period to execute.
 *
 * @ingroup kernel_sched_tests
 */
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

		/* Keep the current thread busy for more than one slice,
		 * even though, when timeslice used up the next thread
		 * should be scheduled in.
		 */
		t32 = k_uptime_get_32();
		while (k_uptime_get_32() - t32 < BUSY_MS) {
#if defined(CONFIG_ARCH_POSIX)
			posix_halt_cpu(); /*sleep until next irq*/
#else
			;
#endif
		}

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
