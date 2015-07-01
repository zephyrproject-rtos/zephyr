/* events.c - test microkernel event APIs */

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
This modules tests the following event APIs:
  task_event_set_handler(), task_event_send(), isr_event_send(),
  task_event_recv(), task_event_recv_wait(), task_event_recv_wait_timeout()

 */

#include <tc_util.h>
#include <zephyr.h>
#include <arch/cpu.h>
#include <toolchain.h>

/* test uses 1 software IRQs */
#define NUM_SW_IRQS 1

#include <irq_test_common.h>
#include <util_test_common.h>

typedef struct {
	kevent_t  event;
} ISR_INFO;

static int  evidence = 0;

static ISR_INFO  isrInfo;

static void (*_trigger_isrEventSignal)(void) = (vvfn)sw_isr_trigger_0;
static int  handlerRetVal = 0;

extern void testFiberInit(void);
extern struct nano_sem fiberSem; /* semaphore that allows test control the fiber */

extern const int _k_num_events; /* non-public microkernel global variable */

/**
 *
 * isr_event_signal_handler - ISR handler to signal an event
 *
 * RETURNS: N/A
 */

void isr_event_signal_handler(void *data)
{
	ISR_INFO *pInfo = (ISR_INFO *) data;

	isr_event_send(pInfo->event);
}

/**
 *
 * releaseTestFiber - release the test fiber
 *
 * RETURNS: N/A
 */

void releaseTestFiber(void)
{
	nano_task_sem_give(&fiberSem);
}

/**
 *
 * microObjectsInit - initialize objects used in this microkernel test suite
 *
 * RETURNS: N/A
 */

void microObjectsInit(void)
{
	struct isrInitInfo i = {
		{isr_event_signal_handler, NULL},
		{&isrInfo, NULL},
	};

	(void) initIRQ(&i);
	testFiberInit();

	TC_PRINT("Microkernel objects initialized\n");
}

/**
 *
 * eventNoWaitTest - test the task_event_recv() API
 *
 * There are three cases to be tested here.  The first is for testing an invalid
 * event.  The second is for testing for an event when there is one.  The third
 * is for testing for an event when there are none.  Note that the "consumption"
 * of the event gets confirmed by the order in which the latter two checks are
 * done.
 *
 * RETURNS: TC_PASS on success, TC_FAIL on failure
 */

int eventNoWaitTest(void)
{
	int  rv;    /* return value from task_event_xxx() calls */

	rv = task_event_send(_k_num_events);   /* An invalid event # */
	if (rv != RC_FAIL) {
		TC_ERROR("task_event_send() returned %d, not %d\n", rv, RC_FAIL);
		return TC_FAIL;
	}

	/* Signal an event */
	rv = task_event_send(EVENT_ID);
	if (rv != RC_OK) {
		TC_ERROR("task_event_send() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	rv = task_event_recv(EVENT_ID);
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	/* No event has been signalled */
	rv = task_event_recv(EVENT_ID);
	if (rv != RC_FAIL) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_FAIL);
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * eventWaitTest - test the task_event_recv_wait() API
 *
 * This test checks task_event_recv_wait() against the following cases:
 *  1. There is already an event waiting (signalled from a task and ISR).
 *  2. The current task must wait on the event until it is signalled
 *     from either another task, an ISR or a fiber.
 *
 * RETURNS: TC_PASS on success, TC_FAIL on failure
 */

int eventWaitTest(void)
{
	int  rv;     /* return value from task_event_xxx() calls */
	int  i;      /* loop counter */

	/*
	 * task_event_recv_wait() to return immediately as there will already be
	 * an event by a task.
	 */

	task_event_send(EVENT_ID);
	rv = task_event_recv_wait(EVENT_ID);
	if (rv != RC_OK) {
		TC_ERROR("Task: task_event_recv_wait() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	/*
	 * task_event_recv_wait() to return immediately as there will already be
	 * an event made ready by an ISR.
	 */

	isrInfo.event = EVENT_ID;
	_trigger_isrEventSignal();
	rv = task_event_recv_wait(EVENT_ID);
	if (rv != RC_OK) {
		TC_ERROR("ISR: task_event_recv_wait() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	/*
	 * task_event_recv_wait() to return immediately as there will already be
	 * an event made ready by a fiber.
	 */

	releaseTestFiber();
	rv = task_event_recv_wait(EVENT_ID);
	if (rv != RC_OK) {
		TC_ERROR("Fiber: task_event_recv_wait() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	task_sem_give(ALTERNATE_SEM);    /* Wake the AlternateTask */

	/*
	 * The 1st pass, task_event_recv_wait() will be signalled from a task,
	 * from an ISR for the second and from a fiber third.
	 */

	for (i = 0; i < 3; i++) {
		rv = task_event_recv_wait(EVENT_ID);
		if (rv != RC_OK) {
			TC_ERROR("task_event_recv_wait() returned %d, not %d\n", rv, RC_OK);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * eventTimeoutTest - test the task_event_recv_wait_timeout() API
 *
 * This test checks task_event_recv_wait_timeout() against the following cases:
 *  1. The current task times out while waiting for the event.
 *  2. There is already an event waiting (signalled from a task).
 *  3. The current task must wait on the event until it is signalled
 *     from either another task, an ISR or a fiber.
 *
 * RETURNS: TC_PASS on success, TC_FAIL on failure
 */

int eventTimeoutTest(void)
{
	int  rv;     /* return value from task_event_xxx() calls */
	int  i;      /* loop counter */

	/* Timeout while waiting for the event */
	rv = task_event_recv_wait_timeout(EVENT_ID, MSEC(100));
	if (rv != RC_TIME) {
		TC_ERROR("task_event_recv_wait_timeout() returned %d, not %d\n", rv, RC_TIME);
		return TC_FAIL;
	}

	/* Let there be an event already waiting to be tested */
	task_event_send(EVENT_ID);
	rv = task_event_recv_wait_timeout(EVENT_ID, MSEC(100));
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv_wait_timeout() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	task_sem_give(ALTERNATE_SEM);    /* Wake AlternateTask() */

	/*
	 * The 1st pass, task_event_recv_wait_timeout() will be signalled from a task,
	 * from an ISR for the second and from a fiber for the third.
	 */

	for (i = 0; i < 3; i++) {
		rv = task_event_recv_wait_timeout(EVENT_ID, MSEC(100));
		if (rv != RC_OK) {
			TC_ERROR("task_event_recv_wait() returned %d, not %d\n", rv, RC_OK);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * isrEventSignalTest - test the isr_event_send() API
 *
 * Although other tests have done some testing using isr_event_send(), none
 * of them have demonstrated that signalling an event more than once does not
 * "queue" events.  That is, should two or more signals of the same event occur
 * before it is tested, it can only be tested for successfully once.
 *
 * RETURNS: TC_PASS on success, TC_FAIL on failure
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

	rv = task_event_recv(EVENT_ID);
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	/* The second event signal should be "lost" */
	rv = task_event_recv(EVENT_ID);
	if (rv != RC_FAIL) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_FAIL);
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * fiberEventSignalTest - test the fiber_event_send() API
 *
 * Signalling an event by fiber_event_send() more than once does not "queue"
 * events.  That is, should two or more signals of the same event occur before
 * it is tested, it can only be tested for successfully once.
 *
 * RETURNS: TC_PASS on success, TC_FAIL on failure
 */

int fiberEventSignalTest(void)
{
	int  rv;    /* return value from task_event_recv() */

	/*
	 * Trigger two fiber event signals.  Only one should be detected.
	 */

	releaseTestFiber();

	rv = task_event_recv(EVENT_ID);
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	/* The second event signal should be "lost" */
	rv = task_event_recv(EVENT_ID);
	if (rv != RC_FAIL) {
		TC_ERROR("task_event_recv() returned %d, not %d\n", rv, RC_FAIL);
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * eventHandler - handler to run on EVENT_ID event
 *
 * @param event    signalled event
 *
 * RETURNS: <handlerRetVal>
 */

int eventHandler(int event)
{
	ARG_UNUSED(event);

	evidence++;

	return handlerRetVal;    /* 0 if not to wake waiting task; 1 if to wake */
}

/**
 *
 * altEventHandler - handler to run on ALT_EVENT event
 *
 * @param event    signalled event
 *
 * RETURNS: 1
 */

int altEventHandler(int event)
{
	ARG_UNUSED(event);

	evidence += 100;

	return 1;
}

/**
 *
 * eventSignalHandlerTest - test the task_event_set_handler() API
 *
 * This test checks that the event handler is set up properly when
 * task_event_set_handler() is called.  It shows that event handlers are tied
 * to the specified event and that the return value from the handler affects
 * whether the event wakes a task waiting upon that event.
 *
 * RETURNS: TC_PASS on success, TC_FAIL on failure
 */

int eventSignalHandlerTest(void)
{
	int  rv;     /* return value from task_event_xxx() calls */

	/* Expect this call to task_event_set_handler() to fail */
	rv = task_event_set_handler(EVENT_ID + 10, eventHandler);
	if (rv != RC_FAIL) {
		TC_ERROR("task_event_set_handler() returned %d not %d\n",
			rv, RC_FAIL);
		return TC_FAIL;
	}

	/* Expect this call to task_event_set_handler() to succeed */
	rv = task_event_set_handler(EVENT_ID, eventHandler);
	if (rv != RC_OK) {
		TC_ERROR("task_event_set_handler() returned %d not %d\n",
			rv, RC_OK);
		return TC_FAIL;
	}

	/* Enable another handler to show that two handlers can be installed */
	rv = task_event_set_handler(ALT_EVENT, altEventHandler);
	if (rv != RC_OK) {
		TC_ERROR("task_event_set_handler() returned %d not %d\n",
			rv, RC_OK);
		return TC_FAIL;
	}

	/*
	 * The alternate task should signal the event, but the handler will
	 * return 0 and the waiting task will not be woken up.  Thus, it should
	 * timeout and get an RC_TIME return code.
	 */

	task_sem_give(ALTERNATE_SEM);    /* Wake alternate task */
	rv = task_event_recv_wait_timeout(EVENT_ID, MSEC(100));
	if (rv != RC_TIME) {
		TC_ERROR("task_event_recv_wait_timeout() returned %d not %d\n", rv, RC_TIME);
		return TC_FAIL;
	}

	/*
	 * The alternate task should signal the event, and the handler will
	 * return 1 this time, which will wake the waiting task.
	 */

	task_sem_give(ALTERNATE_SEM);    /* Wake alternate task again */
	rv = task_event_recv_wait_timeout(EVENT_ID, MSEC(100));
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv_wait_timeout() returned %d not %d\n", rv, RC_OK);
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
	rv = task_event_set_handler(EVENT_ID, NULL);
	if (rv != RC_OK) {
		TC_ERROR("task_event_set_handler() returned %d not %d\n",
			rv, RC_OK);
		return TC_FAIL;
	}

	rv = task_event_set_handler(ALT_EVENT, NULL);
	if (rv != RC_OK) {
		TC_ERROR("task_event_set_handler() returned %d not %d\n",
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

	rv = task_event_recv(EVENT_ID);
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv() returned %d not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	rv = task_event_recv(ALT_EVENT);
	if (rv != RC_OK) {
		TC_ERROR("task_event_recv() returned %d not %d\n", rv, RC_OK);
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * AlternateTask - alternate task to signal various events to a waiting task
 *
 * RETURNS: N/A
 */

void AlternateTask(void)
{
	/* Wait for eventWaitTest() to run. */
	task_sem_take_wait(ALTERNATE_SEM);
	task_event_send(EVENT_ID);
	releaseTestFiber();
	_trigger_isrEventSignal();

	/* Wait for eventTimeoutTest() to run. */
	task_sem_take_wait(ALTERNATE_SEM);
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

	task_sem_take_wait(ALTERNATE_SEM);
	handlerRetVal = 0;
	task_event_send(EVENT_ID);

	task_sem_take_wait(ALTERNATE_SEM);
	handlerRetVal = 1;
	task_event_send(EVENT_ID);
}

/**
 *
 * RegressionTask - main entry point to the test suite
 *
 * RETURNS: N/A
 */

void RegressionTask(void)
{
	int  tcRC;    /* test case return code */

	TC_START("Test Microkernel Events\n");

	microObjectsInit();

	TC_PRINT("Testing task_event_recv() and task_event_send() ...\n");
	tcRC = eventNoWaitTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Testing task_event_recv_wait() and task_event_send() ...\n");
	tcRC = eventWaitTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

	TC_PRINT("Testing task_event_recv_wait_timeout() and task_event_send() ...\n");
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

	TC_PRINT("Testing task_event_set_handler() ...\n");
	tcRC = eventSignalHandlerTest();
	if (tcRC != TC_PASS) {
		goto doneTests;
	}

doneTests:
	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}
