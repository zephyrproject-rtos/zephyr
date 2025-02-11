/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/pm/pm.h>
#include <zephyr/irq_offload.h>
#include <zephyr/debug/stack.h>

#define SLEEP_MS 100
#define NUM_OF_WORK 2
#define TEST_STRING "TEST"

static struct k_work work[NUM_OF_WORK];
static struct k_sem sync_sema;

/**TESTPOINT: stack analyze*/
static void tdata_dump_callback(const struct k_thread *thread, void *user_data)
{
	log_stack_usage(thread);
}

/*work handler*/
static void work_handler(struct k_work *w)
{
	k_thread_foreach(tdata_dump_callback, NULL);
	k_sem_give(&sync_sema);
}

/**
 * @brief Tests for kernel profiling
 * @defgroup kernel_profiling_tests Profiling
 * @ingroup all_tests
 * @{
 * @}
 */

/**
 * @brief Test stack usage through main thread
 *
 * This test prints the main, idle, interrupt and system workqueue
 * stack usage through main thread.
 *
 * @ingroup kernel_profiling_tests
 *
 * @see k_thread_foreach(), log_stack_usage()
 */
ZTEST(profiling_api, test_call_stacks_analyze_main)
{
	TC_PRINT("from main thread:\n");
	k_thread_foreach(tdata_dump_callback, NULL);
}

/**
 * @brief Test stack usage through idle thread
 *
 * This test prints the main, idle, interrupt and system workqueue
 * stack usage through idle thread.
 *
 * @ingroup kernel_profiling_tests
 *
 * @see k_thread_foreach(), pm_system_suspend(), pm_system_resume(),
 * log_stack_usage()
 */
ZTEST(profiling_api_1cpu, test_call_stacks_analyze_idle)
{
	TC_PRINT("from idle thread:\n");
	k_msleep(SLEEP_MS);
}

/**
 * @brief Test stack usage through system workqueue
 *
 * This test prints the main, idle, interrupt and system workqueue
 * stack usage through system workqueue.
 *
 * @ingroup kernel_profiling_tests
 *
 * @see k_thread_foreach(), k_work_init(), k_work_submit(),
 * log_stack_usage()
 */
ZTEST(profiling_api_1cpu, test_call_stacks_analyze_workq)
{
	TC_PRINT("from workq:\n");
	k_sem_init(&sync_sema, 0, NUM_OF_WORK);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_work_init(&work[i], work_handler);
		k_work_submit(&work[i]);
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/*TODO: add test case to capture the usage of interrupt call stack*/

ZTEST_SUITE(profiling_api, NULL, NULL, NULL, NULL, NULL);

ZTEST_SUITE(profiling_api_1cpu, NULL, NULL,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
