/* task.c - test microkernel task APIs under VxMicro */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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
This module tests the following task APIs:
    isr_task_id_get(), isr_task_priority_get(), task_id_get(), task_priority_get(),
    task_resume(), task_suspend(), task_priority_set(),
    task_sleep(), task_yield()
*/

#include <tc_util.h>
#include <vxmicro.h>
#include <arch/cpu.h>

/* test uses 1 software IRQs */
#define NUM_SW_IRQS 1

#include <irq_test_common.h>
#include <util_test_common.h>

#define  RT_PRIO         10     /* RegressionTask prio - must match prj.vpf */
#define  HT_PRIO         20     /* HelperTask prio - must match prj.vpf */

#define  SLEEP_TIME SECONDS(1)

#define  CMD_TASKID    0
#define  CMD_PRIORITY  1

typedef struct {
	int  cmd;
	int  data;
} ISR_INFO;

static vvfn _trigger_isrTaskCommand = (vvfn)sw_isr_trigger_0;

static ISR_INFO   isrInfo;

static int tcRC = TC_PASS;         /* test case return code */
static int helperData;

static volatile int mainTaskNotReady = 0;

/*******************************************************************************
*
* isr_task_command_handler - ISR handler to call isr_task_id_get() and isr_task_priority_get()
*
* RETURNS: N/A
*/

void isr_task_command_handler(void *data)
{
	ISR_INFO   *pInfo = (ISR_INFO *) data;
	int         value = -1;

	switch (pInfo->cmd) {
	case CMD_TASKID:
		value = isr_task_id_get();
		break;

	case CMD_PRIORITY:
		value = isr_task_priority_get();
		break;
	}

	pInfo->data = value;
}

/*******************************************************************************
*
* isrAPIsTest - test isr_task_id_get() and isr_task_priority_get
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int isrAPIsTest(int taskId, int taskPrio)
{
	isrInfo.cmd = CMD_TASKID;
	_trigger_isrTaskCommand();
	if (isrInfo.data != taskId) {
		TC_ERROR("isr_task_id_get() returned %d, not %d\n",
				 isrInfo.data, taskId);
		return TC_FAIL;
	}

	isrInfo.cmd = CMD_PRIORITY;
	_trigger_isrTaskCommand();
	if (isrInfo.data != taskPrio) {
		TC_ERROR("isr_task_priority_get() returned %d, not %d\n",
				 isrInfo.data, taskPrio);
		return TC_FAIL;
	}

	return TC_PASS;
}

/*******************************************************************************
*
* taskMacrosTest - test task_id_get() and task_priority_get() macros
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int taskMacrosTest(int taskId, int taskPrio)
{
	int  value;

	value = task_id_get();
	if (value != taskId) {
		TC_ERROR("task_id_get() returned 0x%x, not 0x%x\n",
				 value, taskId);
		return TC_FAIL;
	}

	value = task_priority_get();
	if (value != taskPrio) {
		TC_ERROR("task_priority_get() returned %d, not %d\n",
				 value, taskPrio);
		return TC_FAIL;
	}

	return TC_PASS;
}

/*******************************************************************************
*
* microObjectsInit - initialize objects used in this microkernel test suite
*
* RETURNS: N/A
*/

void microObjectsInit(void)
{
	struct isrInitInfo i = {
	{ isr_task_command_handler, NULL },
	{ &isrInfo, NULL },
	};

	(void) initIRQ(&i);

	TC_PRINT("Microkernel objects initialized\n");
}

/*******************************************************************************
*
* helperTaskSetPrioTest - helper task portion to test setting the priority
*
* RETURNS: N/A
*/

void helperTaskSetPrioTest(void)
{
	task_sem_take_wait(HT_SEM);
	helperData = task_priority_get();     /* Helper task priority lowered by 5 */
	task_sem_give(RT_SEM);

	task_sem_take_wait(HT_SEM);
	helperData = task_priority_get();     /* Helper task prioirty raised by 10 */
	task_sem_give(RT_SEM);

	task_sem_take_wait(HT_SEM);
	helperData = task_priority_get();     /* Helper task prioirty restored */
	task_sem_give(RT_SEM);
}

/*******************************************************************************
*
* taskSetPrioTest - test the task_priority_set() API
*
* RETURNS: N/A
*/

int taskSetPrioTest(void)
{
	int  rv;

	/* Lower the priority of the current task (RegressionTask) */
	task_priority_set(RT_TASKID, RT_PRIO + 5);
	rv = task_priority_get();
	if (rv != RT_PRIO + 5) {
		TC_ERROR("Expected priority to be changed to %d, not %d\n",
				 RT_PRIO + 5, rv);
		return TC_FAIL;
	}

	/* Raise the priority of the current task (RegressionTask) */
	task_priority_set(RT_TASKID, RT_PRIO - 5);
	rv = task_priority_get();
	if (rv != RT_PRIO - 5) {
		TC_ERROR("Expected priority to be changed to %d, not %d\n",
				 RT_PRIO - 5, rv);
		return TC_FAIL;
	}


	/* Restore the priority of the current task (RegressionTask) */
	task_priority_set(RT_TASKID, RT_PRIO);
	rv = task_priority_get();
	if (rv != RT_PRIO) {
		TC_ERROR("Expected priority to be changed to %d, not %d\n",
				 RT_PRIO, rv);
		return TC_FAIL;
	}


	/* Lower the priority of the helper task (HelperTask) */
	task_priority_set(HT_TASKID, HT_PRIO + 5);
	task_sem_give(HT_SEM);
	task_sem_take_wait(RT_SEM);
	if (helperData != HT_PRIO + 5) {
		TC_ERROR("Expected priority to be changed to %d, not %d\n",
				 HT_PRIO + 5, helperData);
		return TC_FAIL;
	}

	/* Raise the priority of the helper task (HelperTask) */
	task_priority_set(HT_TASKID, HT_PRIO - 5);
	task_sem_give(HT_SEM);
	task_sem_take_wait(RT_SEM);
	if (helperData != HT_PRIO - 5) {
		TC_ERROR("Expected priority to be changed to %d, not %d\n",
				 HT_PRIO - 5, helperData);
		return TC_FAIL;
	}


	/* Restore the priority of the helper task (HelperTask) */
	task_priority_set(HT_TASKID, HT_PRIO);
	task_sem_give(HT_SEM);
	task_sem_take_wait(RT_SEM);
	if (helperData != HT_PRIO) {
		TC_ERROR("Expected priority to be changed to %d, not %d\n",
				 HT_PRIO, helperData);
		return TC_FAIL;
	}

	return TC_PASS;
}

/*******************************************************************************
*
* helperTaskSleepTest - helper task portion to test task_sleep()
*
* RETURNS: N/A
*/

void helperTaskSleepTest(void)
{
	int32_t  firstTick;

	task_sem_take_wait(HT_SEM);

	firstTick = task_tick_get_32();
	while (mainTaskNotReady) {
	}
	helperData = task_tick_get_32() - firstTick;

	task_sem_give(RT_SEM);
}

/*******************************************************************************
*
* taskSleepTest - test task_sleep()
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int taskSleepTest(void)
{
	int32_t  tick;

	tick = task_tick_get_32();           /* Busy wait to align */
	while (tick == task_tick_get_32()) {   /* to tick boundary */
	}

	task_sem_give(HT_SEM);

	mainTaskNotReady = 1;
	task_sleep(SLEEP_TIME);
	mainTaskNotReady = 0;
	task_sem_take_wait(RT_SEM);

	if (helperData != SLEEP_TIME) {
		TC_ERROR("task_sleep() slept for %d ticks, not %d\n",
				 helperData, SLEEP_TIME);
		return TC_FAIL;
	}

	return TC_PASS;
}

/*******************************************************************************
*
* helperTaskYieldTest - helper task portion of task_yield() test
*
* RETURNS: N/A
*/

void helperTaskYieldTest(void)
{
	int  i;
	task_sem_take_wait(HT_SEM);

	for (i = 0; i < 5; i++) {
		helperData++;
		task_yield();
	}

	task_sem_give(RT_SEM);
}

/*******************************************************************************
*
* taskYieldTest - test task_yield()
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int taskYieldTest(void)
{
	int  prevHelperData;
	int  i;

	helperData = 0;

	/* 1st raise the priority of the helper task */
	task_priority_set(HT_TASKID, RT_PRIO);
	task_sem_give(HT_SEM);

	for (i = 0; i < 5; i++) {
		prevHelperData = helperData;
		task_yield();

		if (helperData == prevHelperData) {
			TC_ERROR("Iter %d.  helperData did not change (%d)\n",
					 i + 1, helperData);
			return TC_FAIL;
		}
	}

	/* Restore helper task priority */
	task_priority_set(HT_TASKID, HT_PRIO);

	/* Ensure that the helper task finishes */
	task_sem_take_wait(RT_SEM);

	return TC_PASS;
}

/*******************************************************************************
*
* helperTaskSuspendTest - helper task portion of task_suspend() and
*                         task_resume() tests
*
* RETURNS: N/A
*/

void helperTaskSuspendTest(void)
{
	helperData++;

	task_sem_take_wait(HT_SEM);
}

/*******************************************************************************
*
* taskSuspendTest - test task_suspend() and task_resume()
*
* This test suspends the helper task.  Once it is suspended, the main task
* (RegressionTask) sleeps for one second.  If the helper task is truly
* suspended, it will not execute and modify <helperData>.  Once confirmed,
* the helper task is resumed, and the main task sleeps once more.  If the
* helper task has truly resumed, it will modify <helperData>.
*
* RETURNS: TC_PASS on success or TC_FAIL on failure
*/

int taskSuspendTest(void)
{
	int  prevHelperData;

	task_suspend(HT_TASKID);    /* Suspend the helper task */

	prevHelperData = helperData;
	task_sleep(SLEEP_TIME);

	if (prevHelperData != helperData) {
		TC_ERROR("Helper task did not suspend!\n");
		return TC_FAIL;
	}

	task_resume(HT_TASKID);
	task_sleep(SLEEP_TIME);

	if (prevHelperData == helperData) {
		TC_ERROR("Helper task did not resume!\n");
		return TC_FAIL;
	}

	task_sem_give(HT_SEM);
	return TC_PASS;
}

/*******************************************************************************
*
* HelperTask - helper task to test the task APIs
*
* RETURNS:  N/A
*/

void HelperTask(void)
{
	int  rv;

	task_sem_take_wait(HT_SEM);
	rv = isrAPIsTest(HT_TASKID, HT_PRIO);
	if (rv != TC_PASS) {
		tcRC = TC_FAIL;
		return;
	}
	task_sem_give(RT_SEM);

	task_sem_take_wait(HT_SEM);
	rv = taskMacrosTest(HT_TASKID, HT_PRIO);
	if (rv != TC_PASS) {
		tcRC = TC_FAIL;
		return;
	}
	task_sem_give(RT_SEM);

	helperTaskSetPrioTest();

	helperTaskSleepTest();

	helperTaskYieldTest();

	helperTaskSuspendTest();
}

/*******************************************************************************
*
* RegressionTask - main task to test the task APIs
*
* RETURNS:  N/A
*/

void RegressionTask(void)
{
	int    rv;

	TC_START("Test Microkernel Task API");

	PRINT_LINE;

	microObjectsInit();

	TC_PRINT("Testing isr_task_id_get() and isr_task_priority_get()\n");
	rv = isrAPIsTest(RT_TASKID, RT_PRIO);
	if (rv != TC_PASS) {
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	task_sem_give(HT_SEM);
	task_sem_take_wait(RT_SEM);

	TC_PRINT("Testing task_id_get() and task_priority_get()\n");
	rv = taskMacrosTest(RT_TASKID, RT_PRIO);
	if (rv != TC_PASS) {
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	task_sem_give(HT_SEM);
	task_sem_take_wait(RT_SEM);

	TC_PRINT("Testing task_priority_set()\n");
	if (taskSetPrioTest() != TC_PASS) {
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	TC_PRINT("Testing task_sleep()\n");
	if (taskSleepTest() != TC_PASS) {
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	TC_PRINT("Testing task_yield()\n");
	if (taskYieldTest() != TC_PASS) {
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	TC_PRINT("Testing task_suspend() and task_resume()\n");
	if (taskSuspendTest() != TC_PASS) {
		tcRC = TC_FAIL;
		goto errorReturn;
	}

errorReturn:
	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}  /* RegressionTask */
