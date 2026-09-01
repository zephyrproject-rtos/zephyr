/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_SMP
#error Cannot test MP API if SMP is using the CPUs
#endif

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS > 1);

#define CPU_STACK_SIZE 1024

K_THREAD_STACK_ARRAY_DEFINE(cpu_stacks, CONFIG_MP_MAX_NUM_CPUS, CPU_STACK_SIZE);

int cpu_arg;

volatile int cpu_running[CONFIG_MP_MAX_NUM_CPUS];

/**
 * @brief Tests for the architecture multiprocessing (MP) bring-up API
 *
 * @defgroup kernel_mp_tests MP Tests
 *
 * @ingroup all_tests
 *
 * These tests exercise arch_cpu_start() directly, without the SMP scheduler
 * owning the secondary CPUs.
 * @{
 * @}
 */

/* Entry point executed on each started CPU: validates the argument it was
 * handed and flags that the CPU is running.
 */
FUNC_NORETURN void cpu_fn(void *arg)
{
	zassert_true(arg == &cpu_arg, "mismatched arg");

	int cpu_id = (*(int *)arg) / 12345;
	int mod = (*(int *)arg) % 12345;

	zassert_true(mod == 0, "wrong arg");

	cpu_running[cpu_id] = 1;

	while (1) {
	}
}

/**
 * @brief Verify that arch_cpu_start() brings up every non-boot CPU.
 *
 * @ingroup kernel_mp_tests
 *
 * @details
 * The architecture layer must provide a means to start the non-boot CPUs and
 * to run a caller-supplied function on them with a caller-supplied stack and
 * argument. Each CPU is started with a distinct argument value, and its entry
 * point validates both the argument pointer and its content before flagging
 * that it is running, so a pass proves the CPU came up and was handed the
 * right parameters.
 *
 * Test steps:
 * - For every CPU other than the boot CPU, set the shared argument to a value
 *   derived from the CPU index.
 * - Call arch_cpu_start() with that CPU's stack, stack size, entry function
 *   and the argument address.
 * - In the entry function, check the argument address and value, then set the
 *   per-CPU running flag.
 * - Poll the running flag for up to five seconds.
 *
 * Expected result:
 * - Every non-boot CPU runs the supplied function with the expected argument
 *   and reports itself running within the timeout.
 *
 * @see arch_cpu_start()
 */
ZTEST(multiprocessing, test_mp_start)
{
	for (int i = 1; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		int wait_count;

		TC_PRINT("Starting CPU #%d...\n", i);

		cpu_arg = 12345 * i;

		arch_cpu_start(i, cpu_stacks[i], CPU_STACK_SIZE, cpu_fn, &cpu_arg);

		/* Wait for about 5 (500 * 10ms) seconds for CPU to start. */
		wait_count = 500;
		while (!cpu_running[i]) {
			k_busy_wait(10 * USEC_PER_MSEC);

			wait_count--;
			if (wait_count < 0) {
				break;
			}
		}

		zassert_true(cpu_running[i], "cpu #%d didn't start", i);
	}
}

ZTEST_SUITE(multiprocessing, NULL, NULL, NULL, NULL, NULL);
