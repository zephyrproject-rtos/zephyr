/* Copyright (c) 2022 Intel Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include "tests.h"

/* Experimentally 10ms is enough time to get the second CPU to run on
 * all known platforms.
 */
#define CPU_START_DELAY 10000

/* IPIs happen  much faster than CPU startup */
#define CPU_IPI_DELAY 250

BUILD_ASSERT(CONFIG_SMP);
BUILD_ASSERT(CONFIG_SMP_BOOT_DELAY);

#define STACKSZ 2048
char stack[STACKSZ];

volatile bool mp_flag;

struct k_thread cpu_thr;
K_THREAD_STACK_DEFINE(thr_stack, STACKSZ);

extern void z_smp_start_cpu(int id);

static void thread_fn(void *a, void *b, void *c)
{
	int cpuid = (int) a;

	zassert_true(cpuid == 0 || cpuid == arch_curr_cpu()->id,
		     "running on wrong cpu");
	mp_flag = true;
}

/* Needless to say: since this is starting the SMP CPUs, it needs to
 * be the first test run!
 */
ZTEST(intel_adsp_boot, test_1st_smp_boot_delay)
{
	if (CONFIG_MP_NUM_CPUS < 2) {
		ztest_test_skip();
	}

	for (int i = 1; i < CONFIG_MP_NUM_CPUS; i++) {
		printk("Launch cpu%d\n", i);
		mp_flag = false;
		k_thread_create(&cpu_thr, thr_stack, K_THREAD_STACK_SIZEOF(thr_stack),
				thread_fn, (void *)i, NULL, NULL,
				0, 0, K_FOREVER);
		k_thread_cpu_mask_clear(&cpu_thr);
		k_thread_cpu_mask_enable(&cpu_thr, i);
		k_thread_start(&cpu_thr);

		/* Make sure that thread has not run (because the cpu is halted) */
		k_busy_wait(CPU_START_DELAY);
		zassert_false(mp_flag, "cpu %d must not be running yet", i);

		/* Start the second CPU */
		z_smp_start_cpu(i);

		/* Verify the thread ran */
		k_busy_wait(CPU_START_DELAY);
		zassert_true(mp_flag, "cpu %d did not start", i);

		k_thread_abort(&cpu_thr);
	}
}

ZTEST(intel_adsp_boot, test_3rd_post_boot_ipi)
{
	if (CONFIG_MP_NUM_CPUS < 2) {
		ztest_test_skip();
	}

	/* Spawn the same thread to do the same thing, but this time
	 * expect that the thread is going to run synchronously on
	 * another CPU as soon as its created.  Intended to test
	 * whether IPIs were correctly set up on the runtime-launched
	 * CPU.
	 */
	mp_flag = false;
	k_thread_create(&cpu_thr, thr_stack, K_THREAD_STACK_SIZEOF(thr_stack),
			thread_fn, (void *)0, NULL, NULL,
			1, 0, K_NO_WAIT);

	k_busy_wait(CPU_IPI_DELAY);
	zassert_true(mp_flag, "cpu did not start thread via IPI");
}
