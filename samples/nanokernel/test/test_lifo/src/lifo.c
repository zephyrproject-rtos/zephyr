/* lifo.c - test LIFO APIs under VxMicro */

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
This module tests three basic scenarios with the usage of the following LIFO
routines:

   nano_isr_lifo_get, nano_isr_lifo_put
   nano_fiber_lifo_get, nano_fiber_lifo_get_wait, nano_fiber_lifo_put
   nano_task_lifo_get, nano_task_lifo_get_wait, nano_task_lifo_put

Scenario #1
  Getting (and waiting for an object) from an empty LIFO.  Both fibers and
tasks can wait on a LIFO, but an ISR can not.

Scenario #2
  Getting objects from a non-empty LIFO.  Fibers, tasks and ISRs are all
allowed to get an object from a non-empty LIFO.

Scenario #3:
   Multiple fibers pend on the same LIFO.

These scenarios will be tested using a combinations of tasks, fibers and ISRs.
*/

/* includes */

#include <tc_util.h>
#include <nanokernel/cpu.h>

/* test uses 2 software IRQs */
#define NUM_SW_IRQS 2

#include <irq_test_common.h>
#include <util_test_common.h>

/* defines */

#define FIBER_STACKSIZE    2000
#define FIBER_PRIORITY     4

/* typedefs */

typedef struct {
	struct nano_lifo *channel;  /* LIFO channel */
	void *data;     /* pointer to data to add */
	} ISR_LIFO_INFO;

typedef struct {
	uint32_t   link;     /* 32-bit word for LIFO to use as a link */
	uint32_t   data;     /* miscellaneous data put on LIFO (not important) */
	} LIFO_ITEM;

/* locals */

/* Items to be added/removed from LIFO during the test */
static LIFO_ITEM  lifoItem[4] = {
		{0, 1},
		{0, 2},
		{0, 3},
		{0, 4},
	};

static struct nano_lifo      lifoChannel;         /* LIFO channel used in test */
static struct nano_sem       taskWaitSem;         /* task waits on this semaphore */
static struct nano_sem       fiberWaitSem;        /* fiber waits on this semaphore */
static struct nano_timer     timer;
static void *timerData[1];
static ISR_LIFO_INFO  isrLifoInfo = {&lifoChannel, NULL};

static volatile int  fiberDetectedFailure = 0; /* non-zero on failure */

static char fiberStack[FIBER_STACKSIZE];

static void (*_trigger_nano_isr_lifo_put)(void) = (vvfn)sw_isr_trigger_0;
static void (*_trigger_nano_isr_lifo_get)(void) = (vvfn)sw_isr_trigger_1;

static struct nano_lifo multi_waiters;
static struct nano_sem reply_multi_waiters;

/*******************************************************************************
*
* isr_lifo_put - add an item to a LIFO
*
* This routine is the ISR handler for _trigger_nano_isr_lifo_put().  It adds
* an item to the LIFO in the context of an ISR.
*
* \param data    pointer to ISR handler parameter
*
* RETURNS: N/A
*/

void isr_lifo_put(void *data)
{
	ISR_LIFO_INFO *pInfo = (ISR_LIFO_INFO *) data;

	nano_isr_lifo_put(pInfo->channel, pInfo->data);
}

/*******************************************************************************
*
* isr_lifo_get - get an item from a LIFO
*
* This routine is the ISR handler for _trigger_nano_isr_lifo_get().  It gets
* an item from the LIFO in the context of an ISR.
*
* \param data    pointer to ISR handler parameter
*
* RETURNS: N/A
*/

void isr_lifo_get(void *data)
{
	ISR_LIFO_INFO *pInfo = (ISR_LIFO_INFO *) data;

	pInfo->data = nano_isr_lifo_get(pInfo->channel);
}

/*******************************************************************************
*
* fiberLifoWaitTest - fiber portion of test that waits on a LIFO
*
* This routine works with taskLifoWaitTest() to test the addition and removal
* of items to/from a LIFO.  The cases covered will have a fiber or task waiting
* on an empty LIFO.
*
* RETURNS: 0 on success, -1 on failure
*/

int fiberLifoWaitTest(void)
{
	void *data;     /* ptr to data retrieved from LIFO */

    /*
     * The LIFO is empty; wait for an item to be added to the LIFO
     * from the task.
     */

	TC_PRINT("Fiber waiting on an empty LIFO\n");
	nano_fiber_sem_give(&taskWaitSem);
	data = nano_fiber_lifo_get_wait(&lifoChannel);
	if (data != &lifoItem[0]) {
		fiberDetectedFailure = 1;
		return -1;
		}

	nano_fiber_sem_take_wait(&fiberWaitSem);
	data = nano_fiber_lifo_get_wait(&lifoChannel);
	if (data != &lifoItem[2]) {
		fiberDetectedFailure = 1;
		return -1;
		}

    /*
     * Give the task some time to check the results.  Ideally, this would
     * be waiting for a semaphore instead of a using a delay, but if the
     * main task wakes the fiber before it blocks on the LIFO, the fiber
     * will add the item to the LIFO too soon.  Obviously, a semaphore could
     * not be given if the task is blocked on the LIFO; hence the delay.
     */

	nano_fiber_timer_start(&timer, SECONDS(2));
	nano_fiber_timer_wait(&timer);

    /* The task is waiting on an empty LIFO.  Wake it up. */
	nano_fiber_lifo_put(&lifoChannel, &lifoItem[3]);
	nano_fiber_lifo_put(&lifoChannel, &lifoItem[1]);

    /*
     * Wait for the task to check the results.  If the results pass, then the
     * the task will wake the fiber.  If the results do not pass, then the
     * fiber will wait forever.
     */

	nano_fiber_sem_take_wait(&fiberWaitSem);

	return 0;
}

/*******************************************************************************
*
* fiberLifoNonWaitTest - fiber portion of test that does not wait on a LIFO
*
* This routine works with fiberLifoNonWaitTest() to test the addition and
* removal of items from a LIFO without having to wait.
*
* RETURNS: 0 on success, -1 on failure
*/

int fiberLifoNonWaitTest(void)
{
	void *data;    /* pointer to data retrieved from LIFO */

    /* The LIFO has two items in it; retrieve them both */

	data = nano_fiber_lifo_get(&lifoChannel);
	if (data != (void *) &lifoItem[3]) {
		goto errorReturn;
		}

	data = nano_fiber_lifo_get(&lifoChannel);
	if (data != (void *) &lifoItem[2]) {
		goto errorReturn;
		}

    /* LIFO should be empty--verify. */
	data = nano_fiber_lifo_get(&lifoChannel);
	if (data != NULL) {
		goto errorReturn;
		}

    /*
     * The LIFO is now empty.  Add two items to the LIFO and then wait
     * for the semaphore so that the task can retrieve them.
     */

	TC_PRINT("Task to get LIFO items without waiting\n");
	nano_fiber_lifo_put(&lifoChannel, &lifoItem[0]);
	nano_fiber_lifo_put(&lifoChannel, &lifoItem[1]);
	nano_fiber_sem_give(&taskWaitSem);       /* Wake the task (if blocked) */

    /*
     * Wait for the task to get the items and then trigger an ISR to populate
     * the LIFO.
     */

	nano_fiber_sem_take_wait(&fiberWaitSem);

    /*
     * The task retrieved the two items from the LIFO and then triggered
     * two interrupts to add two other items to the LIFO.  The fiber will
     * now trigger two interrupts to read the two items.
     */

	_trigger_nano_isr_lifo_get();
	if (isrLifoInfo.data != &lifoItem[1]) {
		goto errorReturn;
		}

	_trigger_nano_isr_lifo_get();
	if (isrLifoInfo.data != &lifoItem[3]) {
		goto errorReturn;
		}

    /* The LIFO should now be empty--verify */
	_trigger_nano_isr_lifo_get();
	if (isrLifoInfo.data != NULL) {
		goto errorReturn;
		}

	return 0;

errorReturn:
	fiberDetectedFailure = 1;
	return -1;
}

/*******************************************************************************
*
* fiberEntry - entry point for the fiber portion of the LIFO tests
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

	rv = fiberLifoWaitTest();

	if (rv == 0) {
		fiberLifoNonWaitTest();
		}

}

/*******************************************************************************
*
* taskLifoWaitTest - task portion of test that waits on a LIFO
*
* This routine works with fiberLifoWaitTest() to test the addition and removal
* of items to/from a LIFO.  The cases covered will have a fiber or task waiting
* on an empty LIFO.
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int taskLifoWaitTest(void)
{
	void *data;    /* ptr to data retrieved from LIFO */

    /* Wait on <taskWaitSem> in case fiber's print message blocked */
	nano_fiber_sem_take_wait(&taskWaitSem);

    /* The fiber is waiting on the LIFO.  Wake it. */
	nano_task_lifo_put(&lifoChannel, &lifoItem[0]);

    /*
     * The fiber ran, but is now blocked on the semaphore.  Add an item to the
     * LIFO before giving the semaphore that wakes the fiber so that we can
     * cover the path of nano_fiber_lifo_get_wait() not waiting on the LIFO.
     */

	nano_task_lifo_put(&lifoChannel, &lifoItem[2]);
	nano_task_sem_give(&fiberWaitSem);

    /* Check that the fiber got the correct item (lifoItem[0]) */

	if (fiberDetectedFailure) {
		TC_ERROR(" *** nano_task_lifo_put()/nano_fiber_lifo_get_wait() failure\n");
		return TC_FAIL;
		}

    /* The LIFO is empty.  This time the task will wait for the item. */

	TC_PRINT("Task waiting on an empty LIFO\n");
	data = nano_task_lifo_get_wait(&lifoChannel);
	if (data != (void *) &lifoItem[1]) {
		TC_ERROR(" *** nano_task_lifo_get_wait()/nano_fiber_lifo_put() failure\n");
		return TC_FAIL;
		}

	data = nano_task_lifo_get_wait(&lifoChannel);
	if (data != (void *) &lifoItem[3]) {
		TC_ERROR(" *** nano_task_lifo_get_wait()/nano_fiber_lifo_put() failure\n");
		return TC_FAIL;
		}


    /* Waiting on an empty LIFO passed for both fiber and task. */

	return TC_PASS;
}

/*******************************************************************************
*
* taskLifoNonWaitTest - task portion of test that does not wait on a LIFO
*
* This routine works with fiberLifoNonWaitTest() to test the addition and
* removal of items from a LIFO without having to wait.
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int taskLifoNonWaitTest(void)
{
	void *data;    /* ptr to data retrieved from LIFO */

    /*
     * The fiber is presently waiting for <fiberWaitSem>.  Populate the LIFO
     * before waking the fiber.
     */

	TC_PRINT("Fiber to get LIFO items without waiting\n");
	nano_task_lifo_put(&lifoChannel, &lifoItem[2]);
	nano_task_lifo_put(&lifoChannel, &lifoItem[3]);
	nano_task_sem_give(&fiberWaitSem);    /* Wake the fiber */

    /* Check that fiber received the items correctly */
	if (fiberDetectedFailure) {
		TC_ERROR(" *** nano_task_lifo_put()/nano_fiber_lifo_get() failure\n");
		return TC_FAIL;
		}

    /* Wait for the fiber to be ready */
	nano_task_sem_take_wait(&taskWaitSem);

	data = nano_task_lifo_get(&lifoChannel);
	if (data != (void *) &lifoItem[1]) {
		TC_ERROR(" *** nano_task_lifo_get()/nano_fiber_lifo_put() failure\n");
		return TC_FAIL;
		}

	data = nano_task_lifo_get(&lifoChannel);
	if (data != (void *) &lifoItem[0]) {
		TC_ERROR(" *** nano_task_lifo_get()/nano_fiber_lifo_put() failure\n");
		return TC_FAIL;
		}

	data = nano_task_lifo_get(&lifoChannel);
	if (data != NULL) {
		TC_ERROR(" *** nano_task_lifo_get()/nano_fiber_lifo_put() failure\n");
		return TC_FAIL;
		}

    /*
     * Software interrupts have been configured so that when invoked,
     * the ISR will add an item to the LIFO.  The fiber (when unblocked)
     * trigger software interrupts to get the items from the LIFO from
     * within an ISR.
     *
     * Populate the LIFO.
     */

	TC_PRINT("ISR to get LIFO items without waiting\n");
	isrLifoInfo.data = &lifoItem[3];
	_trigger_nano_isr_lifo_put();
	isrLifoInfo.data = &lifoItem[1];
	_trigger_nano_isr_lifo_put();

	isrLifoInfo.data = NULL;    /* Force NULL to ensure [data] changes */

	nano_task_sem_give(&fiberWaitSem);    /* Wake the fiber */

	if (fiberDetectedFailure) {
		TC_ERROR(" *** nano_isr_lifo_put()/nano_isr_lifo_get() failure\n");
		return TC_FAIL;
		}

	return TC_PASS;
}

/*******************************************************************************
*
* initNanoObjects - initialize nanokernel objects
*
* This routine initializes the nanokernel objects used in the LIFO tests.
*
* RETURNS: N/A
*/

void initNanoObjects(void)
{
	struct isrInitInfo i = {
	{isr_lifo_put, isr_lifo_get},
	{&isrLifoInfo, &isrLifoInfo},
	};

	(void)initIRQ(&i);

	nano_lifo_init(&lifoChannel);   /* Initialize the LIFO channel */
	nano_sem_init(&taskWaitSem);   /* Initialize the task waiting semaphore */
	nano_sem_init(&fiberWaitSem);  /* Initialize the fiber waiting semaphore */
	nano_timer_init(&timer, timerData);

	nano_lifo_init(&multi_waiters);
	nano_sem_init(&reply_multi_waiters);

	TC_PRINT("Nano objects initialized\n");
}

/*
 * Multiple-waiters test
 *
 * NUM_WAITERS fibers pend on the multi_waiters LIFO, then the task puts data
 * on the LIFO NUM_WAITERS times. Each time, the first fiber in the queue wakes
 * up, is context-switched to, verifies the data is the one expected, and gives
 * the reply_multi_waiters semaphore, for a total of NUM_WAITERS times. The
 * task finally must be able to obtain the reply_multi_waiters semaphore
 * NUM_WAITERS times.
 */
#define NUM_WAITERS 3
static char fiber_multi_waiters_stacks[NUM_WAITERS][FIBER_STACKSIZE];
static LIFO_ITEM multi_waiters_items[NUM_WAITERS] = {
	[0 ...(NUM_WAITERS-1)].link = 0,
	[0 ...(NUM_WAITERS-1)].data = 0xabad1dea,
};

/*******************************************************************************
*
* fiber_multi_waiters - fiber entry point for multiple-waiters test
*
* RETURNS: N/A
*/

static void fiber_multi_waiters(int arg1, int arg2)
{
	void *item;

	TC_PRINT("multiple-waiter fiber %d receiving item...\n", arg1);
	item = nano_fiber_lifo_get_wait(&multi_waiters);
	if (item != &multi_waiters_items[arg1]) {
		TC_ERROR(" *** fiber %d did not receive correct item\n", arg1);
		TC_ERROR(" *** received %p instead of %p.\n",
					item, &multi_waiters_items[arg1]);

		/* do NOT give the semaphore, signifying an error */
		return;
	}
	TC_PRINT("multiple-waiter fiber %d got correct item, giving semaphore\n",
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

	/* pend all fibers one the same lifo */
	for (ii = 0; ii < NUM_WAITERS; ii++) {
		task_fiber_start(fiber_multi_waiters_stacks[ii], FIBER_STACKSIZE,
							fiber_multi_waiters, ii, 0, FIBER_PRIORITY, 0);
	}

	/* wake up all the fibers: the task is preempted each time */
	for (ii = 0; ii < NUM_WAITERS; ii++) {
		nano_task_lifo_put(&multi_waiters, &multi_waiters_items[ii]);
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

	if (nano_task_lifo_get(&multi_waiters)) {
		TC_ERROR(" *** multi_waiters should have been empty.\n");
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
	 * Verity a wait q that has been emptied has been reset correctly, so
	 * redo the test. This time, send one message before starting the fibers.
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
* main - entry point to LIFO tests
*
* This is the entry point to the LIFO tests.
*
* RETURNS: N/A
*/

void main(void)
{
	int     rv;       /* return value from tests */

	TC_START("Test Nanokernel LIFO");

	initNanoObjects();

    /*
     * Start the fiber.  The fiber will be given a higher priority than the
     * main task.
     */

	task_fiber_start(fiberStack, FIBER_STACKSIZE, fiberEntry,
		                0, 0, FIBER_PRIORITY, 0);

	rv = taskLifoWaitTest();

	if (rv == TC_PASS) {
		rv = taskLifoNonWaitTest();
		}

	if (rv == TC_PASS) {
		rv = test_multiple_waiters();
		}

	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
