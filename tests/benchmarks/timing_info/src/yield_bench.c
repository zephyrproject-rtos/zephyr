/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <zephyr.h>
#include <tc_util.h>
#include <ksched.h>
#include "timing_info.h"

K_SEM_DEFINE(yield_sem, 0, 1);

/* To time thread creation*/
extern K_THREAD_STACK_DEFINE(my_stack_area, STACK_SIZE);
extern K_THREAD_STACK_DEFINE(my_stack_area_0, STACK_SIZE);
extern struct k_thread my_thread;
extern struct k_thread my_thread_0;

/* u64_t thread_yield_start_time[1000]; */
/* u64_t thread_yield_end_time[1000]; */
/* location of the time stamps*/
extern u32_t __read_swap_end_time_value;
extern u64_t __common_var_swap_end_time;
extern char sline[];

u64_t thread_sleep_start_time;
u64_t thread_sleep_end_time;
u64_t thread_start_time;
u64_t thread_end_time;
static u32_t count;

void thread_yield0_test(void *p1, void *p2, void *p3);
void thread_yield1_test(void *p1, void *p2, void *p3);

k_tid_t yield0_tid;
k_tid_t yield1_tid;
void yield_bench(void)
{
	/* Thread yield*/

	yield0_tid = k_thread_create(&my_thread, my_stack_area,
				     STACK_SIZE,
				     thread_yield0_test,
				     NULL, NULL, NULL,
				     0 /*priority*/, 0, 0);

	yield1_tid = k_thread_create(&my_thread_0, my_stack_area_0,
				     STACK_SIZE,
				     thread_yield1_test,
				     NULL, NULL, NULL,
				     0 /*priority*/, 0, 0);

	/*read the time of start of the sleep till the swap happens */
	__read_swap_end_time_value = 1;

	TIMING_INFO_PRE_READ();
	thread_sleep_start_time =   TIMING_INFO_OS_GET_TIME();
	k_sleep(1000);
	thread_sleep_end_time =   ((u32_t)__common_var_swap_end_time);

	u32_t yield_cycles = (thread_end_time - thread_start_time) / 2000;
	u32_t sleep_cycles = thread_sleep_end_time - thread_sleep_start_time;

	PRINT_STATS("Thread Yield", yield_cycles,
		CYCLES_TO_NS(yield_cycles));
	PRINT_STATS("Thread Sleep", sleep_cycles,
		CYCLES_TO_NS(sleep_cycles));

}


void thread_yield0_test(void *p1, void *p2, void *p3)
{
	k_sem_take(&yield_sem, 10);
	TIMING_INFO_PRE_READ();
	thread_start_time =  TIMING_INFO_OS_GET_TIME();
	while (count != 1000) {
		count++;
		k_yield();
	}
	TIMING_INFO_PRE_READ();
	thread_end_time =  TIMING_INFO_OS_GET_TIME();
	k_thread_abort(yield1_tid);
}

void thread_yield1_test(void *p1, void *p2, void *p3)
{
	k_sem_give(&yield_sem);
	while (1) {
		k_yield();
	}
}
