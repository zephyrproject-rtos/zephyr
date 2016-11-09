/* main.c - test semaphore APIs (kernel version) */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
DESCRIPTION
This module contains the entry points for the tasks used by the kernel version
of the semaphore test application. It also initializes global variables that
identify the various kernel objects used by the test code.

Each test task entry point invokes a test routine that returns a success/failure
indication, then gives a corresponding semaphore. An additional task monitors
these semaphores until it detects a failure or the completion of all test tasks,
then announces the result of the test.
 */

#include <zephyr.h>
#include <irq_offload.h>
#include <tc_util.h>

#include <util_test_common.h>

extern void testFiberInit(void);
extern struct nano_sem fiberSem; /* semaphore that allows test control the fiber */

#define NUM_TEST_TASKS	4	/* # of test tasks to monitor */

static ksem_t testIsrInfo;

/*
 * Note that semaphore group entries are arranged so that resultSems[TC_PASS]
 * refers to SEM_TASKDONE and resultSems[TC_FAIL] refers to SEM_TASKFAIL.
 */

static ksem_t resultSems[] = { SEM_TASKDONE, SEM_TASKFAIL, ENDLIST };

ksem_t group1Sem	= GROUP_SEM1;
ksem_t group2Sem	= GROUP_SEM2;
ksem_t group3Sem	= GROUP_SEM3;
ksem_t group4Sem	= GROUP_SEM4;

#ifdef TEST_PRIV_KSEM
	DEFINE_SEMAPHORE(simpleSem);
	DEFINE_SEMAPHORE(altSem);
	DEFINE_SEMAPHORE(hpSem);
	DEFINE_SEMAPHORE(manyBlockSem);
	DEFINE_SEMAPHORE(blockHpSem);
	DEFINE_SEMAPHORE(blockMpSem);
	DEFINE_SEMAPHORE(blockLpSem);
#else
	ksem_t simpleSem	= SIMPLE_SEM;
	ksem_t altSem		= ALTTASK_SEM;
	ksem_t hpSem		= HIGH_PRI_SEM;
	ksem_t manyBlockSem	= MANY_BLOCKED_SEM;
	ksem_t blockHpSem	= BLOCK_HP_SEM;
	ksem_t blockMpSem	= BLOCK_MP_SEM;
	ksem_t blockLpSem	= BLOCK_LP_SEM;
#endif

ksem_t semList[] = {
	GROUP_SEM1,
	GROUP_SEM2,
	GROUP_SEM3,
	GROUP_SEM4,
	ENDLIST
	};

/**
 *
 * @brief ISR that gives specified semaphore
 *
 * @param isrData    pointer to semaphore to be given
 *
 * @return N/A
 */

static void testIsrHandler(void *isrData)
{
	isr_sem_give(*(ksem_t *)isrData);
}

static void _trigger_isrSemaSignal(void)
{
	irq_offload(testIsrHandler, &testIsrInfo);
}

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
 * @brief Entry point for AlternateTask
 *
 * This routine signals "task done" or "task fail", based on the return code of
 * MsgRcvrTask.
 *
 * @return N/A
 */

void AlternateTaskEntry(void)
{
	extern int AlternateTask(void);

	task_sem_give(resultSems[AlternateTask()]);
}

/**
 *
 * @brief Entry point for HighPriTask
 *
 * This routine signals "task done" or "task fail", based on the return code of
 * HighPriTask.
 *
 * @return N/A
 */

void HighPriTaskEntry(void)
{
	extern int HighPriTask(void);

	task_sem_give(resultSems[HighPriTask()]);
}

/**
 *
 * @brief Entry point for LowPriTask
 *
 * This routine signals "task done" or "task fail", based on the return code of
 * LowPriTask.
 *
 * @return N/A
 */

void LowPriTaskEntry(void)
{
	extern int LowPriTask(void);

	task_sem_give(resultSems[LowPriTask()]);
}


/**
 *
 * @brief Generate interrupt that gives specified semaphore
 *
 * @param semaphore    semaphore to be given
 *
 * @return N/A
 */

void trigger_isrSemaSignal(ksem_t semaphore)
{
	testIsrInfo = semaphore;
	_trigger_isrSemaSignal();
}

/**
 *
 * @brief Release the test fiber
 *
 * @return N/A
 */

void releaseTestFiber(void)
{
	nano_task_sem_give(&fiberSem);
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

	testFiberInit();

	PRINT_DATA("Starting semaphore tests\n");
	PRINT_LINE;

	task_group_start(TESTGROUP);

	/*
	 * the various test tasks start executing automatically;
	 * wait for all tasks to complete or a failure to occur,
	 * then issue the appropriate test case summary message
	 */

	for (tasksDone = 0; tasksDone < NUM_TEST_TASKS; tasksDone++) {
		result = task_sem_group_take(resultSems, SECONDS(60));
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
