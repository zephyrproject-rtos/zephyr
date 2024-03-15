/* Copyright (c) 2021 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel/smp.h>
#include <zephyr/ztest.h>

/* Experimentally 10ms is enough time to get the second CPU to run on
 * all known platforms.
 */
#define CPU_START_DELAY 10000

/* IPIs happen  much faster than CPU startup */
#define CPU_IPI_DELAY 1000

BUILD_ASSERT(CONFIG_SMP);
BUILD_ASSERT(CONFIG_SMP_BOOT_DELAY);
BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS > 1);

#define STACKSZ 2048
char stack[STACKSZ];

volatile bool mp_flag;

struct k_thread cpu_thr;
K_THREAD_STACK_DEFINE(thr_stack, STACKSZ);

static void thread_fn(void *a, void *b, void *c)
{
	mp_flag = true;
}

ZTEST(smp_boot_delay, test_smp_boot_delay)
{
	/* Create a thread of lower priority.  This could run on
	 * another CPU if it was available, but will not preempt us
	 * unless we block (which we do not).
	 */
	k_thread_create(&cpu_thr, thr_stack, K_THREAD_STACK_SIZEOF(thr_stack),
			thread_fn, NULL, NULL, NULL,
			1, 0, K_NO_WAIT);

	/* Make sure that thread has not run (because the cpu is halted) */
	k_busy_wait(CPU_START_DELAY);
	zassert_false(mp_flag, "CPU1 must not be running yet");

	/* Start the second CPU */
	k_smp_cpu_start(1, NULL, NULL);

	/* Verify the thread ran */
	k_busy_wait(CPU_START_DELAY);
	zassert_true(mp_flag, "CPU1 did not start");

	k_thread_abort(&cpu_thr);
	k_thread_join(&cpu_thr, K_FOREVER);

	/* Spawn the same thread to do the same thing, but this time
	 * expect that the thread is going to run synchronously on the
	 * other CPU as soon as its created.  Intended to test whether
	 * IPIs were correctly set up on the runtime-launched CPU.
	 */
	mp_flag = false;
	k_thread_create(&cpu_thr, thr_stack, K_THREAD_STACK_SIZEOF(thr_stack),
			thread_fn, NULL, NULL, NULL,
			1, 0, K_NO_WAIT);

	k_busy_wait(CPU_IPI_DELAY);

	k_thread_abort(&cpu_thr);
	k_thread_join(&cpu_thr, K_FOREVER);

	zassert_true(mp_flag, "CPU1 did not start thread via IPI");
}

volatile bool custom_init_flag;

void custom_init_fn(void *arg)
{
	volatile bool *flag = (void *)arg;

	*flag = true;
}

ZTEST(smp_boot_delay, test_smp_custom_start)
{
	k_tid_t thr;

	if (CONFIG_MP_MAX_NUM_CPUS <= 2) {
		/* CPU#1 has been started in test_smp_boot_delay
		 * so we need another CPU for this test.
		 */
		ztest_test_skip();
	}

	mp_flag = false;
	custom_init_flag = false;

	/* Create a thread pinned on CPU#2 so that it will not
	 * run on other CPUs.
	 */
	thr = k_thread_create(&cpu_thr, thr_stack, K_THREAD_STACK_SIZEOF(thr_stack),
			      thread_fn, NULL, NULL, NULL,
			      1, 0, K_FOREVER);
	(void)k_thread_cpu_pin(thr, 2);
	k_thread_start(thr);

	/* Make sure that thread has not run (because the cpu is halted) */
	k_busy_wait(CPU_START_DELAY);
	zassert_false(mp_flag, "CPU2 must not be running yet");

	/* Start the third CPU */
	k_smp_cpu_start(2, custom_init_fn, (void *)&custom_init_flag);

	/* Verify the thread ran */
	k_busy_wait(CPU_START_DELAY);
	zassert_true(mp_flag, "CPU2 did not start");

	/* Verify that the custom init function has been called. */
	zassert_true(custom_init_flag, "Custom init function has not been called.");

	k_thread_abort(&cpu_thr);
	k_thread_join(&cpu_thr, K_FOREVER);
}


ZTEST_SUITE(smp_boot_delay, NULL, NULL, NULL, NULL, NULL);
