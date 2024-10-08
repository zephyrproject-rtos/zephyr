/*
 * Copyright (c) 2024 Intel Corporation
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
#include <zephyr/tc_util.h>
#include <ksched.h>

#define TEST_STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define BUSY_STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

uint32_t tm_off;

/*
 * Warning! Most of the created threads in this test use the same stack!
 * This is done to reduce the memory footprint as having unique stacks
 * for hundreds or thousands of threads would require substantial memory.
 * We can get away with this approach as the threads sharing the same
 * stack will not be executing, even though they will be ready to run.
 */

static K_THREAD_STACK_DEFINE(test_stack, TEST_STACK_SIZE);

K_THREAD_STACK_ARRAY_DEFINE(busy_stack, CONFIG_MP_MAX_NUM_CPUS - 1, BUSY_STACK_SIZE);
static struct k_thread busy_thread[CONFIG_MP_MAX_NUM_CPUS - 1];

static struct k_thread test_thread[CONFIG_BENCHMARK_NUM_THREADS];

static uint64_t add_cycles[CONFIG_BENCHMARK_NUM_THREADS];
static uint64_t remove_cycles[CONFIG_BENCHMARK_NUM_THREADS];

extern void z_unready_thread(struct k_thread *thread);

static void busy_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
	}
}

/**
 * The test entry routine is not expected to execute.
 */
static void test_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("Thread %u unexpectedly executed\n",
	       (unsigned int)(uintptr_t)p1);

	while (1) {
	}
}

static void start_threads(unsigned int num_threads)
{
	unsigned int i;
	unsigned int bucket_size;

	/* Start the busy threads to execute on the other processors */

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS - 1; i++) {
		k_thread_create(&busy_thread[i], busy_stack[i], BUSY_STACK_SIZE,
				busy_entry, NULL, NULL, NULL,
				-1, 0, K_NO_WAIT);
	}

	bucket_size = (num_threads / CONFIG_NUM_PREEMPT_PRIORITIES) + 1;

	for (i = 0; i < CONFIG_BENCHMARK_NUM_THREADS; i++) {
		k_thread_create(&test_thread[i], test_stack, TEST_STACK_SIZE,
				test_entry, (void *)(uintptr_t)i, NULL, NULL,
				i / bucket_size, 0, K_NO_WAIT);
	}
}

static void cycles_reset(unsigned int num_threads)
{
	unsigned int i;

	for (i = 0; i < num_threads; i++) {
		add_cycles[i] = 0ULL;
		remove_cycles[i] = 0ULL;
	}
}

static void test_decreasing_priority(unsigned int num_threads)
{
	unsigned int i;
	timing_t start;
	timing_t finish;

	for (i = num_threads; i > 0; i--) {
		start = timing_counter_get();
		z_unready_thread(&test_thread[i - 1]);
		finish = timing_counter_get();
		remove_cycles[i - 1] += timing_cycles_get(&start, &finish);
	}

	for (i = 0; i < num_threads; i++) {
		start = timing_counter_get();
		z_ready_thread(&test_thread[i]);
		finish = timing_counter_get();
		add_cycles[i] += timing_cycles_get(&start, &finish);
	}
}

static void test_increasing_priority(unsigned int num_threads)
{
	unsigned int i;
	timing_t start;
	timing_t finish;

	for (i = num_threads; i > 0; i--) {
		start = timing_counter_get();
		z_unready_thread(&test_thread[num_threads - i]);
		finish = timing_counter_get();
		remove_cycles[i - 1] += timing_cycles_get(&start, &finish);
	}

	for (i = num_threads; i > 0; i--) {
		start = timing_counter_get();
		z_ready_thread(&test_thread[i - 1]);
		finish = timing_counter_get();
		add_cycles[num_threads - i] += timing_cycles_get(&start, &finish);
	}
}

static uint64_t sqrt_u64(uint64_t square)
{
	if (square > 1) {
		uint64_t lo = sqrt_u64(square >> 2) << 1;
		uint64_t hi = lo + 1;

		return ((hi * hi) > square) ? lo : hi;
	}

	return square;
}

static void compute_and_report_stats(unsigned int num_threads,
				     unsigned int num_iterations,
				     uint64_t *cycles,
				     const char *str)
{
	uint64_t minimum = cycles[0];
	uint64_t maximum = cycles[0];
	uint64_t total = cycles[0];
	uint64_t average;
	uint64_t std_dev = 0;
	uint64_t tmp;
	uint64_t diff;
	unsigned int i;

	for (i = 1; i < num_threads; i++) {
		if (cycles[i] > maximum) {
			maximum = cycles[i];
		}

		if (cycles[i] < minimum) {
			minimum = cycles[i];
		}

		total += cycles[i];
	}

	minimum /= (uint64_t)num_iterations;
	maximum /= (uint64_t)num_iterations;
	average = total / (num_threads * num_iterations);

	for (i = 0; i < num_threads; i++) {
		tmp = cycles[i] / num_iterations;
		diff = (average > tmp) ? (average - tmp) : (tmp - average);

		std_dev += (diff * diff);
	}
	std_dev /= num_threads;
	std_dev = sqrt_u64(std_dev);

	printk("%s\n", str);

	printk("    Minimum : %7llu cycles (%7u nsec)\n",
	       minimum, (uint32_t)timing_cycles_to_ns(minimum));
	printk("    Maximum : %7llu cycles (%7u nsec)\n",
	       maximum, (uint32_t)timing_cycles_to_ns(maximum));
	printk("    Average : %7llu cycles (%7u nsec)\n",
	       average, (uint32_t)timing_cycles_to_ns(average));
	printk("    Std Deviation: %7llu cycles (%7u nsec)\n",
	       std_dev, (uint32_t)timing_cycles_to_ns(std_dev));
}

int main(void)
{
	unsigned int i;
	unsigned int freq;
#ifdef CONFIG_BENCHMARK_VERBOSE
	char description[120];
	char tag[50];
	struct k_thread *thread;
#endif

	timing_init();

	bench_test_init();

	freq = timing_freq_get_mhz();

	printk("Time Measurements for %s sched queues\n",
	       IS_ENABLED(CONFIG_SCHED_DUMB) ? "dumb" :
	       IS_ENABLED(CONFIG_SCHED_SCALABLE) ? "scalable" : "multiq");
	printk("Timing results: Clock frequency: %u MHz\n", freq);

	start_threads(CONFIG_BENCHMARK_NUM_THREADS);

	timing_start();

	cycles_reset(CONFIG_BENCHMARK_NUM_THREADS);

	for (i = 0; i < CONFIG_BENCHMARK_NUM_ITERATIONS; i++) {
		test_decreasing_priority(CONFIG_BENCHMARK_NUM_THREADS);
	}

	compute_and_report_stats(CONFIG_BENCHMARK_NUM_THREADS,
				 CONFIG_BENCHMARK_NUM_ITERATIONS,
				 add_cycles,
				 "Add threads of decreasing priority");

#ifdef CONFIG_BENCHMARK_VERBOSE
	for (i = 0; i < CONFIG_BENCHMARK_NUM_THREADS; i++) {
		snprintf(tag, sizeof(tag),
			 "ReadyQ.add.to.tail.%04u.waiters", i);
		snprintf(description, sizeof(description),
			 "%-40s - Add thread of priority (%u)",
			 tag, test_thread[i].base.prio);
		PRINT_STATS_AVG(description, (uint32_t)add_cycles[i],
				CONFIG_BENCHMARK_NUM_ITERATIONS);
	}
#endif

	printk("------------------------------------\n");

	compute_and_report_stats(CONFIG_BENCHMARK_NUM_THREADS,
				 CONFIG_BENCHMARK_NUM_ITERATIONS,
				 remove_cycles,
				 "Remove threads of decreasing priority");

#ifdef CONFIG_BENCHMARK_VERBOSE
	for (i = 0; i < CONFIG_BENCHMARK_NUM_THREADS; i++) {
		snprintf(tag, sizeof(tag),
			 "ReadyQ.remove.from.head.%04u.waiters", i);
		snprintf(description, sizeof(description),
			 "%-40s - Remove thread of priority %u",
			 tag, test_thread[i].base.prio);
		PRINT_STATS_AVG(description, (uint32_t)remove_cycles[i],
				CONFIG_BENCHMARK_NUM_ITERATIONS);
	}
#endif

	printk("------------------------------------\n");

	cycles_reset(CONFIG_BENCHMARK_NUM_THREADS);

	for (i = 0; i < CONFIG_BENCHMARK_NUM_ITERATIONS; i++) {
		test_increasing_priority(CONFIG_BENCHMARK_NUM_THREADS);
	}

	compute_and_report_stats(CONFIG_BENCHMARK_NUM_THREADS,
				 CONFIG_BENCHMARK_NUM_ITERATIONS,
				 add_cycles,
				 "Add threads of increasing priority");

#ifdef CONFIG_BENCHMARK_VERBOSE
	for (i = 0; i < CONFIG_BENCHMARK_NUM_THREADS; i++) {
		snprintf(tag, sizeof(tag),
			 "ReadyQ.add.to.head.%04u.waiters", i);
		thread = &test_thread[CONFIG_BENCHMARK_NUM_THREADS - i - 1];
		snprintf(description, sizeof(description),
			 "%-40s - Add priority %u to readyq",
			 tag, thread->base.prio);
		PRINT_STATS_AVG(description, (uint32_t)add_cycles[i],
				CONFIG_BENCHMARK_NUM_ITERATIONS);
	}
#endif

	printk("------------------------------------\n");

	compute_and_report_stats(CONFIG_BENCHMARK_NUM_THREADS,
				 CONFIG_BENCHMARK_NUM_ITERATIONS,
				 remove_cycles,
				 "Remove threads or increasing priority");

#ifdef CONFIG_BENCHMARK_VERBOSE
	for (i = 0; i < CONFIG_BENCHMARK_NUM_THREADS; i++) {
		snprintf(tag, sizeof(tag),
			"ReadyQ.remove.from.tail.%04u.waiters",
			CONFIG_BENCHMARK_NUM_THREADS - i);
		thread = &test_thread[CONFIG_BENCHMARK_NUM_THREADS - i - 1];
		snprintf(description, sizeof(description),
			 "%-40s - Remove lowest priority from readyq (%u)",
			 tag, thread->base.prio);
		PRINT_STATS_AVG(description, (uint32_t)remove_cycles[i],
				CONFIG_BENCHMARK_NUM_ITERATIONS);
	}
#endif

	for (i = 0; i < CONFIG_BENCHMARK_NUM_THREADS; i++) {
		k_thread_abort(&test_thread[i]);
	}

	timing_stop();

	TC_END_REPORT(0);

	return 0;
}
