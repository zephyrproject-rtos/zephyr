/* main.c - test pipe APIs (kernel version) */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
DESCRIPTION
This module contains the entry points for the tasks used by the kernel version
of the pipe test application. It also initializes global variables that
identify the various kernel objects used by the test code.

Each test task entry point invokes a test routine that returns a success/failure
indication, then gives a corresponding semaphore. An additional task monitors
these semaphores until it detects a failure or the completion of all test tasks,
then announces the result of the test.
 */

#include <zephyr.h>
#include <tc_util.h>

#define NUM_TEST_TASKS	2	/* # of test tasks to monitor */

/* # ticks to wait for test completion */
#define TIMEOUT	(60 * sys_clock_ticks_per_sec)

ksem_t regSem		= REGRESSION_SEM;
ksem_t altSem		= ALTERNATE_SEM;
ksem_t counterSem	= COUNTER_SEM;

#ifndef TEST_PRIV_PIPES
kpipe_t pipeId	= PIPE_ID;
#else
DEFINE_PIPE(pipeId, 256);
#endif

/**
 *
 * @brief Entry point for RegressionTask
 *
 * This routine signals "task done" or "task fail", based on the return code of
 * RegressionTask.
 *
 * @return N/A
 */

void RegressionTaskEntry(void)
{
	extern int RegressionTask(void);

	int result = RegressionTask();

	task_fifo_put(RESULTQ, &result, TICKS_NONE);
}

/**
 *
 * @brief Entry point for AlternateTask
 *
 * This routine signals "task done" or "task fail", based on the return code of
 * AlternateTask.
 *
 * @return N/A
 */

void AlternateTaskEntry(void)
{
	extern int AlternateTask(void);

	int result = AlternateTask();

	task_fifo_put(RESULTQ, &result, TICKS_NONE);
}

/**
 *
 * @brief Entry point for MonitorTask
 *
 * This routine keeps tabs on the progress of the tasks doing the actual testing
 * and generates the final test case summary message.
 *
 * @return N/A
 */

void MonitorTaskEntry(void)
{
	int tasksDone;
	int result;
	int msg_value = -1;

	PRINT_DATA("Starting pipe tests\n");
	PRINT_LINE;

	/*
	 * the various test tasks start executing automatically;
	 * wait for all tasks to complete or a failure to occur,
	 * then issue the appropriate test case summary message
	 */

	for (tasksDone = 0; tasksDone < NUM_TEST_TASKS; tasksDone++) {
		result = task_fifo_get(RESULTQ, &msg_value, TIMEOUT);
		if (msg_value != TC_PASS) {
			if (result != RC_OK) {
				TC_ERROR("Monitor task timed out\n");
			}

			TC_END_RESULT(TC_FAIL);
			TC_END_REPORT(TC_FAIL);
			return;
		}
	}

	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
}
