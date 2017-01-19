/* main.c - test access to standard libraries */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
DESCRIPTION
This module contains the entry points for the tasks used by the standard
libraries test application.

Each test task entry point invokes a test routine that returns a success/failure
indication, then gives a corresponding semaphore. An additional task monitors
these semaphores until it detects a failure or the completion of all test tasks,
then announces the result of the test.

NOTE: At present only a single test task is used, but more tasks may be added
in the future to enhance test coverage.
 */

#include <tc_util.h>
#include <zephyr.h>

#include <util_test_common.h>

#define NUM_TEST_TASKS	1	/* # of test tasks to monitor */

/* # ticks to wait for test completion */
#define TIMEOUT	(60 * sys_clock_ticks_per_sec)

/*
 * Note that semaphore group entries are arranged so that resultSems[TC_PASS]
 * refers to SEM_TASKDONE and resultSems[TC_FAIL] refers to SEM_TASKFAIL.
 */

static ksem_t resultSems[] = { SEM_TASKDONE, SEM_TASKFAIL, ENDLIST };

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

	task_sem_give(resultSems[RegressionTask()]);
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
	ksem_t result;
	int tasksDone;

	PRINT_DATA("Starting standard libraries tests\n");
	PRINT_LINE;

	/*
	 * the various test tasks start executing automatically;
	 * wait for all tasks to complete or a failure to occur,
	 * then issue the appropriate test case summary message
	 */

	for (tasksDone = 0; tasksDone < NUM_TEST_TASKS; tasksDone++) {
		result = task_sem_group_take(resultSems, TIMEOUT);
		if (result != resultSems[TC_PASS]) {
			if (result != resultSems[TC_FAIL]) {
				TC_ERROR("Monitor task timed out\n");
			}
			TC_END_REPORT(TC_FAIL);
			return;
		}
	}

	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
}
