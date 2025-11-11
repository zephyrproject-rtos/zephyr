/*
 * Copyright (c) 2023,2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#if CONFIG_MP_MAX_NUM_CPUS == 1
#error "Test requires a system with more than 1 CPU"
#endif

#define IPI_TEST_INTERVAL_DURATION 30

#define NUM_WORK_THREADS (CONFIG_MP_MAX_NUM_CPUS - 1)
#define WORK_STACK_SIZE  4096

#define NUM_PREEMPTIVE_THREADS 5
#define PREEMPTIVE_STACK_SIZE 4096

static K_THREAD_STACK_ARRAY_DEFINE(work_stack, NUM_WORK_THREADS, WORK_STACK_SIZE);
static K_THREAD_STACK_ARRAY_DEFINE(preemptive_stack, NUM_PREEMPTIVE_THREADS, PREEMPTIVE_STACK_SIZE);

static struct k_thread work_thread[NUM_WORK_THREADS];
static unsigned long work_array[NUM_WORK_THREADS][1024];
static volatile unsigned long work_counter[NUM_WORK_THREADS];

static struct k_thread preemptive_thread[NUM_PREEMPTIVE_THREADS];
static unsigned int preemptive_counter[NUM_PREEMPTIVE_THREADS];

static atomic_t ipi_counter;

void z_trace_sched_ipi(void)
{
	atomic_inc(&ipi_counter);
}

void work_entry(void *p1, void *p2, void *p3)
{
	unsigned int index = POINTER_TO_UINT(p1);
	unsigned long *array = p2;
	unsigned long counter;

	while (1) {
		for (unsigned int i = 0; i < 1024; i++) {
			counter = work_counter[index]++;

			array[i] = (array[i] + counter) ^ array[i];
		}
	}
}

void preemptive_entry(void *p1, void *p2, void *p3)
{
	unsigned int index = POINTER_TO_UINT(p1);

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct k_thread *suspend = NULL;
	struct k_thread *resume = NULL;

	if (index != (NUM_PREEMPTIVE_THREADS - 1)) {
		resume = &preemptive_thread[index + 1];
	}

	if (index != 0) {
		suspend = k_current_get();
	}

	while (1) {
		if (resume != NULL) {
			k_thread_resume(resume);
		}

		preemptive_counter[index]++;

		if (suspend != NULL) {
			k_thread_suspend(suspend);
		}
	}
}

void report(void)
{
	unsigned int elapsed_time = IPI_TEST_INTERVAL_DURATION;
	unsigned long total_preempt;
	unsigned long total_work;
	unsigned long last_work_counter[NUM_WORK_THREADS] = {};
	unsigned long last_preempt[NUM_PREEMPTIVE_THREADS] = {};
	unsigned long tmp_work_counter[NUM_WORK_THREADS] = {};
	unsigned long tmp_preempt[NUM_PREEMPTIVE_THREADS] = {};
	unsigned int i;
	unsigned int tmp_ipi_counter;

	atomic_set(&ipi_counter, 0);

	while (1) {
		k_sleep(K_SECONDS(IPI_TEST_INTERVAL_DURATION));

		/*
		 * Get local copies of the counters to minimize
		 * the impacts of delays from printf().
		 */

		total_work = 0;
		for (i = 0; i < NUM_WORK_THREADS; i++) {
			tmp_work_counter[i] = work_counter[i];
			total_work += (tmp_work_counter[i] - last_work_counter[i]);
		}

		/* Sum the preemptive counters. */
		total_preempt = 0;
		for (i = 0; i < NUM_PREEMPTIVE_THREADS; i++) {
			tmp_preempt[i] = preemptive_counter[i];
			total_preempt += (tmp_preempt[i] - last_preempt[i]);
		}

		tmp_ipi_counter = (unsigned int)atomic_set(&ipi_counter, 0);

		printf("**** IPI-Metric Basic Scheduling Test **** Elapsed Time: %u\n",
		       elapsed_time);

		printf("  Preemptive Counter Total: %lu\n", total_preempt);
		for (i = 0; i < NUM_PREEMPTIVE_THREADS; i++) {
			printf("    - Counter #%u: %lu\n",
			       i, tmp_preempt[i] - last_preempt[i]);
			last_preempt[i] = tmp_preempt[i];
		}

		printf("  IPI Count: %u\n", tmp_ipi_counter);

		printf("  Total Work: %lu\n", total_work);

		for (i = 0; i < NUM_WORK_THREADS; i++) {
			printf("    - Work Counter #%u: %lu\n",
			       i, tmp_work_counter[i] - last_work_counter[i]);
			last_work_counter[i] = tmp_work_counter[i];
		}

		elapsed_time += IPI_TEST_INTERVAL_DURATION;
	}
}

int main(void)
{
	unsigned int i;

	for (i = 0; i < NUM_WORK_THREADS; i++) {
		k_thread_create(&work_thread[i], work_stack[i],
				WORK_STACK_SIZE, work_entry,
				UINT_TO_POINTER(i), work_array[i], NULL,
				-1, 0, K_NO_WAIT);
	}

	/*
	 * Create the preemptive threads and switch them to
	 * the suspended state.
	 */

	for (i = 0; i < NUM_PREEMPTIVE_THREADS; i++) {
		k_thread_create(&preemptive_thread[i], preemptive_stack[i],
				PREEMPTIVE_STACK_SIZE, preemptive_entry,
				UINT_TO_POINTER(i), NULL, NULL,
				10 - i, 0, K_FOREVER);
		k_thread_suspend(&preemptive_thread[i]);
		k_wakeup(&preemptive_thread[i]);
	}

	k_thread_resume(&preemptive_thread[0]);

	report();
}
