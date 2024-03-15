/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#if CONFIG_MP_MAX_NUM_CPUS < 2
#error "SMP test requires at least two CPUs!"
#endif

#define STACK_SIZE 1024

#define NUM_THREADS 6

K_THREAD_STACK_ARRAY_DEFINE(thread_stack, NUM_THREADS, STACK_SIZE);
struct k_thread thread[NUM_THREADS];

volatile uint64_t thread_counter[NUM_THREADS];

static void thread_entry(void *p1, void *p2, void *p3)
{
	struct k_thread *resume_thread = p1;
	unsigned int self_index = (unsigned int)(uintptr_t)p2;

	while (1) {
		if (resume_thread != NULL) {
			k_thread_resume(resume_thread);
		}

		thread_counter[self_index]++;

		/*
		 * Contentious spinlocks embedded within tight loops (such as
		 * this one) have a CPU bias induced by arch_spin_relax(). We
		 * counteract this by introducing a configurable delay so that
		 * other threads have a chance to acquire the spinlock and
		 * prevent starvation.
		 */

		for (volatile unsigned int i = 0;
		     i < CONFIG_SMP_TEST_RELAX;
		     i++) {
		}

		if (self_index != 0) {
			k_thread_suspend(k_current_get());
		}
	}
}

ZTEST(smp_suspend_resume, test_smp_thread_suspend_resume_stress)
{
	unsigned int  i;
	uint64_t counter[NUM_THREADS] = {};

	/* Create the threads */

	printk("Starting ...\n");

	for (i = 0; i < NUM_THREADS; i++) {

		k_thread_create(&thread[i], thread_stack[i],
				STACK_SIZE, thread_entry,
				i < (NUM_THREADS - 1) ? &thread[i + 1] : NULL,
				(void *)(uintptr_t)i, NULL,
				10 - i, 0, K_FOREVER);

		k_thread_suspend(&thread[i]);

		k_thread_start(&thread[i]);
	}

	/*
	 * All newly created test threads are currently in the suspend state.
	 * Start the first thread.
	 */

	k_thread_resume(&thread[0]);

	for (unsigned int iteration = 0; iteration < 15; iteration++) {
		k_sleep(K_MSEC(1000));

		for (i = 0; i < NUM_THREADS; i++) {
			zassert_false(counter[i] == thread_counter[i],
				      " -- Thread %u is starving: %llu\n",
				      i, thread_counter[i]);

			counter[i] = thread_counter[i];

		}
	}
}

ZTEST_SUITE(smp_suspend_resume, NULL, NULL, NULL, NULL, NULL);
