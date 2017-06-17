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
#include <linker/sections.h>
#include <ztest.h>

#define NUM_MILLISECONDS        5000
#define TEST_TIMEOUT            20000

static u32_t criticalVar;
static u32_t altTaskIterations;

static struct k_work_q offload_work_q;
static K_THREAD_STACK_DEFINE(offload_work_q_stack,
			     CONFIG_OFFLOAD_WORKQUEUE_STACK_SIZE);

#define STACK_SIZE 1024
static K_THREAD_STACK_DEFINE(stack1, STACK_SIZE);
static K_THREAD_STACK_DEFINE(stack2, STACK_SIZE);

static struct k_thread thread1;
static struct k_thread thread2;

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
	volatile u32_t x;

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

u32_t criticalLoop(u32_t count)
{
	s64_t mseconds;

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
	u32_t nCalls = 0;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	k_sem_give(&ALT_SEM);   /* Activate AlternateTask() */

	nCalls = criticalLoop(nCalls);

	/* Wait for AlternateTask() to complete */
	zassert_true(k_sem_take(&REGRESS_SEM, TEST_TIMEOUT) == 0,
		     "Timed out waiting for REGRESS_SEM");

	zassert_equal(criticalVar, nCalls + altTaskIterations,
		      "Unexpected value for <criticalVar>");

	k_sched_time_slice_set(10, 10);

	k_sem_give(&ALT_SEM);   /* Re-activate AlternateTask() */

	nCalls = criticalLoop(nCalls);

	/* Wait for AlternateTask() to finish */
	zassert_true(k_sem_take(&REGRESS_SEM, TEST_TIMEOUT) == 0,
		     "Timed out waiting for REGRESS_SEM");

	zassert_equal(criticalVar, nCalls + altTaskIterations,
		      "Unexpected value for <criticalVar>");

	k_sem_give(&TEST_SEM);

}

static void init_objects(void)
{
	criticalVar = 0;
	altTaskIterations = 0;
	k_work_q_start(&offload_work_q,
		       offload_work_q_stack,
		       K_THREAD_STACK_SIZEOF(offload_work_q_stack),
		       CONFIG_OFFLOAD_WORKQUEUE_PRIORITY);
}

static void start_threads(void)
{
	k_thread_create(&thread1, stack1, STACK_SIZE,
			AlternateTask, NULL, NULL, NULL,
			K_PRIO_PREEMPT(12), 0, 0);

	k_thread_create(&thread2, stack2, STACK_SIZE,
			RegressionTask, NULL, NULL, NULL,
			K_PRIO_PREEMPT(12), 0, 0);
}

void test_critical(void)
{
	init_objects();
	start_threads();

	zassert_true(k_sem_take(&TEST_SEM, TEST_TIMEOUT * 2) == 0,
		     "Timed out waiting for TEST_SEM");
}

void test_main(void)
{
	ztest_test_suite(kernel_critical_test, ztest_unit_test(test_critical));

	ztest_run_test_suite(kernel_critical_test);
}
