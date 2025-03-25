/*
 * Copyright (c) 2025 Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "coremark.h"

#if CONFIG_COREMARK_RUN_TYPE_VALIDATION
volatile ee_s32 seed1_volatile = 0x3415;
volatile ee_s32 seed2_volatile = 0x3415;
volatile ee_s32 seed3_volatile = 0x66;
#endif

#if CONFIG_COREMARK_RUN_TYPE_PERFORMANCE
volatile ee_s32 seed1_volatile = 0x0;
volatile ee_s32 seed2_volatile = 0x0;
volatile ee_s32 seed3_volatile = 0x66;
#endif

#if CONFIG_COREMARK_RUN_TYPE_PROFILE
volatile ee_s32 seed1_volatile = 0x8;
volatile ee_s32 seed2_volatile = 0x8;
volatile ee_s32 seed3_volatile = 0x8;
#endif

volatile ee_s32 seed4_volatile = CONFIG_COREMARK_ITERATIONS;
volatile ee_s32 seed5_volatile = 0;

ee_u32 default_num_contexts = CONFIG_COREMARK_THREADS_NUMBER;

static CORE_TICKS start_time_val;
static CORE_TICKS stop_time_val;

void start_time(void)
{
	start_time_val = k_cycle_get_32();
}

void stop_time(void)
{
	stop_time_val = k_cycle_get_32();
}

CORE_TICKS get_time(void)
{
	return (stop_time_val - start_time_val);
}

secs_ret time_in_secs(CORE_TICKS ticks)
{
	secs_ret retval = (secs_ret)(k_cyc_to_ns_floor64(ticks) / 1000) / 1000000UL;
	return retval;
}

void portable_init(core_portable *p, int *argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (sizeof(ee_ptr_int) != sizeof(ee_u8 *)) {
		ee_printf(
			"ERROR! Please define ee_ptr_int to a type that holds a "
			"pointer!\n");
	}

	if (sizeof(ee_u32) != 4) {
		ee_printf("ERROR! Please define ee_u32 to a 32b unsigned type!\n");
	}

	p->portable_id = 1;
}

void portable_fini(core_portable *p)
{
	p->portable_id = 0;
}

#if (CONFIG_COREMARK_THREADS_NUMBER > 1)

#define THREAD_STACK_SIZE   (512)
static K_THREAD_STACK_DEFINE(coremark_thread1_stack, THREAD_STACK_SIZE);
static K_THREAD_STACK_DEFINE(coremark_thread2_stack, THREAD_STACK_SIZE);
static K_THREAD_STACK_DEFINE(coremark_thread3_stack, THREAD_STACK_SIZE);
static K_THREAD_STACK_DEFINE(coremark_thread4_stack, THREAD_STACK_SIZE);

static k_thread_stack_t *pstack[] = {
	coremark_thread1_stack,
	coremark_thread2_stack,
	coremark_thread3_stack,
	coremark_thread4_stack
};

static struct k_thread coremark_thread_data[CONFIG_COREMARK_THREADS_NUMBER];

static int active_threads;

static void coremark_thread(void *id, void *pres, void *p3)
{
	iterate(pres);
}

ee_u8 core_start_parallel(core_results *res)
{
	k_thread_create(&coremark_thread_data[active_threads],
					pstack[active_threads],
					THREAD_STACK_SIZE,
					coremark_thread,
					(void *)active_threads,
					res,
					NULL,
					0, 0, K_NO_WAIT);

	/* Thread creation is done from single thread.
	 * CoreMark guarantees number of active thread is limited to defined bound.
	 */
	active_threads++;

	return 0;
}

ee_u8 core_stop_parallel(core_results *res)
{
	active_threads--;

	/* Order in which threads are joined does not matter.
	 * CoreMark will never start threads unless all previously started are joined.
	 * Stack is guaranteed to be used by one thread only.
	 */

	k_thread_join(&coremark_thread_data[active_threads], K_FOREVER);

	return 0;
}

#endif
