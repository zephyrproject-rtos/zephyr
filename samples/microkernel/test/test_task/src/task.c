/* task.c - test microkernel task APIs */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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
This module tests the following task APIs:
    isr_task_id_get(), isr_task_priority_get(), task_id_get(), task_priority_get(),
    task_resume(), task_suspend(), task_priority_set(),
    task_sleep(), task_yield()
 */

#include <tc_util.h>
#include <zephyr.h>
#include <arch/cpu.h>

/* test uses 1 software IRQs */
#define NUM_SW_IRQS 1

#include <irq_test_common.h>
#include <util_test_common.h>

#define  RT_PRIO         10     /* RegressionTask prio - must match prj.mdef */
#define  HT_PRIO         20     /* HelperTask prio - must match prj.mdef */

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

static volatile int is_main_task_ready = 0;

#ifdef TEST_PRIV_TASKS
/* Note this is in reverse order of what is defined under
 * test_task/prj.mdef. This is due to compiler filling linker
 * section like a stack. This is to preserve the same order
 * in memory as test_task.
 */
DEFINE_TASK(RT_TASKID, 10, RegressionTask, 2048, EXE);
DEFINE_TASK(HT_TASKID, 20, HelperTask, 2048, EXE);
#endif

/**
 *
 * @brief ISR handler to call isr_task_id_get() and isr_task_priority_get()
 *
 * @return N/A
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

/**
 *
 * @brief Test isr_task_id_get() and isr_task_priority_get
 *
 * @return TC_PASS on success, TC_FAIL on failure
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

/**
 *
 * @brief Test task_id_get() and task_priority_get() macros
 *
 * @return TC_PASS on success, TC_FAIL on failure
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

/**
 *
 * @brief Initialize objects used in this microkernel test suite
 *
 * @return N/A
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

/**
 *
 * @brief Helper task portion to test setting the priority
 *
 * @return N/A
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

/**
 *
 * @brief Test the task_priority_set() API
 *
 * @return N/A
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

/**
 *
 * @brief Helper task portion to test task_sleep()
 *
 * @return N/A
 */

void helperTaskSleepTest(void)
{
	int32_t  firstTick;

	task_sem_take_wait(HT_SEM);

	firstTick = task_tick_get_32();
	while (!is_main_task_ready) {
		/* busy work */
	}
	helperData = task_tick_get_32() - firstTick;

	task_sem_give(RT_SEM);
}

/**
 *
 * @brief Test task_sleep()
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int taskSleepTest(void)
{
	int32_t  tick;

	task_sem_give(HT_SEM);

	/* align on tick boundary and get current tick */
	tick = task_tick_get_32();
	while (tick == task_tick_get_32()) {
	}

	/* compensate for the extra tick we just waited */
	++tick;

	task_sleep(SLEEP_TIME);

	tick = task_tick_get_32() - tick;

	is_main_task_ready = 1;
	task_sem_take_wait(RT_SEM);

	if (tick != SLEEP_TIME) {
		TC_ERROR("task_sleep() slept for %d ticks, not %d\n", tick, SLEEP_TIME);
		return TC_FAIL;
	}

	/*
	 * Check that the helper task ran for approximately SLEEP_TIME. On QEMU,
	 * when the host CPU is overloaded, it has been observed that the tick
	 * count can be missed by 1 on either side. Allow for 2 ticks to be sure.
	 * This check is only there to make sure that the helper task did run for
	 * approximately the whole time the main task was sleeping.
	 */
	const int tick_error_allowed = 2;
	if (helperData > SLEEP_TIME + tick_error_allowed ||
		helperData < SLEEP_TIME - tick_error_allowed) {
		TC_ERROR("helper task should have run for around %d ticks "
				 "(+/-%d), but ran for %d ticks\n",
					SLEEP_TIME, tick_error_allowed, helperData);
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Helper task portion of task_yield() test
 *
 * @return N/A
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

/**
 *
 * @brief Test task_yield()
 *
 * @return TC_PASS on success, TC_FAIL on failure
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

/**
 *
 * @brief Helper task portion of task_suspend() and
 *                         task_resume() tests
 *
 * @return N/A
 */

void helperTaskSuspendTest(void)
{
	helperData++;

	task_sem_take_wait(HT_SEM);
}

/**
 *
 * @brief Test task_suspend() and task_resume()
 *
 * This test suspends the helper task.  Once it is suspended, the main task
 * (RegressionTask) sleeps for one second.  If the helper task is truly
 * suspended, it will not execute and modify <helperData>.  Once confirmed,
 * the helper task is resumed, and the main task sleeps once more.  If the
 * helper task has truly resumed, it will modify <helperData>.
 *
 * @return TC_PASS on success or TC_FAIL on failure
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

/**
 *
 * @brief Helper task to test the task APIs
 *
 * @return  N/A
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

/**
 *
 * @brief Main task to test the task APIs
 *
 * @return  N/A
 */

void RegressionTask(void)
{
	int    rv;

	TC_START("Test Microkernel Task API");

	PRINT_LINE;

	microObjectsInit();
	task_start(HT_TASKID);

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
