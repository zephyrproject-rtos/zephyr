/*
 * Copyright Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define STACK_SIZE   2048
#define THREAD_COUNT 7

/*
 * crashed: atomic flag used to coordinate crash handling across SMP cores.
 *
 * Root causes fixed:
 * 1. Busy-loop (k_sleep(0)) generated continuous IPIs causing z_arm64_mm_init
 *    re-entry (MMU assertion) on other CPUs during the coredump.
 * 2. Returning from handler for unexpected faults (stack overflow, spinlock
 *    corruption) caused infinite panic loops on SMP.
 * 3. 1024-byte stack overflowed with blocking k_sem_take + SMP overhead.
 * 4. Double-panic possible if two threads both exited the wait simultaneously.
 */
static atomic_t crashed;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);
	atomic_set(&crashed, 1);

	/*
	 * Only return for the expected K_ERR_KERNEL_PANIC so the panicking
	 * thread can terminate and k_thread_join() completes.
	 * For all other reasons (CPU exception, stack overflow, spinlock
	 * corruption) halt the CPU -- returning from an unexpected fault
	 * leaves state undefined and creates an infinite panic loop.
	 */
	if (reason != K_ERR_KERNEL_PANIC) {
		k_fatal_halt(reason);
	}
}

static struct k_thread threads[THREAD_COUNT];
static uint32_t params[THREAD_COUNT];
static K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, THREAD_COUNT, STACK_SIZE);
static K_SEM_DEFINE(sem, 0, 1);

static void func0(uint32_t param)
{
	/*
	 * Block on the semaphore with a per-thread timeout instead of
	 * busy-looping with k_sleep(0). Blocked threads are NOT running so
	 * they generate no scheduling IPIs during the coredump walk.
	 * atomic_cas ensures only one thread calls k_panic() even if two
	 * threads wake simultaneously on SMP.
	 */
	int ret = k_sem_take(&sem, K_MSEC(500 + param));

	if (ret == 0) {
		int expected = 0;

		if (atomic_cas(&crashed, expected, 1)) {
			k_panic();
		}
	}
	/* Timed out or lost the CAS -- exit cleanly */
}

static void test_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t *param = (uint32_t *)p1;

	func0(*param);
}

static void *coredump_threads_suite_setup(void)
{
	/* Spawn a few threads */
	for (int i = 0; i < THREAD_COUNT; i++) {
		params[i] = i;
		k_thread_create(
			&threads[i],
			thread_stacks[i],
			K_THREAD_STACK_SIZEOF(thread_stacks[i]),
			test_thread_entry,
			&params[i],
			NULL,
			NULL,
			(THREAD_COUNT - i), /* arbitrary priority */
			0,
			K_NO_WAIT);

		char thread_name[32];

		snprintf(thread_name, sizeof(thread_name), "thread%d", i);
		k_thread_name_set(&threads[i], thread_name);
	}

	return NULL;
}

ZTEST_SUITE(coredump_threads, NULL, coredump_threads_suite_setup, NULL, NULL, NULL);

ZTEST(coredump_threads, test_crash)
{
	/* Give semaphore allowing one of the waiting threads to continue and panic */
	k_sem_give(&sem);
	for (int i = 0; i < THREAD_COUNT; i++) {
		k_thread_join(&threads[i], K_FOREVER);
	}
}
