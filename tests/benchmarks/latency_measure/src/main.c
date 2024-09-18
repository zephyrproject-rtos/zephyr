/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2023,2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * This file contains the main testing module that invokes all the tests.
 */

#include <zephyr/kernel.h>
#include <zephyr/timestamp.h>
#include "utils.h"
#include "timing_sc.h"
#include <zephyr/tc_util.h>

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

uint32_t tm_off;

BENCH_BMEM struct timestamp_data  timestamp;

#ifdef CONFIG_USERSPACE
K_APPMEM_PARTITION_DEFINE(bench_mem_partition);
#endif

K_THREAD_STACK_DEFINE(start_stack, START_STACK_SIZE);
K_THREAD_STACK_DEFINE(alt_stack, ALT_STACK_SIZE);

K_SEM_DEFINE(pause_sem, 0, 1);

#if (CONFIG_MP_MAX_NUM_CPUS > 1)
struct k_thread  busy_thread[CONFIG_MP_MAX_NUM_CPUS - 1];

#define BUSY_THREAD_STACK_SIZE  (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

K_THREAD_STACK_DEFINE(busy_thread_stack, BUSY_THREAD_STACK_SIZE);
#endif

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
extern int fifo_ops(uint32_t num_iterations, uint32_t options);
extern int fifo_blocking_ops(uint32_t num_iterations, uint32_t start_options,
			     uint32_t alt_options);
extern int lifo_ops(uint32_t num_iterations, uint32_t options);
extern int lifo_blocking_ops(uint32_t num_iterations, uint32_t start_options,
			     uint32_t alt_options);
extern int event_ops(uint32_t num_iterations, uint32_t options);
extern int event_blocking_ops(uint32_t num_iterations, uint32_t start_options,
			      uint32_t alt_options);
extern int condvar_blocking_ops(uint32_t num_iterations, uint32_t start_options,
				uint32_t alt_options);
extern int stack_ops(uint32_t num_iterations, uint32_t options);
extern int stack_blocking_ops(uint32_t num_iterations, uint32_t start_options,
			       uint32_t alt_options);
extern void heap_malloc_free(void);

#if (CONFIG_MP_MAX_NUM_CPUS > 1)
static void busy_thread_entry(void *arg1, void *arg2, void *arg3)
{
	while (1) {
	}
}
#endif

static void test_thread(void *arg1, void *arg2, void *arg3)
{
	uint32_t freq;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

#if (CONFIG_MP_MAX_NUM_CPUS > 1)
	/* Spawn busy threads that will execute on the other cores */

	for (uint32_t i = 0; i < CONFIG_MP_MAX_NUM_CPUS - 1; i++) {
		k_thread_create(&busy_thread[i], &busy_thread_stack[i],
				BUSY_THREAD_STACK_SIZE, busy_thread_entry,
				NULL, NULL, NULL,
				K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);
	}
#endif

#ifdef CONFIG_USERSPACE
	k_mem_domain_add_partition(&k_mem_domain_default,
				   &bench_mem_partition);
#endif

	timing_init();

	bench_test_init();

	freq = timing_freq_get_mhz();

	TC_START("Time Measurement");
	TC_PRINT("Timing results: Clock frequency: %u MHz\n", freq);

	timestamp_overhead_init(CONFIG_BENCHMARK_NUM_ITERATIONS);

	/* Preemptive threads context switching */
	thread_switch_yield(CONFIG_BENCHMARK_NUM_ITERATIONS, false);

	/* Cooperative threads context switching */
	thread_switch_yield(CONFIG_BENCHMARK_NUM_ITERATIONS, true);

	int_to_thread(CONFIG_BENCHMARK_NUM_ITERATIONS);

	/* Thread creation, starting, suspending, resuming and aborting. */

	thread_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, 0);
#ifdef CONFIG_USERSPACE
	thread_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, K_USER);
	thread_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, K_USER);
	thread_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, 0);
#endif

	fifo_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0);
#ifdef CONFIG_USERSPACE
	fifo_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER);
#endif

	fifo_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, 0);
#ifdef CONFIG_USERSPACE
	fifo_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, K_USER);
	fifo_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, 0);
	fifo_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, K_USER);
#endif


	lifo_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0);
#ifdef CONFIG_USERSPACE
	lifo_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER);
#endif

	lifo_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, 0);
#ifdef CONFIG_USERSPACE
	lifo_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, K_USER);
	lifo_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, 0);
	lifo_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, K_USER);
#endif

	event_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0);
#ifdef CONFIG_USERSPACE
	event_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER);
#endif

	event_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, 0);
#ifdef CONFIG_USERSPACE
	event_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, K_USER);
	event_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, 0);
	event_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, K_USER);
#endif

	sema_test_signal(CONFIG_BENCHMARK_NUM_ITERATIONS, 0);
#ifdef CONFIG_USERSPACE
	sema_test_signal(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER);
#endif

	sema_context_switch(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, 0);
#ifdef CONFIG_USERSPACE
	sema_context_switch(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, K_USER);
	sema_context_switch(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, 0);
	sema_context_switch(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, K_USER);
#endif

	condvar_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, 0);
#ifdef CONFIG_USERSPACE
	condvar_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, K_USER);
	condvar_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, 0);
	condvar_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, K_USER);
#endif

	stack_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0);
#ifdef CONFIG_USERSPACE
	stack_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER);
#endif

	stack_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, 0);
#ifdef CONFIG_USERSPACE
	stack_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, 0, K_USER);
	stack_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, 0);
	stack_blocking_ops(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER, K_USER);
#endif

	mutex_lock_unlock(CONFIG_BENCHMARK_NUM_ITERATIONS, 0);
#ifdef CONFIG_USERSPACE
	mutex_lock_unlock(CONFIG_BENCHMARK_NUM_ITERATIONS, K_USER);
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
