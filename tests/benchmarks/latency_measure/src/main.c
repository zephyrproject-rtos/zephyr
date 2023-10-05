/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * This file contains the main testing module that invokes all the tests.
 */

#include <zephyr/kernel.h>
#include <zephyr/timestamp.h>
#include <zephyr/app_memory/app_memdomain.h>
#include "utils.h"
#include "timing_sc.h"
#include <zephyr/tc_util.h>

#define NUM_ITERATIONS  10000

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

uint32_t tm_off;

BENCH_BMEM struct timestamp_data  timestamp;

#ifdef CONFIG_USERSPACE
K_APPMEM_PARTITION_DEFINE(bench_mem_partition);
#endif

K_THREAD_STACK_DEFINE(start_stack, START_STACK_SIZE);
K_THREAD_STACK_DEFINE(alt_stack, START_STACK_SIZE);

K_SEM_DEFINE(pause_sem, 0, 1);

struct k_thread  start_thread;
struct k_thread  alt_thread;

int error_count; /* track number of errors */

extern void thread_switch_yield(uint32_t num_iterations, bool is_cooperative);
extern void int_to_thread(uint32_t num_iterations);
extern void sema_test_signal(uint32_t num_iterations, uint32_t options);
extern void mutex_lock_unlock(uint32_t num_iterations, uint32_t options);
extern void sema_context_switch(uint32_t num_iterations,
				uint32_t start_options, uint32_t alt_options);
extern int thread_ops(uint32_t num_iterations, uint32_t start_options,
		      uint32_t alt_options);
extern void heap_malloc_free(void);

static void test_thread(void *arg1, void *arg2, void *arg3)
{
	uint32_t freq;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

#ifdef CONFIG_USERSPACE
	k_mem_domain_add_partition(&k_mem_domain_default,
				   &bench_mem_partition);
#endif

	timing_init();

	bench_test_init();

	freq = timing_freq_get_mhz();

	TC_START("Time Measurement");
	TC_PRINT("Timing results: Clock frequency: %u MHz\n", freq);

	timestamp_overhead_init(NUM_ITERATIONS);

	/* Preemptive threads context switching */
	thread_switch_yield(NUM_ITERATIONS, false);

	/* Cooperative threads context switching */
	thread_switch_yield(NUM_ITERATIONS, true);

	int_to_thread(NUM_ITERATIONS);

	/* Thread creation, starting, suspending, resuming and aborting. */

	thread_ops(NUM_ITERATIONS, 0, 0);
#ifdef CONFIG_USERSPACE
	thread_ops(NUM_ITERATIONS, 0, K_USER);
	thread_ops(NUM_ITERATIONS, K_USER, K_USER);
	thread_ops(NUM_ITERATIONS, K_USER, 0);
#endif

	sema_test_signal(NUM_ITERATIONS, 0);
#ifdef CONFIG_USERSPACE
	sema_test_signal(NUM_ITERATIONS, K_USER);
#endif

	sema_context_switch(NUM_ITERATIONS, 0, 0);
#ifdef CONFIG_USERSPACE
	sema_context_switch(NUM_ITERATIONS, 0, K_USER);
	sema_context_switch(NUM_ITERATIONS, K_USER, 0);
	sema_context_switch(NUM_ITERATIONS, K_USER, K_USER);
#endif

	mutex_lock_unlock(NUM_ITERATIONS, 0);
#ifdef CONFIG_USERSPACE
	mutex_lock_unlock(NUM_ITERATIONS, K_USER);
#endif

	heap_malloc_free();

	TC_END_REPORT(error_count);
}

K_THREAD_DEFINE(test_thread_id, STACK_SIZE, test_thread, NULL, NULL, NULL,
		K_PRIO_PREEMPT(10), 0, 0);

int main(void)
{
	k_thread_join(test_thread_id, K_FOREVER);
	return 0;
}
