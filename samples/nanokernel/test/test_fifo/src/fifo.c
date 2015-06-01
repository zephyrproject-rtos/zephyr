/* fifo.c - test nanokernel FIFO APIs under VxMicro */

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
This module tests three basic scenarios with the usage of the following FIFO
routines:

   nano_fiber_fifo_get, nano_fiber_fifo_get_wait, nano_fiber_fifo_put
   nano_task_fifo_get, nano_task_fifo_get_wait, nano_task_fifo_put
   nano_isr_fifo_get, nano_isr_fifo_put

Scenario #1
  Task enters items into a queue, starts the fiber and waits for a semaphore.
Fiber extracts all items from the queue and enters some items back into
the queue.  Fiber gives the semaphore for task to continue.  Once the control
is returned back to task, task extracts all items from the queue.

Scenario #2
  Task enters an item into queue2, starts a fiber and extract an item from
queue1 once the item is there.  The fiber will extract an item from queue2
once the item is there and and enter an item to queue1.  The flow of control
goes from task to fiber and so forth.

Scenario #3
  Tests the ISR interfaces.  Function testIsrFifoFromFiber gets an item from
the fifo queue in ISR context.  It then enters four items into the queue
and finishes execution. Control is returned back to function
testTaskFifoGetW which also finished it's execution and returned to main.
Finally function testIsrFifoFromTask is run and it gets all data from
the queue and puts and gets one last item to the queue.  All these are run
in ISR context.
*/

#include <tc_util.h>
#include <arch/cpu.h>

/* test uses 2 software IRQs */
#define NUM_SW_IRQS 2

#include <irq_test_common.h>
#include <util_test_common.h>

#define STACKSIZE               2048
#define NUM_FIFO_ELEMENT        4
#define INVALID_DATA            NULL

#define TCERR1(count)  TC_ERROR("Didn't get back correct FIFO, count %d\n", count)
#define TCERR2         TC_ERROR("Didn't get back correct FIFO\n")
#define TCERR3         TC_ERROR("The queue should be empty!\n")

typedef struct {
	struct nano_fifo *channel;  /* FIFO channel */
	void *data;     /* pointer to data to add */
} ISR_FIFO_INFO;

char __stack fiberStack1[STACKSIZE];
char __stack fiberStack2[STACKSIZE];
char __stack fiberStack3[STACKSIZE];

struct nano_fifo  nanoFifoObj;
struct nano_fifo  nanoFifoObj2;

struct nano_sem   nanoSemObj1;    /* Used to block/wake-up fiber1 */
struct nano_sem   nanoSemObj2;    /* Used to block/wake-up fiber2 */
struct nano_sem   nanoSemObj3;    /* Used to block/wake-up fiber3 */
struct nano_sem   nanoSemObjTask; /* Used to block/wake-up task */

struct nano_timer  timer;
void *timerData[1];

int myFifoData1[4];
int myFifoData2[2];
int myFifoData3[4];
int myFifoData4[2];

void * const pMyFifoData1 = (void *)myFifoData1;
void * const pMyFifoData2 = (void *)myFifoData2;
void * const pMyFifoData3 = (void *)myFifoData3;
void * const pMyFifoData4 = (void *)myFifoData4;

void * const pPutList1[NUM_FIFO_ELEMENT]  = {
	(void *)myFifoData1,
	(void *)myFifoData2,
	(void *)myFifoData3,
	(void *)myFifoData4
	};

void * const pPutList2[NUM_FIFO_ELEMENT] = {
	(void *)myFifoData4,
	(void *)myFifoData3,
	(void *)myFifoData2,
	(void *)myFifoData1
	};

int retCode = TC_PASS;

static ISR_FIFO_INFO  isrFifoInfo = {&nanoFifoObj, NULL};

static void (*_trigger_nano_isr_fifo_put)(void) = (vvfn)sw_isr_trigger_0;
static void (*_trigger_nano_isr_fifo_get)(void) = (vvfn)sw_isr_trigger_1;

void fiber1(void);
void fiber2(void);
void fiber3(void);

void initNanoObjects(void);
void testTaskFifoGetW(void);

/*******************************************************************************
*
* isr_fifo_put - add an item to a FIFO
*
* This routine is the ISR handler for _trigger_nano_isr_fifo_put().  It adds
* an item to the FIFO in the context of an ISR.
*
* \param parameter    pointer to ISR handler parameter
*
* RETURNS: N/A
*/

void isr_fifo_put(void *parameter)
{
	ISR_FIFO_INFO *pInfo = (ISR_FIFO_INFO *) parameter;

	nano_isr_fifo_put(pInfo->channel, pInfo->data);
}

/*******************************************************************************
*
* isr_fifo_get - get an item from a FIFO
*
* This routine is the ISR handler for _trigger_nano_isr_fifo_get().  It gets
* an item from the FIFO in the context of an ISR.
*
* \param parameter    pointer to ISR handler parameter
*
* RETURNS: N/A
*/

void isr_fifo_get(void *parameter)
{
	ISR_FIFO_INFO *pInfo = (ISR_FIFO_INFO *) parameter;

	pInfo->data = nano_isr_fifo_get(pInfo->channel);
}


/*******************************************************************************
*
* fiber1 - entry point for the first fiber
*
* RETURNS: N/A
*/

void fiber1(void)
{
	void   *pData;      /* pointer to FIFO object get from the queue */
	int     count = 0;  /* counter */

	/* Wait for fiber1 to be activated. */
	nano_fiber_sem_take_wait(&nanoSemObj1);

	/* Wait for data to be added to <nanoFifoObj> by task */
	pData = nano_fiber_fifo_get_wait(&nanoFifoObj);
	if (pData != pPutList1[0]) {
		TC_ERROR("fiber1 (1) - expected 0x%x, got 0x%x\n",
				 pPutList1[0], pData);
		retCode = TC_FAIL;
		return;
	}

	/* Wait for data to be added to <nanoFifoObj2> by fiber3 */
	pData = nano_fiber_fifo_get_wait(&nanoFifoObj2);
	if (pData != pPutList2[0]) {
		TC_ERROR("fiber1 (2) - expected 0x%x, got 0x%x\n",
				 pPutList2[0], pData);
		retCode = TC_FAIL;
		return;
	}

	nano_fiber_sem_take_wait(&nanoSemObj1);   /* Wait for fiber1 to be reactivated */

	TC_PRINT("Test Fiber FIFO Get\n\n");
	/* Get all FIFOs */
	while ((pData = nano_fiber_fifo_get(&nanoFifoObj)) != NULL) {
		TC_PRINT("FIBER FIFO Get: count = %d, ptr is %p\n", count, pData);
		if ((count >= NUM_FIFO_ELEMENT) || (pData != pPutList1[count])) {
			TCERR1(count);
			retCode = TC_FAIL;
			return;
		}
		count++;
	}

	TC_END_RESULT(retCode);
	PRINT_LINE;

	/*
	 * Entries in the FIFO queue have to be unique.
	 * Put data.
	 */
	TC_PRINT("Test Fiber FIFO Put\n");
	TC_PRINT("\nFIBER FIFO Put Order: ");
	for (int i=0; i<NUM_FIFO_ELEMENT; i++) {
		nano_fiber_fifo_put(&nanoFifoObj, pPutList2[i]);
		TC_PRINT(" %p,", pPutList2[i]);
	}
	TC_PRINT("\n");
	PRINT_LINE;

	/* Give semaphore to allow the main task to run */
	nano_fiber_sem_give(&nanoSemObjTask);

} /* fiber1 */


/*******************************************************************************
*
* testFiberFifoGetW - test the nano_fiber_fifo_get_wait() interface
*
* This function tests the fifo put and get wait interfaces in the fiber context.
* It gets data from nanoFifoObj2 queue and puts data to nanoFifoObj queue.
*
* RETURNS: N/A
*/

void testFiberFifoGetW(void)
{
	void *pGetData;   /* pointer to FIFO object get from the queue */
	void *pPutData;   /* pointer to FIFO object to put to the queue */

	TC_PRINT("Test Fiber FIFO Get Wait Interfaces\n\n");
	pGetData = nano_fiber_fifo_get_wait(&nanoFifoObj2);
	TC_PRINT("FIBER FIFO Get from queue2: %p\n", pGetData);
	/* Verify results */
	if (pGetData != pMyFifoData1) {
		retCode = TC_FAIL;
		TCERR2;
		return;
	}

	pPutData = pMyFifoData2;
	TC_PRINT("FIBER FIFO Put to queue1: %p\n", pPutData);
	nano_fiber_fifo_put(&nanoFifoObj, pPutData);

	pGetData = nano_fiber_fifo_get_wait(&nanoFifoObj2);
	TC_PRINT("FIBER FIFO Get from queue2: %p\n", pGetData);
	/* Verify results */
	if (pGetData != pMyFifoData3) {
		retCode = TC_FAIL;
		TCERR2;
		return;
	}

	pPutData = pMyFifoData4;
	TC_PRINT("FIBER FIFO Put to queue1: %p\n", pPutData);
	nano_fiber_fifo_put(&nanoFifoObj, pPutData);

	TC_END_RESULT(retCode);

}  /* testFiberFifoGetW */


/*******************************************************************************
*
* testIsrFifoFromFiber - test ISR FIFO routines (triggered from fiber)
*
* This function tests the fifo put and get interfaces in the isr context.
* It is invoked from a fiber.
*
* We use nanoFifoObj queue to put and get data.
*
* RETURNS: N/A
*/

void testIsrFifoFromFiber(void)
{
	void *pGetData;   /* pointer to FIFO object get from the queue */

	TC_PRINT("Test ISR FIFO (invoked from Fiber)\n\n");

	/* This is data pushed by function testFiberFifoGetW */
	_trigger_nano_isr_fifo_get();
	pGetData = isrFifoInfo.data;

	TC_PRINT("ISR FIFO Get from queue1: %p\n", pGetData);
	if (isrFifoInfo.data != pMyFifoData4) {
		retCode = TC_FAIL;
		TCERR2;
		return;
	}

	/* Verify that the queue is empty */
	_trigger_nano_isr_fifo_get();
	pGetData = isrFifoInfo.data;

	if (pGetData != NULL) {
		TC_PRINT("Get from queue1: %p\n", pGetData);
		retCode = TC_FAIL;
		TCERR3;
		return;
	}

	/* Put more item into queue */
	TC_PRINT("\nISR FIFO (running in fiber context) Put Order:\n");
	for (int i=0; i<NUM_FIFO_ELEMENT; i++) {
		isrFifoInfo.data = pPutList1[i];
		TC_PRINT(" %p,", pPutList1[i]);
		_trigger_nano_isr_fifo_put();
	}
	TC_PRINT("\n");

	TC_END_RESULT(retCode);

}  /* testIsrFifoFromFiber */


/*******************************************************************************
*
* testIsrFifoFromTask - test ISR FIFO routines (triggered from task)
*
* This function tests the fifo put and get interfaces in the isr context.
* It is invoked from a task.
*
* We use nanoFifoObj queue to put and get data.
*
* RETURNS: N/A
*/

void testIsrFifoFromTask(void)
{
	void *pGetData;   /* pointer to FIFO object get from the queue */
	void *pPutData;   /* pointer to FIFO object put to queue */
	int count = 0;      /* counter */

	TC_PRINT("Test ISR FIFO (invoked from Task)\n\n");

	/* This is data pushed by function testIsrFifoFromFiber
	 * Get all FIFOs
	 */
	_trigger_nano_isr_fifo_get();
	pGetData = isrFifoInfo.data;

	while (pGetData != NULL) {
		TC_PRINT("Get from queue1: count = %d, ptr is %p\n", count, pGetData);
		if ((count >= NUM_FIFO_ELEMENT) || (pGetData != pPutList1[count])) {
			TCERR1(count);
			retCode = TC_FAIL;
			return;
		}

		/* Get the next element */
		_trigger_nano_isr_fifo_get();
		pGetData = isrFifoInfo.data;
		count++;
	}  /* while */


	/* Put data into queue and get it again */
	pPutData = pPutList2[3];

	isrFifoInfo.data = pPutData;
	_trigger_nano_isr_fifo_put();
	isrFifoInfo.data = NULL;    /* force data to a new value */
	/* Get data from queue */
	_trigger_nano_isr_fifo_get();
	pGetData = isrFifoInfo.data;
	/* Verify data */
	if (pGetData != pPutData) {
		retCode = TC_FAIL;
		TCERR2;
		return;
	} else {
		TC_PRINT("\nTest ISR FIFO (invoked from Task) - put %p and get back %p\n",
				 pPutData, pGetData);
	}

	TC_END_RESULT(retCode);
}  /* testIsrFifoFromTask */

/*******************************************************************************
*
* fiber2 - entry point for the second fiber
*
* RETURNS: N/A
*/

void fiber2(void)
{
	void *pData;    /* pointer to FIFO object from the queue */

	/* Wait for fiber2 to be activated */

	nano_fiber_sem_take_wait(&nanoSemObj2);

	/* Wait for data to be added to <nanoFifoObj> */
	pData = nano_fiber_fifo_get_wait(&nanoFifoObj);
	if (pData != pPutList1[1]) {
		TC_ERROR("fiber2 (1) - expected 0x%x, got 0x%x\n",
				 pPutList1[1], pData);
		retCode = TC_FAIL;
		return;
	}

	/* Wait for data to be added to <nanoFifoObj2> by fiber3 */
	pData = nano_fiber_fifo_get_wait(&nanoFifoObj2);
	if (pData != pPutList2[1]) {
		TC_ERROR("fiber2 (2) - expected 0x%x, got 0x%x\n",
				 pPutList2[1], pData);
		retCode = TC_FAIL;
		return;
	}

	nano_fiber_sem_take_wait(&nanoSemObj2);   /* Wait for fiber2 to be reactivated */

	/* Fiber #2 has been reactivated by main task */
	for (int i = 0; i < 4; i++) {
		pData = nano_fiber_fifo_get_wait(&nanoFifoObj);
		if (pData != pPutList1[i]) {
			TC_ERROR("fiber2 (3) - iteration %d expected 0x%x, got 0x%x\n",
					 i, pPutList1[i], pData);
			retCode = TC_FAIL;
			return;
		}
	}

	nano_fiber_sem_give(&nanoSemObjTask); /* Wake main task */
	nano_fiber_sem_take_wait(&nanoSemObj2);   /* Wait for fiber2 to be reactivated */

	testFiberFifoGetW();
	PRINT_LINE;
	testIsrFifoFromFiber();

	TC_END_RESULT(retCode);
}  /* fiber2 */

/*******************************************************************************
*
* fiber3 - entry point for the third fiber
*
* RETURNS: N/A
*/

void fiber3(void)
{
	void *pData;

	/* Wait for fiber3 to be activated */
	nano_fiber_sem_take_wait(&nanoSemObj3);

	/* Put two items onto <nanoFifoObj2> to unblock fibers #1 and #2. */
	nano_fiber_fifo_put(&nanoFifoObj2, pPutList2[0]);    /* Wake fiber1 */
	nano_fiber_fifo_put(&nanoFifoObj2, pPutList2[1]);    /* Wake fiber2 */

	/* Wait for fiber3 to be re-activated */
	nano_fiber_sem_take_wait(&nanoSemObj3);

	/* Immediately get the data from <nanoFifoObj2>. */
	pData = nano_fiber_fifo_get_wait(&nanoFifoObj2);
	if (pData != pPutList2[0]) {
		retCode = TC_FAIL;
		TC_ERROR("fiber3 (1) - got 0x%x from <nanoFifoObj2>, expected 0x%x\n",
				 pData, pPutList2[0]);
	}

	/* Put three items onto the FIFO for the task to get */
	nano_fiber_fifo_put(&nanoFifoObj2, pPutList2[0]);
	nano_fiber_fifo_put(&nanoFifoObj2, pPutList2[1]);
	nano_fiber_fifo_put(&nanoFifoObj2, pPutList2[2]);

	/* Sleep for 2 seconds */
	nano_fiber_timer_start(&timer, SECONDS(2));
	nano_fiber_timer_wait(&timer);

	/* Put final item onto the FIFO for the task to get */
	nano_fiber_fifo_put(&nanoFifoObj2, pPutList2[3]);

	/* Wait for fiber3 to be re-activated (not expected to occur) */
	nano_fiber_sem_take_wait(&nanoSemObj3);
}


/*******************************************************************************
*
* testTaskFifoGetW - test the nano_task_fifo_get_wait() interface
*
* This is in the task context.  It puts data to nanoFifoObj2 queue and gets
* data from nanoFifoObj queue.
*
* RETURNS: N/A
*/

void testTaskFifoGetW(void)
{
	void *pGetData;   /* pointer to FIFO object get from the queue */
	void *pPutData;   /* pointer to FIFO object to put to the queue */

	PRINT_LINE;
	TC_PRINT("Test Task FIFO Get Wait Interfaces\n\n");
	pPutData = pMyFifoData1;
	TC_PRINT("TASK FIFO Put to queue2: %p\n", pPutData);
	nano_task_fifo_put(&nanoFifoObj2, pPutData);

	/* Activate fiber2 */
	nano_task_sem_give(&nanoSemObj2);

	pGetData = nano_task_fifo_get_wait(&nanoFifoObj);
	TC_PRINT("TASK FIFO Get from queue1: %p\n", pGetData);
	/* Verify results */
	if (pGetData != pMyFifoData2) {
		retCode = TC_FAIL;
		TCERR2;
		return;
	}

	pPutData = pMyFifoData3;
	TC_PRINT("TASK FIFO Put to queue2: %p\n", pPutData);
	nano_task_fifo_put(&nanoFifoObj2, pPutData);

	TC_END_RESULT(retCode);
} /* testTaskFifoGetW */

/*******************************************************************************
*
* initNanoObjects - initialize nanokernel objects
*
* This routine initializes the nanokernel objects used in the FIFO tests.
*
* RETURNS: N/A
*/

void initNanoObjects(void)
{
	struct isrInitInfo i = {
	{isr_fifo_put, isr_fifo_get},
	{&isrFifoInfo, &isrFifoInfo},
	};

	(void)initIRQ(&i);

	nano_fifo_init(&nanoFifoObj);
	nano_fifo_init(&nanoFifoObj2);

	nano_sem_init(&nanoSemObj1);
	nano_sem_init(&nanoSemObj2);
	nano_sem_init(&nanoSemObj3);
	nano_sem_init(&nanoSemObjTask);

	nano_timer_init(&timer, timerData);
} /* initNanoObjects */

/*******************************************************************************
*
* main - entry point to FIFO tests
*
* This is the entry point to the FIFO tests.
*
* RETURNS: N/A
*/

void main(void)
{
	void   *pData;      /* pointer to FIFO object get from the queue */
	int     count = 0;  /* counter */

	TC_START("Test Nanokernel FIFO");

	/* Initialize the FIFO queues and semaphore */
	initNanoObjects();

	/* Create and start the three (3) fibers. */

	task_fiber_start(&fiberStack1[0], STACKSIZE, (nano_fiber_entry_t) fiber1,
					 0, 0, 7, 0);

	task_fiber_start(&fiberStack2[0], STACKSIZE, (nano_fiber_entry_t) fiber2,
					 0, 0, 7, 0);

	task_fiber_start(&fiberStack3[0], STACKSIZE, (nano_fiber_entry_t) fiber3,
					 0, 0, 7, 0);

	/*
	 * The three fibers have each blocked on a different semaphore.  Giving
	 * the semaphore nanoSemObjX will unblock fiberX (where X = {1, 2, 3}).
	 *
	 * Activate fibers #1 and #2.  They will each block on nanoFifoObj.
	 */

	nano_task_sem_give(&nanoSemObj1);
	nano_task_sem_give(&nanoSemObj2);

	/* Put two items into <nanoFifoObj> to unblock fibers #1 and #2. */
	nano_task_fifo_put(&nanoFifoObj, pPutList1[0]);    /* Wake fiber1 */
	nano_task_fifo_put(&nanoFifoObj, pPutList1[1]);    /* Wake fiber2 */

	/* Activate fiber #3 */
	nano_task_sem_give(&nanoSemObj3);

	/*
	 * All three fibers should be blocked on their semaphores.  Put data into
	 * <nanoFifoObj2>.  Fiber #3 will read it after it is reactivated.
	 */

	nano_task_fifo_put(&nanoFifoObj2, pPutList2[0]);
	nano_task_sem_give(&nanoSemObj3);    /* Reactivate fiber #3 */

	for (int i = 0; i < 4; i++) {
		pData = nano_task_fifo_get_wait(&nanoFifoObj2);
		if (pData != pPutList2[i]) {
			TC_ERROR("nano_task_fifo_get_wait() expected 0x%x, got 0x%x\n",
					 pPutList2[i], pData);
			goto exit;
		}
	}

	/* Add items to <nanoFifoObj> for fiber #2 */
	for (int i = 0; i < 4; i++) {
		nano_task_fifo_put(&nanoFifoObj, pPutList1[i]);
	}

	nano_task_sem_give(&nanoSemObj2);   /* Activate fiber #2 */

	/* Wait for fibers to finish */
	nano_task_sem_take_wait(&nanoSemObjTask);

	if (retCode == TC_FAIL) {
		goto exit;
	}

	/*
	 * Entries in the FIFO queue have to be unique.
	 * Put data to queue.
	 */

	TC_PRINT("Test Task FIFO Put\n");
	TC_PRINT("\nTASK FIFO Put Order: ");
	for (int i=0; i<NUM_FIFO_ELEMENT; i++) {
		nano_task_fifo_put(&nanoFifoObj, pPutList1[i]);
		TC_PRINT(" %p,", pPutList1[i]);
	}
	TC_PRINT("\n");

	PRINT_LINE;

	nano_task_sem_give(&nanoSemObj1);      /* Activate fiber1 */

	if (retCode == TC_FAIL) {
		goto exit;
	}

	/*
	 * Wait for fiber1 to complete execution. (Using a semaphore gives
	 * the fiber the freedom to do blocking-type operations if it wants to.)
	 */
	nano_task_sem_take_wait(&nanoSemObjTask);

	TC_PRINT("Test Task FIFO Get\n");

	/* Get all FIFOs */
	while ((pData = nano_task_fifo_get(&nanoFifoObj)) != NULL) {
		TC_PRINT("TASK FIFO Get: count = %d, ptr is %p\n", count, pData);
		if ((count >= NUM_FIFO_ELEMENT) || (pData != pPutList2[count])) {
			TCERR1(count);
			retCode = TC_FAIL;
			goto exit;
		}
		count++;
	}

	/* Test FIFO Get Wait interfaces*/
	testTaskFifoGetW();
	PRINT_LINE;

	testIsrFifoFromTask();
	PRINT_LINE;

exit:
	TC_END_RESULT(retCode);
	TC_END_REPORT(retCode);
}
