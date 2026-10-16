/*
 * Copyright (c) 2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>

#if CONFIG_MP_MAX_NUM_CPUS < 2
#error "SMP test requires at least two CPUs!"
#endif

#define NUM_THREADS CONFIG_MP_MAX_NUM_CPUS
#define STACK_SIZE 1024 + CONFIG_TEST_EXTRA_STACK_SIZE

K_THREAD_STACK_ARRAY_DEFINE(thread_stack, NUM_THREADS, STACK_SIZE);
struct k_thread thread[NUM_THREADS];

struct isr_args {
	volatile bool *sync;
	volatile bool *wait;
	struct k_thread *target;
};

volatile bool sync[NUM_THREADS];

struct isr_args isr_args[NUM_THREADS];

static void isr(const void *args)
{
	const struct isr_args *var = args;

	*(var->sync) = true;            /* Flag that ISR is in progress */

	while (*(var->wait) == false) { /* Wait upon dependent CPU */
	}

	k_thread_abort(var->target);    /* Abort thread on another CPU */

	/*
	 * Give other CPUs time to also call k_thread_abort() to establish
	 * the circular dependency that this test is designed to exercise.
	 * On platforms with large emulator/simulator quantum sizes, without
	 * this delay one thread may complete its ISR and return before
	 * another thread even calls k_thread_abort() on it.
	 */
	k_busy_wait(100);
}

static void thread_entry(void *p1, void *p2, void *p3)
{
	unsigned int index = (unsigned int)(uintptr_t)p1;
	struct isr_args *var = p2;

	printk("Thread %u started\n", index);

	irq_offload(isr, var);

	zassert_true(false, "Thread %u did not abort!", index);
}

/**
 * @brief Verify that circular cross-CPU thread aborts from ISRs do not
 *        deadlock.
 *
 * @ingroup kernel_smp_tests
 *
 * @details
 * Aborting a thread that is running on another CPU makes the aborting CPU wait
 * for that thread to be switched out. If every CPU simultaneously aborts the
 * thread interrupted on the next CPU, the wait becomes circular and a naive
 * implementation deadlocks. The test sets up exactly that ring: one thread per
 * CPU, each entering an ISR that waits for the next thread to reach its own
 * ISR before aborting it.
 *
 * Test steps:
 * - Build the abort ring so thread i waits on thread i+1 and aborts it,
 *   wrapping around at the last thread.
 * - Create one thread per CPU at a priority above the test thread; each thread
 *   immediately invokes an ISR through irq_offload().
 * - In each ISR, flag that it is in progress, spin until the next thread's ISR
 *   is in progress, call k_thread_abort() on that thread, then delay so the
 *   other CPUs reach their abort call too.
 * - Join every thread from the test thread.
 *
 * Expected result:
 * - All threads are aborted and joined, and no thread returns from its ISR
 *   into its entry point.
 *
 * @see k_thread_abort()
 * @see irq_offload()
 */
ZTEST(smp_abort, test_smp_thread_abort_deadlock)
{
	unsigned int  i;
	int  priority;

	priority = k_thread_priority_get(k_current_get());

	/*
	 * Each thread will run on its own CPU and invoke an ISR.
	 * Each ISR will wait until the next thread enters its ISR
	 * before attempting to abort that thread. This ensures that
	 * we have a scenario where each CPU is attempting to abort
	 * the active thread that was interrupted by an ISR.
	 */

	for (i = 0; i < NUM_THREADS; i++) {
		isr_args[i].sync = &sync[i];
		isr_args[i].wait = &sync[(i + 1) % NUM_THREADS];
		isr_args[i].target = &thread[(i + 1) % NUM_THREADS];
	}

	for (i = 0; i < NUM_THREADS; i++) {

		k_thread_create(&thread[i], thread_stack[i],
				STACK_SIZE, thread_entry,
				(void *)(uintptr_t)i, &isr_args[i], NULL,
				priority - 1, 0, K_NO_WAIT);
	}

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_join(&thread[i], K_FOREVER);
	}

	printk("Done!\n");
}

ZTEST_SUITE(smp_abort, NULL, NULL, NULL, NULL, NULL);
