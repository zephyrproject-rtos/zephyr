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


extern char sline[];

K_SEM_DEFINE(sem_bench, 0, 1);
K_SEM_DEFINE(sem_bench_1, 0, 1);

/* To time thread creation*/
extern K_THREAD_STACK_DEFINE(my_stack_area, STACK_SIZE);
extern K_THREAD_STACK_DEFINE(my_stack_area_0, STACK_SIZE);
extern struct k_thread my_thread;
extern struct k_thread my_thread_0;

/* u64_t thread_yield_start_time[1000]; */
/* u64_t thread_yield_end_time[1000]; */
u64_t thread_start_time;
u64_t thread_end_time;
u64_t sem_start_time;
u64_t sem_end_time;
u64_t sem_give_start_time;
u64_t sem_give_end_time;

u32_t swap_called;
u64_t test_time2;
u64_t test_time1;

void thread_sem0_test(void *p1, void *p2, void *p3);
void thread_sem1_test(void *p1, void *p2, void *p3);
void thread_sem0_give_test(void *p1, void *p2, void *p3);
void thread_sem1_give_test(void *p1, void *p2, void *p3);

k_tid_t sem0_tid;
k_tid_t sem1_tid;

extern u64_t __common_var_swap_end_time;
extern u32_t __read_swap_end_time_value;

void semaphore_bench(void)
{

	/* Thread yield*/

	sem0_tid = k_thread_create(&my_thread, my_stack_area,
				   STACK_SIZE,
				   thread_sem0_test, NULL, NULL, NULL,
				   2 /*priority*/, 0, 0);
	sem1_tid = k_thread_create(&my_thread_0, my_stack_area_0,
				   STACK_SIZE, thread_sem1_test,
				   NULL, NULL, NULL,
				   2 /*priority*/, 0, 0);

	k_sleep(1000);


	/* u64_t test_time1 = _tsc_read(); */
	sem_end_time = (__common_var_swap_end_time);
	u32_t sem_cycles = sem_end_time - sem_start_time;

	sem0_tid = k_thread_create(&my_thread, my_stack_area,
				   STACK_SIZE, thread_sem0_give_test,
				   NULL, NULL, NULL,
				   2 /*priority*/, 0, 0);
	sem1_tid = k_thread_create(&my_thread_0, my_stack_area_0,
				   STACK_SIZE, thread_sem1_give_test,
				   NULL, NULL, NULL,
				   2 /*priority*/, 0, 0);

	k_sleep(1000);
	sem_give_end_time = (__common_var_swap_end_time);
	u32_t sem_give_cycles = sem_give_end_time - sem_give_start_time;


	/* Semaphore without context switch*/
	TIMING_INFO_PRE_READ();
	u32_t sem_give_wo_cxt_start = TIMING_INFO_OS_GET_TIME();

	k_sem_give(&sem_bench);

	TIMING_INFO_PRE_READ();
	u32_t sem_give_wo_cxt_end = TIMING_INFO_OS_GET_TIME();
	u32_t sem_give_wo_cxt_cycles = sem_give_wo_cxt_end -
					  sem_give_wo_cxt_start;

	TIMING_INFO_PRE_READ();
	u32_t sem_take_wo_cxt_start = TIMING_INFO_OS_GET_TIME();

	k_sem_take(&sem_bench, 10);
	TIMING_INFO_PRE_READ();
	u32_t sem_take_wo_cxt_end = TIMING_INFO_OS_GET_TIME();
	u32_t sem_take_wo_cxt_cycles = sem_take_wo_cxt_end -
					  sem_take_wo_cxt_start;

	/* TC_PRINT("test_time1 , %d cycles\n", (u32_t)test_time1); */
	/* TC_PRINT("test_time2 , %d cycles\n", (u32_t)test_time2); */

	PRINT_STATS("Semaphore Take with context switch",
		sem_cycles, CYCLES_TO_NS(sem_cycles));
	PRINT_STATS("Semaphore Give with context switch",
		sem_give_cycles, CYCLES_TO_NS(sem_give_cycles));

	PRINT_STATS("Semaphore Take without context switch",
		sem_take_wo_cxt_cycles,
		CYCLES_TO_NS(sem_take_wo_cxt_cycles));
	PRINT_STATS("Semaphore Give without context switch",
		sem_give_wo_cxt_cycles,
		CYCLES_TO_NS(sem_give_wo_cxt_cycles));

}
/******************************************************************************/
K_MUTEX_DEFINE(mutex0);
void mutex_bench(void)
{
	u32_t mutex_lock_start_time;
	u32_t mutex_lock_end_time;
	u32_t mutex_lock_diff = 0;

	u32_t mutex_unlock_start_time;
	u32_t mutex_unlock_end_time;
	u32_t mutex_unlock_diff = 0;
	u32_t count = 0;

	for (int i = 0; i < 1000; i++) {
		s64_t before = k_uptime_get();

		TIMING_INFO_PRE_READ();
		mutex_lock_start_time = TIMING_INFO_OS_GET_TIME();

		k_mutex_lock(&mutex0, 100);

		TIMING_INFO_PRE_READ();
		mutex_lock_end_time = TIMING_INFO_OS_GET_TIME();

		TIMING_INFO_PRE_READ();
		mutex_unlock_start_time = TIMING_INFO_OS_GET_TIME();

		k_mutex_unlock(&mutex0);

		TIMING_INFO_PRE_READ();
		mutex_unlock_end_time = TIMING_INFO_OS_GET_TIME();

		/* If timer interrupt occurs we need to omit that sample*/
		s64_t after = k_uptime_get();

		if (after - before)
			continue;
		count++;

		mutex_lock_diff += (mutex_lock_end_time -
					mutex_lock_start_time);
		mutex_unlock_diff += (mutex_unlock_end_time -
				      mutex_unlock_start_time);
	}

	PRINT_STATS("Mutex lock", mutex_lock_diff / count,
		CYCLES_TO_NS(mutex_lock_diff / count));

	PRINT_STATS("Mutex unlock", mutex_unlock_diff / count,
		CYCLES_TO_NS(mutex_unlock_diff / count));
}

/******************************************************************************/
void thread_sem1_test(void *p1, void *p2, void *p3)
{

	k_sem_give(&sem_bench); /* sync the 2 threads*/

	__read_swap_end_time_value = 1;
	TIMING_INFO_PRE_READ();
	sem_start_time =  TIMING_INFO_OS_GET_TIME();
	k_sem_take(&sem_bench, 10);
}

u32_t sem_count;
void thread_sem0_test(void *p1, void *p2, void *p3)
{
	k_sem_take(&sem_bench, 10);/* To sync threads */

	k_sem_give(&sem_bench);
	sem_count++;
	k_thread_abort(sem0_tid);
}
/******************************************************************************/
void thread_sem1_give_test(void *p1, void *p2, void *p3)
{
	k_sem_give(&sem_bench);         /* sync the 2 threads*/

	k_sem_take(&sem_bench_1, 1000); /* clear the previous sem_give*/
}

void thread_sem0_give_test(void *p1, void *p2, void *p3)
{
	k_sem_take(&sem_bench, 10);/* To sync threads */

	/* To make sure that the sem give will cause a swap to occur */
	k_thread_priority_set(sem1_tid, 1);

	__read_swap_end_time_value = 1;
	TIMING_INFO_PRE_READ();
	sem_give_start_time =  TIMING_INFO_OS_GET_TIME();
	k_sem_give(&sem_bench_1);

}
