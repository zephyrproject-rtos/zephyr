/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <ztest.h>
#include <kernel.h>

#define CPU1_STACK_SIZE 1024

K_THREAD_STACK_DEFINE(cpu1_stack, CPU1_STACK_SIZE);

int cpu_arg;

volatile int cpu_running;

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
void cpu1_fn(int key, void *arg)
{
	zassert_true(key, "bad irq key");
	zassert_true(arg == &cpu_arg && *(int *)arg == 12345, "wrong arg");

	cpu_running = 1;

	while (1) {
	}
}

/**
 * @brief Test to verify CPU start
 *
 * @ingroup kernel_mp_tests
 *
 * @see _arch_start_cpu()
 */
void test_mp_start(void)
{
	cpu_arg = 12345;

	_arch_start_cpu(1, cpu1_stack, CPU1_STACK_SIZE, cpu1_fn, &cpu_arg);

	while (!cpu_running) {
	}

	zassert_true(cpu_running, "cpu1 didn't start");
}

void test_main(void)
{
	ztest_test_suite(multiprocessing,
			 ztest_unit_test(test_mp_start));
	ztest_run_test_suite(multiprocessing);
}
