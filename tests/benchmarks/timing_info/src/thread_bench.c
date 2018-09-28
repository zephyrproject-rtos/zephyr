/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Measure time
 *
 */
#include <kernel.h>
#include <zephyr.h>
#include <tc_util.h>
#include <ksched.h>
#include "timing_info.h"
char sline[256];
/* FILE *output_file = stdout; */

/* location of the time stamps*/
extern u32_t __read_swap_end_time_value;
extern u64_t __common_var_swap_end_time;

volatile u64_t thread_abort_end_time;
volatile u64_t thread_abort_start_time;

/* Thread suspend*/
volatile u64_t thread_suspend_start_time;
volatile u64_t thread_suspend_end_time;

/* Thread resume*/
volatile u64_t thread_resume_start_time;
volatile u64_t thread_resume_end_time;

/* Thread sleep*/
volatile u64_t thread_sleep_start_time;
volatile u64_t thread_sleep_end_time;

/*For benchmarking msg queues*/
k_tid_t producer_tid;
k_tid_t consumer_tid;

/* To time thread creation*/
K_THREAD_STACK_DEFINE(my_stack_area, STACK_SIZE);
K_THREAD_STACK_DEFINE(my_stack_area_0, STACK_SIZE);
struct k_thread my_thread;
struct k_thread my_thread_0;

u32_t __read_swap_end_time_value_test = 1;
u64_t dummy_time;
u64_t start_time;
u64_t test_end_time;

/* Disable the overhead calculations, this is needed to calculate
 * the overhead created by the benchmarking code itself.
 */
#define DISABLE_OVERHEAD_MEASUREMENT

#if defined(CONFIG_X86) && !defined(DISABLE_OVERHEAD_MEASUREMENT)
u32_t benchmarking_overhead_swap(void)
{

	__asm__ __volatile__ (
		"pushl %eax\n\t"
		"pushl %edx\n\t"
		"rdtsc\n\t"
		"mov %eax,start_time\n\t"
		"mov %edx,start_time+4\n\t"
		"cmp $0x1,__read_swap_end_time_value_test\n\t"
		"jne time_read_not_needed_test\n\t"
		"movw $0x2,__read_swap_end_time_value\n\t"
		"pushl %eax\n\t"
		"pushl %edx\n\t"
		"rdtsc\n\t"
		"mov %eax,dummy_time\n\t"
		"mov %edx,dummy_time+4\n\t"
		"pop %edx\n\t"
		"pop %eax\n\t"
		"time_read_not_needed_test:\n\t"
		"rdtsc\n\t"
		"mov %eax,test_end_time\n\t"
		"mov %edx,test_end_time+4\n\t"
		"pop %edx\n\t"
		"pop %eax\n\t");

	return(test_end_time - start_time);
}
#endif

void test_thread_entry(void *p, void *p1, void *p2)
{
	static int i;

	i++;
}


void thread_swap_test(void *p1, void *p2, void *p3)
{
	__read_swap_end_time_value = 1;
	TIMING_INFO_PRE_READ();
	thread_abort_start_time = TIMING_INFO_OS_GET_TIME();
	k_thread_abort(_current);
}

void thread_suspend_test(void *p1, void *p2, void *p3);
void yield_bench(void);
void heap_malloc_free_bench(void);
void main_sem_bench(void);
void main_mutex_bench(void);
void main_msg_bench(void);

void system_thread_bench(void)
{
	/*Thread create*/
	u64_t thread_create_start_time;
	u64_t thread_create_end_time;

	DECLARE_VAR(thread, create)

	/*Thread cancel*/
	u64_t thread_cancel_start_time;
	u64_t thread_cancel_end_time;
	DECLARE_VAR(thread, cancel)

	/* Thread Abort*/
	DECLARE_VAR(thread, abort)

	/* Thread Suspend*/
	DECLARE_VAR(thread, suspend)

	/* Thread Resume*/
	DECLARE_VAR(thread, resume)


	/* to measure context switch time */
	k_thread_create(&my_thread_0, my_stack_area_0, STACK_SIZE,
			thread_swap_test,
			NULL, NULL, NULL,
			-1 /*priority*/, 0, 0);

	k_sleep(1);
	thread_abort_end_time = (__common_var_swap_end_time);
	__end_swap_time = __common_var_swap_end_time;

	u32_t total_swap_cycles = __end_swap_time - __start_swap_time;

	/* Interrupt latency*/
	u64_t local_end_intr_time = __end_intr_time;
	u64_t local_start_intr_time = __start_intr_time;

	/*******************************************************************/
	/* thread create*/
	TIMING_INFO_PRE_READ();
	thread_create_start_time = TIMING_INFO_OS_GET_TIME();

	k_tid_t my_tid = k_thread_create(&my_thread, my_stack_area,
					 STACK_SIZE,
					 thread_swap_test,
					 NULL, NULL, NULL,
					 5 /*priority*/, 0, 10);
	TIMING_INFO_PRE_READ();
	thread_create_end_time = TIMING_INFO_OS_GET_TIME();

	/* thread Termination*/
	TIMING_INFO_PRE_READ();
	thread_cancel_start_time = TIMING_INFO_OS_GET_TIME();
	k_thread_abort(my_tid);

	TIMING_INFO_PRE_READ();
	thread_cancel_end_time = TIMING_INFO_OS_GET_TIME();

	/* Thread suspend*/
	k_tid_t sus_res_tid = k_thread_create(&my_thread, my_stack_area,
					      STACK_SIZE,
					      thread_suspend_test,
					      NULL, NULL, NULL,
					      -1 /*priority*/, 0, 0);

	TIMING_INFO_PRE_READ();
	thread_suspend_end_time = TIMING_INFO_OS_GET_TIME();
	/* At this point test for resume*/
	k_thread_resume(sus_res_tid);

	/* calculation for creation */
	CALCULATE_TIME(, thread, create)

	/* calculation for cancel */
	CALCULATE_TIME(, thread, cancel)

	/* calculation for abort */
	CALCULATE_TIME(, thread, abort)

	/* calculation for suspend */
	CALCULATE_TIME(, thread, suspend)

	/* calculation for resume */
	thread_resume_start_time = thread_suspend_end_time;
	CALCULATE_TIME(, thread, resume)

	/*******************************************************************/

	/* Only print lower 32bit of time result */
	PRINT_STATS("Context switch",
		(u32_t)(total_swap_cycles & 0xFFFFFFFFULL),
		(u32_t) CYCLES_TO_NS(total_swap_cycles));

	/*TC_PRINT("Swap Overhead:%d cycles\n", benchmarking_overhead_swap());*/

	/*Interrupt latency */
	u32_t intr_latency_cycles = SUBTRACT_CLOCK_CYCLES(local_end_intr_time) -
		SUBTRACT_CLOCK_CYCLES(local_start_intr_time);

	PRINT_STATS("Interrupt latency",
		(u32_t)(intr_latency_cycles),
		(u32_t) (CYCLES_TO_NS(intr_latency_cycles)));

	/*tick overhead*/
	u32_t tick_overhead_cycles =  SUBTRACT_CLOCK_CYCLES(__end_tick_time) -
				SUBTRACT_CLOCK_CYCLES(__start_tick_time);
	PRINT_STATS("Tick overhead",
		(u32_t)(tick_overhead_cycles),
		(u32_t) (CYCLES_TO_NS(tick_overhead_cycles)));

	/*thread creation*/
	PRINT_STATS("Thread Creation",
		(u32_t)((thread_create_end_time - thread_create_start_time) &
			   0xFFFFFFFFULL),
		(u32_t) ((total_thread_create_time) & 0xFFFFFFFFULL));

	/*thread cancel*/
	PRINT_STATS("Thread cancel",
		(u32_t)((thread_cancel_end_time - thread_cancel_start_time) &
			   0xFFFFFFFFULL),
		(u32_t) (total_thread_cancel_time  & 0xFFFFFFFFULL));

	/*thread abort*/
	PRINT_STATS("Thread abort",
		(u32_t)((thread_abort_end_time - thread_abort_start_time) &
			   0xFFFFFFFFULL),
		(u32_t) (total_thread_abort_time  & 0xFFFFFFFFULL));

	/*thread suspend*/
	PRINT_STATS("Thread Suspend",
		(u32_t)((thread_suspend_end_time - thread_suspend_start_time) &
			   0xFFFFFFFFULL),
		(u32_t) (total_thread_suspend_time  & 0xFFFFFFFFULL));

	/*thread resume*/
	PRINT_STATS("Thread Resume",
		(u32_t)((thread_resume_end_time - thread_suspend_end_time)
			   & 0xFFFFFFFFULL),
		(u32_t) (total_thread_resume_time  & 0xFFFFFFFFULL));


}

void thread_suspend_test(void *p1, void *p2, void *p3)
{
	TIMING_INFO_PRE_READ();
	thread_suspend_start_time = TIMING_INFO_OS_GET_TIME();
	k_thread_suspend(_current);

	/* comes to this line once its resumed*/
	TIMING_INFO_PRE_READ();
	thread_resume_end_time = TIMING_INFO_OS_GET_TIME();

	/* k_thread_suspend(_current); */
}

void heap_malloc_free_bench(void)
{
	/* heap malloc*/
	u64_t heap_malloc_start_time = 0;
	u64_t heap_malloc_end_time = 0;

	/* heap free*/
	u64_t heap_free_start_time = 0;
	u64_t heap_free_end_time = 0;

	s32_t count = 0;
	u32_t sum_malloc = 0;
	u32_t sum_free = 0;

	while (count++ != 100) {
		TIMING_INFO_PRE_READ();
		heap_malloc_start_time = TIMING_INFO_OS_GET_TIME();
		void *allocated_mem = k_malloc(10);

		TIMING_INFO_PRE_READ();
		heap_malloc_end_time = TIMING_INFO_OS_GET_TIME();
		if (allocated_mem == NULL) {
			TC_PRINT("\n Malloc failed at count %d\n", count);
			break;
		}
		TIMING_INFO_PRE_READ();
		heap_free_start_time = TIMING_INFO_OS_GET_TIME();

		k_free(allocated_mem);

		TIMING_INFO_PRE_READ();
		heap_free_end_time = TIMING_INFO_OS_GET_TIME();
		sum_malloc += heap_malloc_end_time - heap_malloc_start_time;
		sum_free += heap_free_end_time - heap_free_start_time;
	}

	PRINT_STATS("Heap Malloc",
		(u32_t)((sum_malloc / count) & 0xFFFFFFFFULL),
		(u32_t)(CYCLES_TO_NS(sum_malloc / count)));
	PRINT_STATS("Heap Free",
		(u32_t)((sum_free / count) & 0xFFFFFFFFULL),
		(u32_t)(CYCLES_TO_NS(sum_free / count)));

}
