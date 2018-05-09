/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_profiling
 * @{
 * @defgroup t_profiling_api test_profiling_api
 * @brief TestPurpose: verify profiling APIs.
 * @details All TESTPOINTs extracted from kernel-doc comments in <kernel.h>
 * - API coverage
 *   - k_call_stacks_analyze
 * @}
 */

#include <ztest.h>
#include <irq_offload.h>

#define SLEEP_MS 100
#define NUM_OF_WORK 2

static struct k_work work[NUM_OF_WORK];
static struct k_sem sync_sema;

static void tprofiling_stack(void *p)
{
	/**TESTPOINT: stack analyze*/
	for (int i = 0; i < 2; i++) {
		k_call_stacks_analyze();
	}
}

/*power hook functions*/
int _sys_soc_suspend(s32_t ticks)
{
	tprofiling_stack(NULL);
	return 0;
}

void _sys_soc_resume(void)
{
}

/*work handler*/
static void work_handler(struct k_work *w)
{
	tprofiling_stack(NULL);
	k_sem_give(&sync_sema);
}

/*test cases*/
void test_call_stacks_analyze_main(void)
{
	TC_PRINT("from main thread:\n");
	tprofiling_stack(NULL);
}

void test_call_stacks_analyze_idle(void)
{
	TC_PRINT("from idle thread:\n");
	k_sleep(SLEEP_MS);
}

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
