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

/**
 * @file
 * @brief Test microkernel event APIs
 *
 * This modules tests the following event APIs:
 *    task_event_handler_set()
 *    task_event_send()
 *    isr_event_send()
 *    task_event_recv()
 */

#include <tc_util.h>
#include <zephyr.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <irq_offload.h>

#include <util_test_common.h>

typedef struct {
	kevent_t  event;
} ISR_INFO;

static int  evidence = 0;

static ISR_INFO  isrInfo;
static int  handlerRetVal = 0;

extern void testFiberInit(void);
extern struct nano_sem fiberSem; /* semaphore that allows test control the fiber */

extern kevent_t _k_event_list_end[];

/**
 *
 * @brief ISR handler to signal an event
 *
 * @return N/A
 */

void isr_event_signal_handler(void *data)
{
	ISR_INFO *pInfo = (ISR_INFO *) data;

	isr_event_send(pInfo->event);
}

static void _trigger_isrEventSignal(void)
{
	irq_offload(isr_event_signal_handler, &isrInfo);
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
 * @brief Initialize objects used in this microkernel test suite
 *
 * @return N/A
 */

void microObjectsInit(void)
{
	testFiberInit();

	TC_PRINT("Microkernel objects initialized\n");
}

/**
 *
 * @brief Test the task_event_recv(TICKS_NONE) API
 *
 * There are two cases to be tested here.  The first is for testing for an
 * event when there is one.  The second is for testing for an event when there
 * are none.  Note that the "consumption" of the event gets confirmed by the
 * order in which the latter two checks are done.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int eventNoWaitTest(void)
{
	int  rv;    /* return value from task_event_xxx() calls */

	/* Signal an event */
	rv = task_event_send(EVENT_ID);
	if (rv != RC_OK) {
		TC_ERROR("task_event_send() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	rv = task_event_recv(EVENT_ID, TICKS_NONE);
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	/* No event has been signalled */
	rv = task_event_recv(EVENT_ID, TICKS_NONE);
	if (rv != RC_FAIL) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_FAIL);
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Test the task_event_recv(TICKS_UNLIMITED) API
 *
 * This test checks task_event_recv(TICKS_UNLIMITED) against the following
 * cases:
 *  1. There is already an event waiting (signalled from a task and ISR).
 *  2. The current task must wait on the event until it is signalled
 *     from either another task, an ISR or a fiber.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int eventWaitTest(void)
{
	int  rv;     /* return value from task_event_xxx() calls */
	int  i;      /* loop counter */

	/*
	 * task_event_recv() to return immediately as there will already be
	 * an event by a task.
	 */

	task_event_send(EVENT_ID);
	rv = task_event_recv(EVENT_ID, TICKS_UNLIMITED);
	if (rv != RC_OK) {
		TC_ERROR("Task: task_event_recv() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	/*
	 * task_event_recv() to return immediately as there will already be
	 * an event made ready by an ISR.
	 */

	isrInfo.event = EVENT_ID;
	_trigger_isrEventSignal();
	rv = task_event_recv(EVENT_ID, TICKS_UNLIMITED);
	if (rv != RC_OK) {
		TC_ERROR("ISR: task_event_recv() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	/*
	 * task_event_recv() to return immediately as there will already be
	 * an event made ready by a fiber.
	 */

	releaseTestFiber();
	rv = task_event_recv(EVENT_ID, TICKS_UNLIMITED);
	if (rv != RC_OK) {
		TC_ERROR("Fiber: task_event_recv() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	task_sem_give(ALTERNATE_SEM);    /* Wake the AlternateTask */

	/*
	 * The 1st pass, task_event_recv() will be signalled from a task,
	 * from an ISR for the second and from a fiber third.
	 */

	for (i = 0; i < 3; i++) {
		rv = task_event_recv(EVENT_ID, TICKS_UNLIMITED);
		if (rv != RC_OK) {
			TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_OK);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * @brief Test the task_event_recv(timeout) API
 *
 * This test checks task_event_recv(timeout) against the following cases:
 *  1. The current task times out while waiting for the event.
 *  2. There is already an event waiting (signalled from a task).
 *  3. The current task must wait on the event until it is signalled
 *     from either another task, an ISR or a fiber.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int eventTimeoutTest(void)
{
	int  rv;     /* return value from task_event_xxx() calls */
	int  i;      /* loop counter */

	/* Timeout while waiting for the event */
	rv = task_event_recv(EVENT_ID, MSEC(100));
	if (rv != RC_TIME) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_TIME);
		return TC_FAIL;
	}

	/* Let there be an event already waiting to be tested */
	task_event_send(EVENT_ID);
	rv = task_event_recv(EVENT_ID, MSEC(100));
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	task_sem_give(ALTERNATE_SEM);    /* Wake AlternateTask() */

	/*
	 * The 1st pass, task_event_recv(timeout) will be signalled from a task,
	 * from an ISR for the second and from a fiber for the third.
	 */

	for (i = 0; i < 3; i++) {
		rv = task_event_recv(EVENT_ID, MSEC(100));
		if (rv != RC_OK) {
			TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_OK);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * @brief Test the isr_event_send() API
 *
 * Although other tests have done some testing using isr_event_send(), none
 * of them have demonstrated that signalling an event more than once does not
 * "queue" events.  That is, should two or more signals of the same event occur
 * before it is tested, it can only be tested for successfully once.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int isrEventSignalTest(void)
{
	int  rv;    /* return value from task_event_recv() */

	/*
	 * The single case of an event made ready has already been tested.
	 * Trigger two ISR event signals.  Only one should be detected.
	 */

	isrInfo.event = EVENT_ID;

	_trigger_isrEventSignal();
	_trigger_isrEventSignal();

	rv = task_event_recv(EVENT_ID, TICKS_NONE);
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	/* The second event signal should be "lost" */
	rv = task_event_recv(EVENT_ID, TICKS_NONE);
	if (rv != RC_FAIL) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_FAIL);
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Test the fiber_event_send() API
 *
 * Signalling an event by fiber_event_send() more than once does not "queue"
 * events.  That is, should two or more signals of the same event occur before
 * it is tested, it can only be tested for successfully once.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int fiberEventSignalTest(void)
{
	int  rv;    /* return value from task_event_recv(TICKS_NONE) */

	/*
	 * Trigger two fiber event signals.  Only one should be detected.
	 */

	releaseTestFiber();

	rv = task_event_recv(EVENT_ID, TICKS_NONE);
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	/* The second event signal should be "lost" */
	rv = task_event_recv(EVENT_ID, TICKS_NONE);
	if (rv != RC_FAIL) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_FAIL);
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Handler to run on EVENT_ID event
 *
 * @param event    signalled event
 *
 * @return <handlerRetVal>
 */

int eventHandler(int event)
{
	ARG_UNUSED(event);

	evidence++;

	return handlerRetVal;    /* 0 if not to wake waiting task; 1 if to wake */
}

/**
 *
 * @brief Handler to run on ALT_EVENT event
 *
 * @param event    signalled event
 *
 * @return 1
 */

int altEventHandler(int event)
{
	ARG_UNUSED(event);

	evidence += 100;

	return 1;
}

/**
 *
 * @brief Test the task_event_handler_set() API
 *
 * This test checks that the event handler is set up properly when
 * task_event_handler_set() is called.  It shows that event handlers are tied
 * to the specified event and that the return value from the handler affects
 * whether the event wakes a task waiting upon that event.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int eventSignalHandlerTest(void)
{
	int  rv;     /* return value from task_event_xxx() calls */

	/*
	 * NOTE: We cannot test for the validity of an event ID, since
	 * task_event_handler_set() only checks for valid event IDs via an
	 * __ASSERT() and only in debug kernels (an __ASSERT() stops the system).
	 */

	/* Expect this call to task_event_handler_set() to succeed */
	rv = task_event_handler_set(EVENT_ID, eventHandler);
	if (rv != RC_OK) {
		TC_ERROR("task_event_handler_set() returned %d not %d\n",
			rv, RC_OK);
		return TC_FAIL;
	}

	/* Enable another handler to show that two handlers can be installed */
	rv = task_event_handler_set(ALT_EVENT, altEventHandler);
	if (rv != RC_OK) {
		TC_ERROR("task_event_handler_set() returned %d not %d\n",
			rv, RC_OK);
		return TC_FAIL;
	}

	/*
	 * The alternate task should signal the event, but the handler will
	 * return 0 and the waiting task will not be woken up.  Thus, it should
	 * timeout and get an RC_TIME return code.
	 */

	task_sem_give(ALTERNATE_SEM);    /* Wake alternate task */
	rv = task_event_recv(EVENT_ID, MSEC(100));
	if (rv != RC_TIME) {
		TC_ERROR("task_event_recv() returned %d not %d\n", rv, RC_TIME);
		return TC_FAIL;
	}

	/*
	 * The alternate task should signal the event, and the handler will
	 * return 1 this time, which will wake the waiting task.
	 */

	task_sem_give(ALTERNATE_SEM);    /* Wake alternate task again */
	rv = task_event_recv(EVENT_ID, MSEC(100));
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv() returned %d not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	if (evidence != 2) {
		TC_ERROR("Expected event handler evidence to be %d not %d\n",
			2, evidence);
		return TC_FAIL;
	}

	/*
	 * Signal the alternate event.  This demonstrates that two event handlers
	 * can be simultaneously installed for two different events.
	 */

	task_event_send(ALT_EVENT);
	if (evidence != 102) {
		TC_ERROR("Expected event handler evidence to be %d not %d\n",
			2, evidence);
		return TC_FAIL;
	}

	/* Uninstall the event handlers */
	rv = task_event_handler_set(EVENT_ID, NULL);
	if (rv != RC_OK) {
		TC_ERROR("task_event_handler_set() returned %d not %d\n",
			rv, RC_OK);
		return TC_FAIL;
	}

	rv = task_event_handler_set(ALT_EVENT, NULL);
	if (rv != RC_OK) {
		TC_ERROR("task_event_handler_set() returned %d not %d\n",
			rv, RC_OK);
		return TC_FAIL;
	}

	task_event_send(EVENT_ID);
	task_event_send(ALT_EVENT);

	if (evidence != 102) {
		TC_ERROR("Event handlers did not uninstall\n");
		return TC_FAIL;
	}

	/* Clear out the waiting events */

	rv = task_event_recv(EVENT_ID, TICKS_NONE);
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv() returned %d not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	rv = task_event_recv(ALT_EVENT, TICKS_NONE);
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv() returned %d not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Alternate task to signal various events to a waiting task
 *
 * @return N/A
 */

void AlternateTask(void)
{
	/* Wait for eventWaitTest() to run. */
	task_sem_take(ALTERNATE_SEM, TICKS_UNLIMITED);
	task_event_send(EVENT_ID);
	releaseTestFiber();
	_trigger_isrEventSignal();

	/* Wait for eventTimeoutTest() to run. */
	task_sem_take(ALTERNATE_SEM, TICKS_UNLIMITED);
	task_event_send(EVENT_ID);
	releaseTestFiber();
	_trigger_isrEventSignal();

	/*
	 * Wait for eventSignalHandlerTest() to run.
	 *
	 * When <handlerRetVal> is zero (0), the waiting task will not get woken up
	 * after the event handler for EVENT_ID runs.  When it is one (1), the
	 * waiting task will get woken up after the event handler for EVENT_ID runs.
	 */

	task_sem_take(ALTERNATE_SEM, TICKS_UNLIMITED);
	handlerRetVal = 0;
	task_event_send(EVENT_ID);

	task_sem_take(ALTERNATE_SEM, TICKS_UNLIMITED);
	handlerRetVal = 1;
	task_event_send(EVENT_ID);
}

/**
 *
 * @brief Main entry point to the test suite
 *
 * @return N/A
 */

void RegressionTask(void)
{
	int  tcRC;    /* test case return code */

	TC_START("Test Microkernel Events\n");

	microObjectsInit();

	TC_PRINT("Testing task_event_recv(TICKS_NONE) and task_event_send() ...\n");
	tcRC = eventNoWaitTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Testing task_event_recv(TICKS_UNLIMITED) and task_event_send() ...\n");
	tcRC = eventWaitTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Testing task_event_recv(timeout) and task_event_send() ...\n");
	tcRC = eventTimeoutTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Testing isr_event_send() ...\n");
	tcRC = isrEventSignalTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Testing fiber_event_send() ...\n");
	tcRC = fiberEventSignalTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Testing task_event_handler_set() ...\n");
	tcRC = eventSignalHandlerTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

doneTests:
	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}
