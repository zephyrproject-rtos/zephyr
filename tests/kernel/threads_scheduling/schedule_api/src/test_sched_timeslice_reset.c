/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#define STACK_SIZE 512
#define NUM_THREAD 3
static K_THREAD_STACK_ARRAY_DEFINE(tstack, NUM_THREAD, STACK_SIZE);
/* slice size in millisecond*/
#define SLICE_SIZE 200
/* busy for more than one slice*/
#define BUSY_MS (SLICE_SIZE+20)
/* a half timeslice*/
#define HALF_SLICE_SIZE (SLICE_SIZE >> 1)

K_SEM_DEFINE(sema, 0, NUM_THREAD);
/*elapsed_slice taken by last thread*/
static s64_t elapsed_slice;
/*expected elapsed duration*/
static s64_t expected_slice[NUM_THREAD] = {
	HALF_SLICE_SIZE,/* the ztest native thread taking a half timeslice*/
	SLICE_SIZE,     /* the spawned thread taking a full timeslice, reset*/
	SLICE_SIZE      /* the spawned thread taking a full timeslice, reset*/
};
static int thread_idx;

static void thread_tslice(void *p1, void *p2, void *p3)
{
	s64_t t = k_uptime_delta(&elapsed_slice);

	#ifdef CONFIG_DEBUG
	TC_PRINT("thread[%d] elapsed slice %lld, ", thread_idx, t);
	TC_PRINT("expected %lld\n", expected_slice[thread_idx]);
	#endif
	/** TESTPOINT: timeslice should be reset for each preemptive thread*/
	zassert_true(t <= expected_slice[thread_idx], NULL);
	thread_idx = (thread_idx + 1) % NUM_THREAD;

	u32_t t32 = k_uptime_get_32();

	/* Keep the current thread busy for more than one slice, even though,
	 * when timeslice used up the next thread should be scheduled in.
	 */
	while (k_uptime_get_32() - t32 < BUSY_MS)
		;
	k_sem_give(&sema);
}

/*test cases*/

void test_slice_reset(void)
{
	u32_t t32;
	k_tid_t tid[NUM_THREAD];
	struct k_thread t[NUM_THREAD];
	int old_prio = k_thread_priority_get(k_current_get());

	thread_idx = 0;
	/*disable timeslice*/
	k_sched_time_slice_set(0, K_PRIO_PREEMPT(0));

	for (int j = 0; j < 2; j++) {
		k_sem_reset(&sema);
		/* update priority for current thread*/
		k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(j));
		/* create delayed threads with equal preemptive priority*/
		for (int i = 0; i < NUM_THREAD; i++) {
			tid[i] = k_thread_create(&t[i], tstack[i], STACK_SIZE,
				thread_tslice, NULL, NULL, NULL,
				K_PRIO_PREEMPT(j), 0, 0);
		}
		/* enable time slice*/
		k_sched_time_slice_set(SLICE_SIZE, K_PRIO_PREEMPT(0));
		k_uptime_delta(&elapsed_slice);

		/* current thread (ztest native) consumed a half timeslice*/
		t32 = k_uptime_get_32();
		while (k_uptime_get_32() - t32 < HALF_SLICE_SIZE)
			;

		/* relinquish CPU and wait for each thread to complete*/
		for (int i = 0; i < NUM_THREAD; i++) {
			k_sem_take(&sema, K_FOREVER);
		}

		/* test case teardown*/
		for (int i = 0; i < NUM_THREAD; i++) {
			k_thread_abort(tid[i]);
		}
		/* disable time slice*/
		k_sched_time_slice_set(0, K_PRIO_PREEMPT(0));
	}
	k_thread_priority_set(k_current_get(), old_prio);
}
