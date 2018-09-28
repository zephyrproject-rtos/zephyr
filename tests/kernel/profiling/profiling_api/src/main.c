/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>
#include <misc/stack.h>

#define SLEEP_MS 100
#define NUM_OF_WORK 2
#define TEST_STRING "TEST"

static struct k_work work[NUM_OF_WORK];
static struct k_sem sync_sema;

/**TESTPOINT: stack analyze*/
static void tdata_dump_callback(const struct k_thread *thread, void *user_data)
{
	stack_analyze("Test", (char *)thread->stack_info.start,
						thread->stack_info.size);
}

/*power hook functions*/
int _sys_soc_suspend(s32_t ticks)
{
	static bool test_flag;

	/* Call k_thread_foreach only once otherwise it will
	 * flood the console with stack dumps.
	 */
	if (!test_flag) {
		k_thread_foreach(tdata_dump_callback, NULL);
		test_flag = true;
	}

	return 0;
}

void _sys_soc_resume(void)
{
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
 * @see k_thread_foreach(), stack_analyze()
 */
void test_call_stacks_analyze_main(void)
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
 * @see k_thread_foreach(), _sys_soc_suspend(), _sys_soc_resume(),
 * stack_analyze()
 */
void test_call_stacks_analyze_idle(void)
{
	TC_PRINT("from idle thread:\n");
	k_sleep(SLEEP_MS);
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
 * stack_analyze()
 */
void test_call_stacks_analyze_workq(void)
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

void test_main(void)
{
	ztest_test_suite(profiling_api,
			 ztest_unit_test(test_call_stacks_analyze_main),
			 ztest_unit_test(test_call_stacks_analyze_idle),
			 ztest_unit_test(test_call_stacks_analyze_workq));
	ztest_run_test_suite(profiling_api);
}
