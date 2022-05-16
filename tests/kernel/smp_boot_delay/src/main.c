/* Copyright (c) 2021 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

/* Experimentally 10ms is enough time to get the second CPU to run on
 * all known platforms.
 */
#define CPU_START_DELAY 10000

/* IPIs happen  much faster than CPU startup */
#define CPU_IPI_DELAY 100

BUILD_ASSERT(CONFIG_SMP);
BUILD_ASSERT(CONFIG_SMP_BOOT_DELAY);
BUILD_ASSERT(CONFIG_KERNEL_COHERENCE);
BUILD_ASSERT(CONFIG_MP_NUM_CPUS > 1);

#define STACKSZ 2048
char stack[STACKSZ];

volatile bool mp_flag;

struct k_thread cpu1_thr;
K_THREAD_STACK_DEFINE(thr_stack, STACKSZ);

extern void z_smp_start_cpu(int id);

static void thread_fn(void *a, void *b, void *c)
{
	mp_flag = true;
}

void test_smp_boot_delay(void)
{
	/* Create a thread of lower priority.  This could run on
	 * another CPU if it was available, but will not preempt us
	 * unless we block (which we do not).
	 */
	k_thread_create(&cpu1_thr, thr_stack, K_THREAD_STACK_SIZEOF(thr_stack),
			thread_fn, NULL, NULL, NULL,
			1, 0, K_NO_WAIT);

	/* Make sure that thread has not run (because the cpu is halted) */
	k_busy_wait(CPU_START_DELAY);
	zassert_false(mp_flag, "CPU1 must not be running yet");

	/* Start the second CPU */
	z_smp_start_cpu(1);

	/* Verify the thread ran */
	k_busy_wait(CPU_START_DELAY);
	zassert_true(mp_flag, "CPU1 did not start");

	k_thread_abort(&cpu1_thr);
}

void test_post_boot_ipi(void)
{
	/* Spawn the same thread to do the same thing, but this time
	 * expect that the thread is going to run synchronously on the
	 * other CPU as soon as its created.  Intended to test whether
	 * IPIs were correctly set up on the runtime-launched CPU.
	 */
	mp_flag = false;
	k_thread_create(&cpu1_thr, thr_stack, K_THREAD_STACK_SIZEOF(thr_stack),
			thread_fn, NULL, NULL, NULL,
			1, 0, K_NO_WAIT);

	k_busy_wait(CPU_IPI_DELAY);
	zassert_true(mp_flag, "CPU1 did not start thread via IPI");
}

void test_main(void)
{
	ztest_test_suite(smp_boot_delay,
			 ztest_unit_test(test_smp_boot_delay),
			 ztest_unit_test(test_post_boot_ipi)
			 );

	ztest_run_test_suite(smp_boot_delay);
}
