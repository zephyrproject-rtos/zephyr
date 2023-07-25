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
 * @brief Tests for multi processing
 *
 * @defgroup kernel_mp_tests MP Tests
 *
 * @ingroup all_tests
 *
 * @{
 * @}
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
 * @brief Test to verify CPU start
 *
 * @ingroup kernel_mp_tests
 *
 * @details
 * Test Objective:
 * - To verify kernel architecture layer shall provide a means to start non-boot
 *   CPUs on SMP systems.
 *   The way we verify it is to call it by give it parameters especially the
 *   target executing function, etc. Then check if the function is running or
 *   not.
 *
 * Testing techniques:
 * - Interface testing, function and block box testing,
 *   dynamic analysis and testing
 *
 * Prerequisite Conditions:
 * - CONFIG_MP_MAX_NUM_CPUS > 1
 *
 * Input Specifications:
 * - CPU ID: the cpu want to start
 * - Stack structure
 * - Stack size
 * - Target executing function
 * - An argument that pass to the function
 *
 * Test Procedure:
 * -# In main thread, given and set a global variable cpu_arg to 12345.
 * -# Call arch_start_cpu() with parameters
 * -# Enter a while loop and wait for cpu_running equals to 1.
 * -# In target function, check if the address is &cpu_arg and its content
 *  equal to 12345.
 * -# Set the global flag variable cpu_running to 1.
 * -# In main thread, check if the cpu_running equals to 1.
 *
 * Expected Test Result:
 * - The given function execute cpu is running and .
 *
 * Pass/Fail Criteria:
 * - Successful if the check of step 4, 6 are all pass.
 * - Failure if one of the check of step 4, 6 is failed.
 *
 * Assumptions and Constraints:
 * - This test using for the platform that support MP or SMP, in our current
 *   scenario which own over two CPUs.
 *
 * @see arch_start_cpu()
 */
ZTEST(multiprocessing, test_mp_start)
{
	for (int i = 1; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		int wait_count;

		TC_PRINT("Starting CPU #%d...\n", i);

		cpu_arg = 12345 * i;

		arch_start_cpu(i, cpu_stacks[i], CPU_STACK_SIZE, cpu_fn, &cpu_arg);

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
