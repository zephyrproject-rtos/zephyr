/* sema.c - test nanokernel semaphore APIs */

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
This module tests three basic scenarios with the usage of the following semaphore
routines:

   nano_sem_init
   nano_fiber_sem_give, nano_fiber_sem_take, nano_fiber_sem_take_wait
   nano_task_sem_give, nano_task_sem_take, nano_task_sem_take_wait
   nano_isr_sem_give, nano_isr_sem_take

Scenario #1:
   A task, fiber or ISR does not wait for the semaphore when taking it.

Scenario #2:
   A task or fiber must wait for the semaphore to be given before it gets it.

Scenario #3:
   Multiple fibers pend on the same semaphore.
*/

#include <tc_util.h>
#include <arch/cpu.h>

/* test uses 2 software IRQs */
#define NUM_SW_IRQS 2

#include <irq_test_common.h>
#include <util_test_common.h>

#define FIBER_STACKSIZE    2000
#define FIBER_PRIORITY     4

typedef struct {
	struct nano_sem *sem;    /* ptr to semaphore */
	int              data;   /* data */
} ISR_SEM_INFO;

typedef enum {
	STS_INIT = -1,
	STS_TASK_WOKE_FIBER,
	STS_FIBER_WOKE_TASK,
	STS_ISR_WOKE_TASK
} SEM_TEST_STATE;

static SEM_TEST_STATE  semTestState;
static ISR_SEM_INFO    isrSemInfo;
static struct nano_sem        testSem;
static int             fiberDetectedFailure = 0;

static struct nano_timer     timer;
static void *timerData[1];

static char __stack fiberStack[FIBER_STACKSIZE];

static void (*_trigger_nano_isr_sem_give)(void) = (vvfn)sw_isr_trigger_0;
static void (*_trigger_nano_isr_sem_take)(void) = (vvfn)sw_isr_trigger_1;

static struct nano_sem multi_waiters;
static struct nano_sem reply_multi_waiters;

/*******************************************************************************
*
* isr_sem_take - take a semaphore
*
* This routine is the ISR handler for _trigger_nano_isr_sem_take().  It takes a
* semaphore within the context of an ISR.
*
* \param data    pointer to ISR handler parameter
*
* RETURNS: N/A
*/

void isr_sem_take(void *data)
{
	ISR_SEM_INFO *pInfo = (ISR_SEM_INFO *) data;

	pInfo->data = nano_isr_sem_take(pInfo->sem);
}

/*******************************************************************************
*
* isr_sem_give - give a semaphore
*
* This routine is the ISR handler for _trigger_nano_isr_sem_take().  It gives a
* semaphore within the context of an ISR.
*
* \param data    pointer to ISR handler parameter
*
* RETURNS: N/A
*/

void isr_sem_give(void *data)
{
	ISR_SEM_INFO *pInfo = (ISR_SEM_INFO *) data;

	nano_isr_sem_give(pInfo->sem);
	pInfo->data = 1;     /* Indicate semaphore has been given */
}

/*******************************************************************************
*
* testSemFiberNoWait - give and take the semaphore in a fiber without blocking
*
* This test gives and takes the test semaphore in the context of a fiber
* without blocking on the semaphore.
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int testSemFiberNoWait(void)
{
	int  i;

	TC_PRINT("Giving and taking a semaphore in a fiber (non-blocking)\n");

	/*
	 * Give the semaphore many times and then make sure that it can only be
	 * taken that many times.
	 */

	for (i = 0; i < 32; i++) {
		nano_fiber_sem_give(&testSem);
	}

	for (i = 0; i < 32; i++) {
		if (nano_fiber_sem_take(&testSem) != 1) {
			TC_ERROR(" *** Expected nano_fiber_sem_take() to succeed, not fail\n");
			goto errorReturn;
		}
	}

	if (nano_fiber_sem_take(&testSem) != 0) {
		TC_ERROR(" *** Expected  nano_fiber_sem_take() to fail, not succeed\n");
		goto errorReturn;
	}

	return TC_PASS;

errorReturn:
	fiberDetectedFailure = 1;
	return TC_FAIL;
}

/*******************************************************************************
*
* fiberEntry - entry point for the fiber portion of the semaphore tests
*
* NOTE: The fiber portion of the tests have higher priority than the task
* portion of the tests.
*
* \param arg1    unused
* \param arg2    unused
*
* RETURNS: N/A
*/

static void fiberEntry(int arg1, int arg2)
{
	int  rv;      /* return value from a test */

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	rv = testSemFiberNoWait();
	if (rv != TC_PASS) {
		return;
	}

	/*
	 * At this point <testSem> is not available.  Wait for <testSem> to become
	 * available (the main task will give it).
	 */

	nano_fiber_sem_take_wait(&testSem);

	semTestState = STS_TASK_WOKE_FIBER;

	/*
	 * Delay for two seconds.  This gives the main task time to print
	 * any messages (very important if I/O link is slow!), and wait
	 * on <testSem>.  Once the delay is done, this fiber will give <testSem>
	 * thus waking the main task.
	 */

	nano_fiber_timer_start(&timer, SECONDS(2));
	nano_fiber_timer_wait(&timer);

	/*
	 * The main task is now waiting on <testSem>.  Give the semaphore <testSem>
	 * to wake it.
	 */

	nano_fiber_sem_give(&testSem);

	/*
	 * Some small delay must be done so that the main task can process the
	 * semaphore signal.
	 */

	semTestState = STS_FIBER_WOKE_TASK;

	nano_fiber_timer_start(&timer, SECONDS(2));
	nano_fiber_timer_wait(&timer);

	/*
	 * The main task should be waiting on <testSem> again.  This time, instead
	 * of giving the semaphore from the semaphore, give it from an ISR to wake
	 * the main task.
	 */

	isrSemInfo.data = 0;
	isrSemInfo.sem = &testSem;
	_trigger_nano_isr_sem_give();

	if (isrSemInfo.data == 1) {
		semTestState = STS_ISR_WOKE_TASK;
	}
}

/*******************************************************************************
*
* initNanoObjects - initialize nanokernel objects
*
* This routine initializes the nanokernel objects used in the semaphore tests.
*
* RETURNS: N/A
*/

void initNanoObjects(void)
{
	struct isrInitInfo i = {
	{isr_sem_give, isr_sem_take},
	{&isrSemInfo, &isrSemInfo},
	};

	(void)initIRQ(&i);

	nano_sem_init(&testSem);
	nano_sem_init(&multi_waiters);
	nano_sem_init(&reply_multi_waiters);
	nano_timer_init(&timer, timerData);

	TC_PRINT("Nano objects initialized\n");
}

/*******************************************************************************
*
* testSemIsrNoWait - give and take the semaphore in an ISR without blocking
*
* This test gives and takes the test semaphore in the context of an ISR without
* blocking on the semaphore.
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int testSemIsrNoWait(void)
{
	int  i;

	TC_PRINT("Giving and taking a semaphore in an ISR (non-blocking)\n");

	/*
	 * Give the semaphore many times and then make sure that it can only be
	 * taken that many times.
	 */

	isrSemInfo.sem = &testSem;
	for (i = 0; i < 32; i++) {
		_trigger_nano_isr_sem_give();
	}

	for (i = 0; i < 32; i++) {
		isrSemInfo.data = 0;
		_trigger_nano_isr_sem_take();
		if (isrSemInfo.data != 1) {
			TC_ERROR(" *** Expected nano_isr_sem_take() to succeed, not fail\n");
			goto errorReturn;
		}
	}

	_trigger_nano_isr_sem_take();
	if (isrSemInfo.data != 0) {
		TC_ERROR(" *** Expected  nano_isr_sem_take() to fail, not succeed!\n");
		goto errorReturn;
	}

	return TC_PASS;

errorReturn:
	return TC_FAIL;
}

/*******************************************************************************
*
* testSemTaskNoWait - give and take the semaphore in a task without blocking
*
* This test gives and takes the test semaphore in the context of a task without
* blocking on the semaphore.
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int testSemTaskNoWait(void)
{
	int  i;     /* loop counter */

	TC_PRINT("Giving and taking a semaphore in a task (non-blocking)\n");

	/*
	 * Give the semaphore many times and then make sure that it can only be
	 * taken that many times.
	 */

	for (i = 0; i < 32; i++) {
		nano_task_sem_give(&testSem);
	}

	for (i = 0; i < 32; i++) {
		if (nano_task_sem_take(&testSem) != 1) {
			TC_ERROR(" *** Expected nano_task_sem_take() to succeed, not fail\n");
			goto errorReturn;
		}
	}

	if (nano_task_sem_take(&testSem) != 0) {
		TC_ERROR(" *** Expected  nano_task_sem_take() to fail, not succeed!\n");
		goto errorReturn;
	}

	return TC_PASS;

errorReturn:
	return TC_FAIL;
}

/*******************************************************************************
*
* testSemWait - perform tests that wait on a semaphore
*
* This routine works with fiberEntry() to perform the tests that wait on
* a semaphore.
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int testSemWait(void)
{
	if (fiberDetectedFailure != 0) {
		TC_ERROR(" *** Failure detected in the fiber.");
		return TC_FAIL;
	}

	nano_task_sem_give(&testSem);    /* Wake the fiber. */

	if (semTestState != STS_TASK_WOKE_FIBER) {
		TC_ERROR(" *** Expected task to wake fiber.  It did not.\n");
		return TC_FAIL;
	}

	TC_PRINT("Semaphore from the task woke the fiber\n");

	nano_task_sem_take_wait(&testSem);   /* Wait on <testSem> */

	if (semTestState != STS_FIBER_WOKE_TASK) {
		TC_ERROR(" *** Expected fiber to wake task.  It did not.\n");
		return TC_FAIL;
	}

	TC_PRINT("Semaphore from the fiber woke the task\n");

	nano_task_sem_take_wait(&testSem);  /* Wait on <testSem> again. */

	if (semTestState != STS_ISR_WOKE_TASK) {
		TC_ERROR(" *** Expected ISR to wake task.  It did not.\n");
		return TC_FAIL;
	}

	TC_PRINT("Semaphore from the ISR woke the task.\n");
	return TC_PASS;
}

/*
 * Multiple-waiters test
 *
 * NUM_WAITERS fibers pend on the multi_waiters semaphore, then the task give
 * the semaphore NUM_WAITERS times. Each time, the first fiber in the queue
 * wakes up, is context-switched to, and gives the reply_multi_waiters
 * semaphore, for a total of NUM_WAITERS times. The task finally must be able
 * to obtain the reply_multi_waiters semaphore NUM_WAITERS times.
 */
#define NUM_WAITERS 3
static char __stack fiber_multi_waiters_stacks[NUM_WAITERS][FIBER_STACKSIZE];

/*******************************************************************************
*
* fiber_multi_waiters - fiber entry point for multiple-waiters test
*
* RETURNS: N/A
*/

static void fiber_multi_waiters(int arg1, int arg2)
{
	TC_PRINT("multiple-waiter fiber %d trying to get semaphore...\n", arg1);
	nano_fiber_sem_take_wait(&multi_waiters);
	TC_PRINT("multiple-waiter fiber %d acquired semaphore, sending reply\n",
				arg1);
	nano_fiber_sem_give(&reply_multi_waiters);
}

/*******************************************************************************
*
* do_test_multiple_waiters - task part of multiple-waiter test, repeatable
*
* RETURNS: N/A
*/

static int do_test_multiple_waiters(void)
{
	int ii;

	/* pend all fibers one the same semaphore */
	for (ii = 0; ii < NUM_WAITERS; ii++) {
		task_fiber_start(fiber_multi_waiters_stacks[ii], FIBER_STACKSIZE,
							fiber_multi_waiters, ii, 0, FIBER_PRIORITY, 0);
	}

	/* wake up all the fibers: the task is preempted each time */
	for (ii = 0; ii < NUM_WAITERS; ii++) {
		nano_task_sem_give(&multi_waiters);
	}

	/* reply_multi_waiters will have been given once for each fiber */
	for (ii = 0; ii < NUM_WAITERS; ii++) {
		if (!nano_task_sem_take(&reply_multi_waiters)) {
			TC_ERROR(" *** Cannot take sem supposedly given by waiters.\n");
			return TC_FAIL;
		}
	}

	TC_PRINT("Task took multi-waiter reply semaphore %d times, as expected.\n",
				NUM_WAITERS);

	if (nano_task_sem_take(&multi_waiters)) {
		TC_ERROR(" *** multi_waiters should have been empty.\n");
		return TC_FAIL;
	}

	if (nano_task_sem_take(&reply_multi_waiters)) {
		TC_ERROR(" *** reply_multi_waiters should have been empty.\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/*******************************************************************************
*
* test_multiple_waiters - entry point for multiple-waiters test
*
* RETURNS: N/A
*/

static int test_multiple_waiters(void)
{
	TC_PRINT("First pass\n");
	if (do_test_multiple_waiters() == TC_FAIL) {
		TC_ERROR(" *** First pass test failed.\n");
		return TC_FAIL;
	}

	/*
	 * Verify a wait q that has been emptied has been reset correctly, so
	 * redo the test.
	 */

	TC_PRINT("Second pass\n");
	if (do_test_multiple_waiters() == TC_FAIL) {
		TC_ERROR(" *** Second pass test failed.\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

/*******************************************************************************
*
* main - entry point to semaphore tests
*
* This is the entry point to the semaphore tests.
*
* RETURNS: N/A
*/

void main(void)
{
	int     rv;       /* return value from tests */

	TC_START("Test Nanokernel Semaphores");

	initNanoObjects();

	rv = testSemTaskNoWait();
	if (rv != TC_PASS) {
		goto doneTests;
	}

	rv = testSemIsrNoWait();
	if (rv != TC_PASS) {
		goto doneTests;
	}

	semTestState = STS_INIT;

	/*
	 * Start the fiber.  The fiber will be given a higher priority than the
	 * main task.
	 */

	task_fiber_start(fiberStack, FIBER_STACKSIZE, fiberEntry,
					 0, 0, FIBER_PRIORITY, 0);

	rv = testSemWait();
	if (rv != TC_PASS) {
		goto doneTests;
	}

	rv = test_multiple_waiters();
	if (rv != TC_PASS) {
		goto doneTests;
	}

doneTests:
	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
