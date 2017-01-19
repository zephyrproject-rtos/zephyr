/* critical.c - test the offload workqueue API */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * DESCRIPTION
 * This module tests the offload workqueue.
 */

#include <zephyr.h>
#include <sections.h>
#include <ztest.h>

#define NUM_MILLISECONDS        5000
#define TEST_TIMEOUT            20000

static uint32_t criticalVar;
static uint32_t altTaskIterations;

static struct k_work_q offload_work_q;
static char __stack offload_work_q_stack[CONFIG_OFFLOAD_WORKQUEUE_STACK_SIZE];

#define STACK_SIZE 1024
static char __stack stack1[STACK_SIZE];
static char __stack stack2[STACK_SIZE];

K_SEM_DEFINE(ALT_SEM, 0, UINT_MAX);
K_SEM_DEFINE(REGRESS_SEM, 0, UINT_MAX);
K_SEM_DEFINE(TEST_SEM, 0, UINT_MAX);

/**
 *
 * @brief Routine to be called from a workqueue
 *
 * This routine increments the global variable <criticalVar>.
 *
 * @return 0
 */

void criticalRtn(struct k_work *unused)
{
	volatile uint32_t x;

	ARG_UNUSED(unused);

	x = criticalVar;
	criticalVar = x + 1;

}

/**
 *
 * @brief Common code for invoking offload work
 *
 * @param count number of critical section calls made thus far
 *
 * @return number of critical section calls made by task
 */

uint32_t criticalLoop(uint32_t count)
{
	int64_t mseconds;

	mseconds = k_uptime_get();
	while (k_uptime_get() < mseconds + NUM_MILLISECONDS) {
		struct k_work work_item;

		k_work_init(&work_item, criticalRtn);
		k_work_submit_to_queue(&offload_work_q, &work_item);
		count++;
	}

	return count;
}

/**
 *
 * @brief Alternate task
 *
 * This routine invokes the workqueue many times.
 *
 * @return N/A
 */

void AlternateTask(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	k_sem_take(&ALT_SEM, K_FOREVER);        /* Wait to be activated */

	altTaskIterations = criticalLoop(altTaskIterations);

	k_sem_give(&REGRESS_SEM);

	k_sem_take(&ALT_SEM, K_FOREVER);        /* Wait to be re-activated */

	altTaskIterations = criticalLoop(altTaskIterations);

	k_sem_give(&REGRESS_SEM);
}

/**
 *
 * @brief Regression task
 *
 * This routine calls invokes the workqueue many times. It also checks to
 * ensure that the number of times it is called matches the global variable
 * <criticalVar>.
 *
 * @return N/A
 */

void RegressionTask(void *arg1, void *arg2, void *arg3)
{
	uint32_t nCalls = 0;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	k_sem_give(&ALT_SEM);   /* Activate AlternateTask() */

	nCalls = criticalLoop(nCalls);

	/* Wait for AlternateTask() to complete */
	assert_true(k_sem_take(&REGRESS_SEM, TEST_TIMEOUT) == 0,
		    "Timed out waiting for REGRESS_SEM");

	assert_equal(criticalVar, nCalls + altTaskIterations,
		     "Unexpected value for <criticalVar>");

	k_sched_time_slice_set(10, 10);

	k_sem_give(&ALT_SEM);   /* Re-activate AlternateTask() */

	nCalls = criticalLoop(nCalls);

	/* Wait for AlternateTask() to finish */
	assert_true(k_sem_take(&REGRESS_SEM, TEST_TIMEOUT) == 0,
		    "Timed out waiting for REGRESS_SEM");

	assert_equal(criticalVar, nCalls + altTaskIterations,
		     "Unexpected value for <criticalVar>");

	k_sem_give(&TEST_SEM);

}

static void init_objects(void)
{
	criticalVar = 0;
	altTaskIterations = 0;
	k_work_q_start(&offload_work_q,
		       offload_work_q_stack,
		       sizeof(offload_work_q_stack),
		       CONFIG_OFFLOAD_WORKQUEUE_PRIORITY);
}

static void start_threads(void)
{
	k_thread_spawn(stack1, STACK_SIZE,
		       AlternateTask, NULL, NULL, NULL,
		       K_PRIO_PREEMPT(12), 0, 0);

	k_thread_spawn(stack2, STACK_SIZE,
		       RegressionTask, NULL, NULL, NULL,
		       K_PRIO_PREEMPT(12), 0, 0);
}

void test_critical(void)
{
	init_objects();
	start_threads();

	assert_true(k_sem_take(&TEST_SEM, TEST_TIMEOUT * 2) == 0,
		    "Timed out waiting for TEST_SEM");
}

void test_main(void)
{
	ztest_test_suite(kernel_critical_test, ztest_unit_test(test_critical));

	ztest_run_test_suite(kernel_critical_test);
}
