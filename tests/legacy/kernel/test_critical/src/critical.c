/* critical.c - test the task_offload_to_fiber() API */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
DESCRIPTION
This module tests the task_offload_to_fiber() API.
 */

#include <zephyr.h>
#include <tc_util.h>
#include <sections.h>

#define NUM_TICKS    500
#define TEST_TIMEOUT 2000

static uint32_t  criticalVar = 0;
static uint32_t altTaskIterations = 0;

/**
 *
 * @brief Routine to be called from _k_server()
 *
 * This routine increments the global variable <criticalVar>.
 *
 * @return 0
 */

int criticalRtn(void)
{
	volatile  uint32_t x;

	x = criticalVar;
	criticalVar = x + 1;

	return 0;
}

/**
 *
 * @brief Common code for invoking task_offload_to_fiber()
 *
 * @param count    number of critical section calls made thus far
 *
 * @return number of critical section calls made by task
 */

uint32_t criticalLoop(uint32_t count)
{
	int32_t  ticks;

	ticks = sys_tick_get_32();
	while (sys_tick_get_32() < ticks + NUM_TICKS) {
		task_offload_to_fiber(criticalRtn, &criticalVar);
		count++;
	}

	return count;
}

/**
 *
 * @brief Alternate task
 *
 * This routine calls task_offload_to_fiber() many times.
 *
 * @return N/A
 */

void AlternateTask(void)
{
	task_sem_take(ALT_SEM, TICKS_UNLIMITED);     /* Wait to be activated */

	altTaskIterations = criticalLoop(altTaskIterations);

	task_sem_give(REGRESS_SEM);

	task_sem_take(ALT_SEM, TICKS_UNLIMITED);    /* Wait to be re-activated */

	altTaskIterations = criticalLoop(altTaskIterations);

	task_sem_give(REGRESS_SEM);
}

/**
 *
 * @brief Regression task
 *
 * This routine calls task_offload_to_fiber() many times.  It also checks to
 * ensure that the number of times it is called matches the global variable
 * <criticalVar>.
 *
 * @return N/A
 */

void RegressionTask(void)
{
	uint32_t nCalls = 0;
	int      status;

	TC_START("Test Microkernel Critical Section API\n");

	task_sem_give(ALT_SEM);      /* Activate AlternateTask() */

	nCalls = criticalLoop(nCalls);

	/* Wait for AlternateTask() to complete */
	status = task_sem_take(REGRESS_SEM, TEST_TIMEOUT);
	if (status != RC_OK) {
		TC_ERROR("Timed out waiting for REGRESS_SEM\n");
		goto errorReturn;
	}

	if (criticalVar != nCalls + altTaskIterations) {
		TC_ERROR("Unexpected value for <criticalVar>.  Expected %d, got %d\n",
				 nCalls + altTaskIterations, criticalVar);
		goto errorReturn;
	}
	TC_PRINT("Obtained expected <criticalVar> value of %u\n", criticalVar);

	TC_PRINT("Enabling time slicing ...\n");

	sys_scheduler_time_slice_set(1, 10);

	task_sem_give(ALT_SEM);      /* Re-activate AlternateTask() */

	nCalls = criticalLoop(nCalls);

	/* Wait for AlternateTask() to finish */
	status = task_sem_take(REGRESS_SEM, TEST_TIMEOUT);
	if (status != RC_OK) {
		TC_ERROR("Timed out waiting for REGRESS_SEM\n");
		goto errorReturn;
	}

	if (criticalVar != nCalls + altTaskIterations) {
		TC_ERROR("Unexpected value for <criticalVar>.  Expected %d, got %d\n",
				 nCalls + altTaskIterations, criticalVar);
		goto errorReturn;
	}
	TC_PRINT("Obtained expected <criticalVar> value of %u\n", criticalVar);

	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
	return;

errorReturn:
	TC_END_RESULT(TC_FAIL);
	TC_END_REPORT(TC_FAIL);
}
