/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * This file contains tests that will measure the length of time required
 * to add and remove threads from a wait queue that holds a varying number of
 * threads. Each thread added to (and removed from) the wait queue is a dummy
 * thread. As these dummy threads are inherently non-executable, this helps
 * prevent the addition/removal of threads to/from the ready queue from being
 * included in these measurements. Furthermore, the use of dummy threads helps
 * reduce the memory footprint as not only are thread stacks not required,
 * but we also do not need the full k_thread structure for each of these
 * dummy threads.
 */

#include <zephyr/kernel.h>
#include <zephyr/timestamp.h>
#include <zephyr/timing/timing.h>
#include "utils.h"
#include <zephyr/tc_util.h>
#include <wait_q.h>
#include <ksched.h>
#include <stdio.h>

uint32_t tm_off;

static struct _thread_base dummy_thread[CONFIG_BENCHMARK_NUM_THREADS];
static _wait_q_t wait_q;

uint64_t add_cycles[CONFIG_BENCHMARK_NUM_THREADS];
uint64_t remove_cycles[CONFIG_BENCHMARK_NUM_THREADS];

/**
 * Initialize each dummy thread.
 */
static void dummy_threads_init(unsigned int num_threads)
{
	unsigned int i;
	unsigned int bucket_size;

	bucket_size = (num_threads / CONFIG_NUM_PREEMPT_PRIORITIES) + 1;

	for (i = 0; i < num_threads; i++) {
		z_init_thread_base(&dummy_thread[i], i / bucket_size,
				   _THREAD_DUMMY, 0);
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

/**
 * Each successive dummy thread added to the wait queue is either of the
 * same or lower priority. Each dummy thread removed from the wait queue
 * is of the same or lower priority than the one previous.
 */
static void test_decreasing_priority(_wait_q_t *q, unsigned int num_threads)
{
	unsigned int i;
	timing_t start;
	timing_t finish;

	/* Add to tail of wait queue */

	for (i = 0; i < num_threads; i++) {
		start = timing_counter_get();
		z_pend_thread((struct k_thread *)&dummy_thread[i],
			      q, K_FOREVER);
		finish = timing_counter_get();

		add_cycles[i] += timing_cycles_get(&start, &finish);
	}

	/* Remove from head of wait queue */

	for (i = 0; i < num_threads; i++) {
		start = timing_counter_get();
		z_unpend_thread((struct k_thread *)&dummy_thread[i]);
		finish = timing_counter_get();

		remove_cycles[i] += timing_cycles_get(&start, &finish);
	}
}

static void test_increasing_priority(_wait_q_t *q, unsigned int num_threads)
{
	unsigned int i;
	timing_t start;
	timing_t finish;
	struct k_thread *thread;

	/* Add to head of wait queue */

	for (i = 0; i < num_threads; i++) {
		start = timing_counter_get();
		thread = (struct k_thread *)
			 &dummy_thread[num_threads - i - 1];
		z_pend_thread(thread, q, K_FOREVER);
		finish = timing_counter_get();

		add_cycles[i] += timing_cycles_get(&start, &finish);
	}

	/* Remove from tail of wait queue */

	for (i = 0; i < num_threads; i++) {
		start = timing_counter_get();
		thread = (struct k_thread *)
			 &dummy_thread[num_threads - i - 1];
		z_unpend_thread(thread);
		finish = timing_counter_get();

		remove_cycles[i] += timing_cycles_get(&start, &finish);
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

	/* Calculate standard deviation */

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

	printk("Time Measurements for %s wait queues\n",
	       IS_ENABLED(CONFIG_WAITQ_DUMB) ? "dumb" : "scalable");
	printk("Timing results: Clock frequency: %u MHz\n", freq);

	z_waitq_init(&wait_q);

	dummy_threads_init(CONFIG_BENCHMARK_NUM_THREADS);

	timing_start();

	cycles_reset(CONFIG_BENCHMARK_NUM_THREADS);

	for (i = 0; i < CONFIG_BENCHMARK_NUM_ITERATIONS; i++) {
		test_decreasing_priority(&wait_q, CONFIG_BENCHMARK_NUM_THREADS);
	}

	compute_and_report_stats(CONFIG_BENCHMARK_NUM_THREADS,
				 CONFIG_BENCHMARK_NUM_ITERATIONS,
				 add_cycles,
				 "Add threads of decreasing priority");

#ifdef CONFIG_BENCHMARK_VERBOSE
	for (i = 0; i < CONFIG_BENCHMARK_NUM_THREADS; i++) {
		snprintf(tag, sizeof(tag),
			 "WaitQ.add.to.tail.%04u.waiters", i);
		snprintf(description, sizeof(description),
			 "%-40s - Add thread of priority %u",
			 tag, dummy_thread[i].prio);
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
			 "WaitQ.remove.from.head.%04u.waiters",
			 CONFIG_BENCHMARK_NUM_THREADS - i);
		snprintf(description, sizeof(description),
			 "%-40s - Remove thread of priority %u",
			 tag, dummy_thread[i].prio);
		PRINT_STATS_AVG(description, (uint32_t)remove_cycles[i],
				CONFIG_BENCHMARK_NUM_ITERATIONS);
	}
#endif

	printk("------------------------------------\n");

	cycles_reset(CONFIG_BENCHMARK_NUM_THREADS);

	for (i = 0; i < CONFIG_BENCHMARK_NUM_ITERATIONS; i++) {
		test_increasing_priority(&wait_q, CONFIG_BENCHMARK_NUM_THREADS);
	}

	compute_and_report_stats(CONFIG_BENCHMARK_NUM_THREADS,
				 CONFIG_BENCHMARK_NUM_ITERATIONS,
				add_cycles,
				 "Add threads of increasing priority");

#ifdef CONFIG_BENCHMARK_VERBOSE
	for (i = 0; i < CONFIG_BENCHMARK_NUM_THREADS; i++) {
		snprintf(tag, sizeof(tag),
			 "WaitQ.add.to.head.%04u.waiters", i);
		thread = (struct k_thread *)
			 &dummy_thread[CONFIG_BENCHMARK_NUM_THREADS - i - 1];
		snprintf(description, sizeof(description),
			 "%-40s - Add priority %u to waitq",
			 tag, thread->base.prio);
		PRINT_STATS_AVG(description, (uint32_t)add_cycles[i],
				CONFIG_BENCHMARK_NUM_ITERATIONS);
	}
#endif

	printk("------------------------------------\n");

	compute_and_report_stats(CONFIG_BENCHMARK_NUM_THREADS,
				 CONFIG_BENCHMARK_NUM_ITERATIONS,
				 remove_cycles,
				 "Remove threads of increasing priority");

#ifdef CONFIG_BENCHMARK_VERBOSE
	for (i = 0; i < CONFIG_BENCHMARK_NUM_THREADS; i++) {
		snprintf(tag, sizeof(tag),
			"WaitQ.remove.from.tail.%04u.waiters",
			CONFIG_BENCHMARK_NUM_THREADS - i);
		thread = (struct k_thread *)
			 &dummy_thread[CONFIG_BENCHMARK_NUM_THREADS - i - 1];
		snprintf(description, sizeof(description),
			 "%-40s - Remove priority %u from waitq",
			 tag, thread->base.prio);
		PRINT_STATS_AVG(description, (uint32_t)remove_cycles[i],
				CONFIG_BENCHMARK_NUM_ITERATIONS);
	}
#endif

	timing_stop();

	TC_END_REPORT(0);

	return 0;
}
