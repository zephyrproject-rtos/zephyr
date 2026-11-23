/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/platform/hooks.h>

#include <kernel_internal.h>

/* Time given to the hook to run on the parked CPUs, in milliseconds */
#define HOOK_TIMEOUT_MS 1000

/* Number of times the hook ran on each CPU */
static atomic_t hook_calls[CONFIG_MP_MAX_NUM_CPUS];

/*
 * Runs on a parked CPU with interrupts locked, so it only uses atomic
 * operations. It waits for an interrupt, the way a SoC would to save power,
 * and returns when one is pending, which leaves the CPU in the default park
 * loop.
 */
void soc_fatal_cpu_park_hook(void)
{
	(void)atomic_inc(&hook_calls[arch_curr_cpu()->id]);

#if defined(CONFIG_ARM64) || defined(CONFIG_RISCV)
	__asm__ volatile("wfi" ::: "memory");
#endif
}

static bool hook_ran_on_cpus(unsigned int halting_cpu)
{
	for (unsigned int cpu = 0; cpu < arch_num_cpus(); cpu++) {
		if ((cpu != halting_cpu) && (atomic_get(&hook_calls[cpu]) == 0)) {
			return false;
		}
	}

	return true;
}

/**
 * @brief The park hook runs once on each CPU that is not halting the system
 *
 * @details Park the other CPUs the way the fatal error handler does, and
 * check that the hook ran exactly once on each of them and not on the CPU
 * that did the parking.
 *
 * The parked CPUs never resume, so this must be the last test run.
 *
 * @ingroup kernel_fatal_tests
 */
ZTEST(park_hook, test_hook_runs_on_parked_cpus)
{
	unsigned int key;
	unsigned int halting_cpu;

	if (arch_num_cpus() < 2U) {
		ztest_test_skip();
	}

	/*
	 * z_fatal_pause_cpus() leaves interrupts locked, take a key first to
	 * be able to restore them once the other CPUs are parked.
	 */
	key = arch_irq_lock();
	halting_cpu = arch_curr_cpu()->id;

	z_fatal_pause_cpus();

	arch_irq_unlock(key);

	/*
	 * A CPU is counted as parked before the hook runs, so z_fatal_pause_cpus()
	 * returning does not mean the hook has run yet.
	 */
	for (int i = 0; (i < HOOK_TIMEOUT_MS) && !hook_ran_on_cpus(halting_cpu); i++) {
		k_busy_wait(USEC_PER_MSEC);
	}

	for (unsigned int cpu = 0; cpu < arch_num_cpus(); cpu++) {
		atomic_val_t expected = (cpu == halting_cpu) ? 0 : 1;

		zassert_equal(atomic_get(&hook_calls[cpu]), expected,
			      "hook ran %ld times on CPU %u, expected %ld",
			      (long)atomic_get(&hook_calls[cpu]), cpu, (long)expected);
	}
}

ZTEST_SUITE(park_hook, NULL, NULL, NULL, NULL, NULL);
