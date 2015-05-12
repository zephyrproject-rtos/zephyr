/* main.c - test semaphore APIs (kernel version) */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

/* includes */

#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <vxmicro.h>
#include <tc_util.h>

/* test uses 1 software IRQs */
#define NUM_SW_IRQS 1

#include <irq_test_common.h>
#include <util_test_common.h>

extern void testFiberInit(void);
extern struct nano_sem fiberSem; /* semaphore that allows test control the fiber */

/* defines */

#define NUM_TEST_TASKS	4	/* # of test tasks to monitor */

/* locals */

static ksem_t testIsrInfo;

static CMD_PKT_SET_INSTANCE(cmdPktSet, 2)

/*
 * Note that semaphore group entries are arranged so that resultSems[TC_PASS]
 * refers to SEM_TASKDONE and resultSems[TC_FAIL] refers to SEM_TASKFAIL.
 */

static ksem_t resultSems[] = { SEM_TASKDONE, SEM_TASKFAIL, ENDLIST };

/* globals */

ksem_t simpleSem	= SIMPLE_SEM;
ksem_t altSem		= ALTTASK_SEM;
ksem_t hpSem		= HIGH_PRI_SEM;
ksem_t manyBlockSem	= MANY_BLOCKED_SEM;
ksem_t group1Sem	= GROUP_SEM1;
ksem_t group2Sem	= GROUP_SEM2;
ksem_t group3Sem	= GROUP_SEM3;
ksem_t group4Sem	= GROUP_SEM4;
ksem_t blockHpSem	= BLOCK_HP_SEM;
ksem_t blockMpSem	= BLOCK_MP_SEM;
ksem_t blockLpSem	= BLOCK_LP_SEM;

ksem_t semList[] = {
	GROUP_SEM1,
	GROUP_SEM2,
	GROUP_SEM3,
	GROUP_SEM4,
	ENDLIST
	};

static vvfn _trigger_isrSemaSignal = (vvfn) sw_isr_trigger_0;

/*******************************************************************************
*
* RegressionTaskEntry - entry point for RegressionTask
*
* This routine signals "task done" or "task fail", based on the return code of
* RegressionTask.
*
* RETURNS: N/A
*/

void RegressionTaskEntry(void)
{
	extern int RegressionTask(void);

	task_sem_give (resultSems[RegressionTask ()]);
}

/*******************************************************************************
*
* AlternateTaskEntry - entry point for AlternateTask
*
* This routine signals "task done" or "task fail", based on the return code of
* MsgRcvrTask.
*
* RETURNS: N/A
*/

void AlternateTaskEntry(void)
{
	extern int AlternateTask(void);

	task_sem_give (resultSems[AlternateTask ()]);
}

/*******************************************************************************
*
* HighPriTaskEntry - entry point for HighPriTask
*
* This routine signals "task done" or "task fail", based on the return code of
* HighPriTask.
*
* RETURNS: N/A
*/

void HighPriTaskEntry(void)
{
	extern int HighPriTask(void);

	task_sem_give (resultSems[HighPriTask ()]);
}

/*******************************************************************************
*
* LowPriTaskEntry - entry point for LowPriTask
*
* This routine signals "task done" or "task fail", based on the return code of
* LowPriTask.
*
* RETURNS: N/A
*/

void LowPriTaskEntry(void)
{
	extern int LowPriTask(void);

	task_sem_give (resultSems[LowPriTask ()]);
}

/*******************************************************************************
*
* testIsrHandler - ISR that gives specified semaphore
*
* \param isrData    pointer to semaphore to be given
*
* RETURNS: N/A
*/

static void testIsrHandler(void *isrData)
{
	isr_sem_give(*(ksem_t *)isrData, &CMD_PKT_SET(cmdPktSet));
}

/*******************************************************************************
*
* trigger_isrSemaSignal - generate interrupt that gives specified semaphore
*
* \param semaphore    semaphore to be given
*
* RETURNS: N/A
*/

void trigger_isrSemaSignal(ksem_t semaphore)
{
	testIsrInfo = semaphore;
	_trigger_isrSemaSignal();
}

/*******************************************************************************
*
* releaseTestFiber - release the test fiber
*
* RETURNS: N/A
*/

void releaseTestFiber(void)
{
	nano_task_sem_give(&fiberSem);
}

/*******************************************************************************
*
* testInterruptsInit - initialize interrupt-related code
*
* Binds an ISR to the interrupt vector used to give semaphores from interrupt
* level.
*
* RETURNS: N/A
*/

static void testInterruptsInit(void)
{
	struct isrInitInfo i = {
	{ testIsrHandler, NULL},
	{ &testIsrInfo, NULL},
	};

	(void) initIRQ (&i);
}

/*******************************************************************************
*
* MonitorTaskEntry - entry point for MonitorTask
*
* This routine keeps tabs on the progress of the tasks doing the actual testing
* and generates the final test case summary message.
*
* RETURNS: N/A
*/

void MonitorTaskEntry(void)
{
	ksem_t result;
	int tasksDone;

	testInterruptsInit();
	testFiberInit();

	PRINT_DATA("Starting semaphore tests\n");
	PRINT_LINE;

    /*
     * the various test tasks start executing automatically;
     * wait for all tasks to complete or a failure to occur,
     * then issue the appropriate test case summary message
     */

	for (tasksDone = 0; tasksDone < NUM_TEST_TASKS; tasksDone++) {
	result = task_sem_group_take_wait_timeout(resultSems, SECONDS(60));
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
