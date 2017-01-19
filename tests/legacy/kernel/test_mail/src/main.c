/* main.c - test mailbox APIs (kernel version) */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
DESCRIPTION
This module contains the entry points for the tasks used by the kernel version
of the mailbox test application. It also initializes global variables that
identify the various kernel objects used by the test code.

Each test task entry point invokes a test routine that returns a success/failure
indication, then gives a corresponding semaphore. An additional task monitors
these semaphores until it detects a failure or the completion of all test tasks,
then announces the result of the test.
 */

#include <tc_util.h>
#include <zephyr.h>

#define NUM_TEST_TASKS	2	/* # of test tasks to monitor */

/* # ticks to wait for test completion */
#define TIMEOUT	(60 * sys_clock_ticks_per_sec)

/*
 * Note that semaphore group entries are arranged so that resultSems[TC_PASS]
 * refers to SEM_TASKDONE and resultSems[TC_FAIL] refers to SEM_TASKFAIL.
 */

static ksem_t resultSems[] = { SEM_TASKDONE, SEM_TASKFAIL, ENDLIST };

ktask_t msgSenderTask	= MSGSENDERTASK;
ktask_t msgRcvrTask		= MSGRCVRTASK;

ksem_t semSync1			= SEM_SYNC1;
ksem_t semSync2			= SEM_SYNC2;

#ifndef TEST_PRIV_MBX
kmbox_t myMbox			= MYMBOX;
kmbox_t noRcvrMbox		= NORCVRMBOX;
#else
extern const kmbox_t myMbox;
extern const kmbox_t noRcvrMbox;
#endif

kmemory_pool_t testPool			= TESTPOOL;
kmemory_pool_t smallBlkszPool	= SMALLBLKSZPOOL;

/**
 *
 * @brief Entry point for MsgSenderTask
 *
 * This routine signals "task done" or "task fail", based on the return code of
 * MsgSenderTask.
 *
 * @return N/A
 */

void MsgSenderTaskEntry(void)
{
	extern int MsgSenderTask(void);

	task_sem_give(resultSems[MsgSenderTask()]);
}

/**
 *
 * @brief Entry point for MsgRcvrTask
 *
 * This routine signals "task done" or "task fail", based on the return code of
 * MsgRcvrTask.
 *
 * @return N/A
 */

void MsgRcvrTaskEntry(void)
{
	extern int MsgRcvrTask(void);

	task_sem_give(resultSems[MsgRcvrTask()]);
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

	PRINT_DATA("Starting mailbox tests\n");
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
			TC_END_RESULT(TC_FAIL);
			TC_END_REPORT(TC_FAIL);
			return;
		}
	}

	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
}
