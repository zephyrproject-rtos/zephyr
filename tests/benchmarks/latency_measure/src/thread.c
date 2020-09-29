/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <timing/timing.h>
#include "utils.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
/* stack used by the threads */
static K_THREAD_STACK_DEFINE(t1_stack, STACK_SIZE);

static struct k_thread t1;

timing_t timestamp_start_create_c;
timing_t timestamp_end_create_c;
timing_t timestamp_start_start_c;
timing_t timestamp_end_start_c;
timing_t timestamp_start_suspend_c;
timing_t timestamp_end_suspend_c;
timing_t timestamp_start_resume_c;
timing_t timestamp_end_resume_c;

timing_t timestamp_start_abort_1;
timing_t timestamp_end_abort_1;

void thread_suspend_resume(void *p1, void *p2, void *p3)
{
	timestamp_start_suspend_c = timing_counter_get();
	k_thread_suspend(_current);

	/* comes to this line once its resumed*/
	timestamp_start_resume_c = timing_counter_get();

}

int suspend_resume(void)
{
	uint32_t diff;

	timing_start();

	timestamp_start_create_c = timing_counter_get();

	k_tid_t t1_tid = k_thread_create(&t1, t1_stack, STACK_SIZE,
					 thread_suspend_resume, NULL, NULL,
					 NULL, K_PRIO_PREEMPT(6), 0, K_FOREVER);

	timestamp_end_create_c = timing_counter_get();
	k_thread_name_set(t1_tid, "t1");

	timestamp_start_start_c = timing_counter_get();
	k_thread_start(t1_tid);

	timestamp_end_suspend_c = timing_counter_get();
	k_thread_resume(t1_tid);
	timestamp_end_resume_c = timing_counter_get();


	diff = timing_cycles_get(&timestamp_start_create_c,
				 &timestamp_end_create_c);
	PRINT_STATS("Time to create a thread (without start)", diff);

	diff = timing_cycles_get(&timestamp_start_start_c,
				 &timestamp_start_suspend_c);
	PRINT_STATS("Time to start a thread", diff);

	diff = timing_cycles_get(&timestamp_start_suspend_c,
				 &timestamp_end_suspend_c);
	PRINT_STATS("Time to suspend a thread", diff);

	diff = timing_cycles_get(&timestamp_start_resume_c,
				 &timestamp_end_resume_c);
	PRINT_STATS("Time to resume a thread", diff);

	timestamp_start_abort_1 = timing_counter_get();
	k_thread_abort(t1_tid);
	timestamp_end_abort_1 = timing_counter_get();

	diff = timing_cycles_get(&timestamp_start_abort_1,
				 &timestamp_end_abort_1);
	PRINT_STATS("Time to abort a thread (not running)", diff);

	timing_stop();
	return 0;
}
