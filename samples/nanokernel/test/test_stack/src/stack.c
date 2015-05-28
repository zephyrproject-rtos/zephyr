/* stack.c - test nanokernel STACK APIs under VxMicro */

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
This module tests three basic scenarios with the usage of the following STACK
routines:

   nano_fiber_stack_pop, nano_fiber_stack_pop_wait, nano_fiber_stack_push
   nano_task_stack_pop, nano_task_stack_pop_wait, nano_task_stack_push
   nano_isr_stack_pop, nano_isr_stack_push

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
  Tests the ISR interfaces.  Fiber2 pops an item from queue1 in ISR context.
It then enters four items into the queue and finishes execution.  Control
is returned back to function testTaskStackPopW which also finished it's execution
and returned to main.  Finally function testIsrStackFromTask is run and
it popped all data from queue1, push and pop one last item to the queue.  All
these are run in ISR context.
*/

/* includes */

#include <tc_util.h>
#include <arch/cpu.h>

/* test uses 2 software IRQs */
#define NUM_SW_IRQS 2

#include <irq_test_common.h>
#include <util_test_common.h>

/* defines */
#define STACKSIZE               2048
#define NUM_STACK_ELEMENT       4
#define STARTNUM                1       /* Used to compute data to put in the stack */
#define MULTIPLIER              100     /* Used to compute data to put in the stack */
#define MYNUMBER                50      /* Used to compute data to put in the stack */
#define INVALID_DATA            0       /* Invalid data on stack */

#define TCERR1(count)  TC_ERROR("Didn't get back correct data, count %d\n", count)
#define TCERR2         TC_ERROR("Didn't get back correct data\n")
#define TCERR3         TC_ERROR("The stack should be empty!\n")

/* typedefs */

typedef struct {
	struct nano_stack *channel;      /* STACK channel */
	uint32_t           data;         /* data to add */
} ISR_STACK_INFO;


/* globals */

char fiberStack1[STACKSIZE];
char fiberStack2[STACKSIZE];
char fiberStack3[STACKSIZE];

struct nano_timer timer;
struct nano_stack nanoStackObj;
struct nano_stack nanoStackObj2;
struct nano_sem   nanoSemObj; /* Used for transferring control between
                               * main and fiber1
                               */

uint32_t myData[NUM_STACK_ELEMENT];
uint32_t myIsrData[NUM_STACK_ELEMENT];  /* Data used for testing
                                         * nano_isr_stack_push and
                                         * nanoIsrStatckPop interfaces
                                         */
uint32_t stack1[NUM_STACK_ELEMENT];
uint32_t stack2[NUM_STACK_ELEMENT];

void *timerData[1];
int retCode = TC_PASS;

/* locals */

static ISR_STACK_INFO  isrStackInfo = {&nanoStackObj, 0};

static void (*_trigger_nano_isr_stack_push)(void) = (vvfn)sw_isr_trigger_0;
static void (*_trigger_nano_isr_stack_pop)(void) = (vvfn)sw_isr_trigger_1;

/* forward declarations */

void initData(void);
void fiber1(void);
void fiber2(void);
void initNanoObjects(void);
void testFiberStackPopW(void);
void testTaskStackPopW(void);
/* Isr related functions */
void isr_stack_push(void *parameter);
void isr_stack_pop(void *parameter);
void testIsrStackFromFiber(void);
void testIsrStackFromTask(void);


/*******************************************************************************
*
* initData
*
* Initialize myData and myIsrData arrays.
*
* RETURNS: none
*/

void initData(void)
{
	for (int i=0; i< NUM_STACK_ELEMENT; i++) {
		myData[i] = (STARTNUM + i) * MULTIPLIER;
		myIsrData[i] = myData[i] + MYNUMBER;
	}
} /* initData */

/*******************************************************************************
*
* isr_stack_push - add an item to a STACK
*
* This routine is the ISR handler for _trigger_nano_isr_stack_push().  It adds
* an item to the STACK in the context of an ISR.
*
* \param parameter    pointer to ISR handler parameter
*
* RETURNS: N/A
*/

void isr_stack_push(void *parameter)
{
	ISR_STACK_INFO *pInfo = (ISR_STACK_INFO *) parameter;

	nano_isr_stack_push(pInfo->channel, pInfo->data);

}  /* isr_stack_push */

/*******************************************************************************
*
* isr_stack_pop - get an item from a STACK
*
* This routine is the ISR handler for _trigger_nano_isr_stack_pop().  It gets
* an item from the STACK in the context of an ISR.  If the queue is empty,
* it sets data to INVALID_DATA.
*
* \param parameter    pointer to ISR handler parameter
*
* RETURNS: N/A
*/

void isr_stack_pop(void *parameter)
{
	ISR_STACK_INFO *pInfo = (ISR_STACK_INFO *) parameter;

	if (nano_isr_stack_pop(pInfo->channel, &(pInfo->data)) == 0) {
		/* the stack is empty, set data to INVALID_DATA */
		pInfo->data = INVALID_DATA;
	}

}  /* isr_stack_pop */


/*******************************************************************************
*
* fiber1
*
* This is the fiber started from the main task.  Gets all items from
* the STACK queue and puts four items back to the STACK queue.  Control is
* transferred back to the main task.
*
* RETURNS: N/A
*/

void fiber1(void)
{
	uint32_t    data;        /* data used to put and get from the stack queue */
	int         count = 0;   /* counter */

	TC_PRINT("Test Fiber STACK Pop\n\n");
	/* Get all data */
	while (nano_fiber_stack_pop(&nanoStackObj, &data) != 0) {
		TC_PRINT("FIBER STACK Pop: count = %d, data is %d\n", count, data);
		if ((count >= NUM_STACK_ELEMENT) || (data != myData[NUM_STACK_ELEMENT - 1 - count])) {
			TCERR1(count);
			retCode = TC_FAIL;
			return;
		}
		count++;
	}

	TC_END_RESULT(retCode);
	PRINT_LINE;

	/* Put data */
	TC_PRINT("Test Fiber STACK Push\n");
	TC_PRINT("\nFIBER STACK Put Order: ");
	for (int i=NUM_STACK_ELEMENT; i>0; i--) {
		nano_fiber_stack_push(&nanoStackObj, myData[i-1]);
		TC_PRINT(" %d,", myData[i-1]);
	}
	TC_PRINT("\n");
	PRINT_LINE;

	/* Give semaphore to allow the main task to run */
	nano_fiber_sem_give(&nanoSemObj);

} /* fiber1 */



/*******************************************************************************
*
* testFiberStackPopW
*
* This function tests the stack push and pop wait interfaces in the fiber context.
* It gets data from nanoStackObj2 queue and puts data to nanoStackObj queue.
*
* RETURNS: N/A
*/

void testFiberStackPopW(void)
{
	uint32_t  data;     /* data used to put and get from the stack queue */

	TC_PRINT("Test Fiber STACK Pop Wait Interfaces\n\n");
	data = nano_fiber_stack_pop_wait(&nanoStackObj2);
	TC_PRINT("FIBER STACK Pop from queue2: %d\n", data);
	/* Verify results */
	if (data != myData[0]) {
		retCode = TC_FAIL;
		TCERR2;
		return;
	}

	data = myData[1];
	TC_PRINT("FIBER STACK Push to queue1: %d\n", data);
	nano_fiber_stack_push(&nanoStackObj, data);

	data = nano_fiber_stack_pop_wait(&nanoStackObj2);
	TC_PRINT("FIBER STACK Pop from queue2: %d\n", data);
	/* Verify results */
	if (data != myData[2]) {
		retCode = TC_FAIL;
		TCERR2;
		return;
	}

	data = myData[3];
	TC_PRINT("FIBER STACK Push to queue1: %d\n", data);
	nano_fiber_stack_push(&nanoStackObj, data);

	TC_END_RESULT(retCode);

}  /* testFiberStackPopW */

/*******************************************************************************
*
* testIsrStackFromFiber
*
* This function tests the stack push and pop interfaces in the isr context.
* It is invoked from a fiber.
*
* We use nanoStackObj queue to push and pop data.
*
* RETURNS: N/A
*/

void testIsrStackFromFiber(void)
{
	uint32_t  result = INVALID_DATA;     /* data used to put and get from the stack queue */

	TC_PRINT("Test ISR STACK (invoked from Fiber)\n\n");

	/* This is data pushed by function testFiberStackPopW */
	_trigger_nano_isr_stack_pop();
	result = isrStackInfo.data;
	if (result != INVALID_DATA) {
		TC_PRINT("ISR STACK (running in fiber context) Pop from queue1: %d\n", result);
		if (result != myData[3]) {
			retCode = TC_FAIL;
			TCERR2;
			return;
		}
	}

	/* Verify that the STACK is empty */
	_trigger_nano_isr_stack_pop();
	result = isrStackInfo.data;
	if (result != INVALID_DATA) {
		 TC_PRINT("Pop from queue1: %d\n", result);
		 retCode = TC_FAIL;
		 TCERR3;
		 return;
	}

	/* Put more data into STACK */
	TC_PRINT("ISR STACK (running in fiber context) Push to queue1:\n");
	for (int i=0; i<NUM_STACK_ELEMENT; i++) {
		isrStackInfo.data = myIsrData[i];
		TC_PRINT("  %d, ", myIsrData[i]);
		_trigger_nano_isr_stack_push();
	}
	TC_PRINT("\n");

	/* Set variable to INVALID_DATA to ensure [data] changes */
	isrStackInfo.data = INVALID_DATA;

	TC_END_RESULT(retCode);

}  /* testIsrStackFromFiber */

/*******************************************************************************
*
* testIsrStackFromTask
*
* This function tests the stack push and pop interfaces in the isr context.
* It is invoked from a task.
*
* We use nanoStackObj queue to push and pop data.
*
* RETURNS: N/A
*/

void testIsrStackFromTask(void)
{
	uint32_t  result = INVALID_DATA;     /* data used to put and get from the stack queue */
	int       count  = 0;

	TC_PRINT("Test ISR STACK (invoked from Task)\n\n");

	/* Get all data */
	_trigger_nano_isr_stack_pop();
	result = isrStackInfo.data;

	while (result != INVALID_DATA) {
		TC_PRINT("  Pop from queue1: count = %d, data is %d\n", count, result);
		if ((count >= NUM_STACK_ELEMENT) || (result != myIsrData[NUM_STACK_ELEMENT - count - 1])) {
			TCERR1(count);
			retCode = TC_FAIL;
			return;
		}  /* if */

		/* Get the next element */
		_trigger_nano_isr_stack_pop();
		result = isrStackInfo.data;
		count++;
	}  /* while */


	/* Put data into stack and get it again */
	isrStackInfo.data = myIsrData[3];
	_trigger_nano_isr_stack_push();
	isrStackInfo.data = INVALID_DATA;   /* force variable to a new value */
	/* Get data from stack */
	_trigger_nano_isr_stack_pop();
	result = isrStackInfo.data;
	/* Verify data */
	if (result != myIsrData[3]) {
		TCERR2;
		retCode = TC_FAIL;
		return;
	} else {
		TC_PRINT("\nTest ISR STACK (invoked from Task) - push %d and pop back %d\n",
				 myIsrData[3], result);
	}

	TC_END_RESULT(retCode);
}

/*******************************************************************************
*
* fiber2
*
* This is the fiber started from the testTaskStackPopW function.
*
* RETURNS: N/A
*/

void fiber2(void)
{
	testFiberStackPopW();
	PRINT_LINE;
	testIsrStackFromFiber();

	TC_END_RESULT(retCode);
}


/*******************************************************************************
*
* testTaskStackPopW
*
* This is in the task context.  It puts data to nanoStackObj2 queue and gets
* data from nanoStackObj queue.
*
* RETURNS: N/A
*/

void testTaskStackPopW(void)
{
	uint32_t  data;     /* data used to put and get from the stack queue */

	PRINT_LINE;
	TC_PRINT("Test STACK Pop Wait Interfaces\n\n");
	data = myData[0];
	TC_PRINT("TASK  STACK Push to queue2: %d\n", data);
	nano_task_stack_push(&nanoStackObj2, data);

	/* Start fiber */
	task_fiber_start(&fiberStack2[0], STACKSIZE,
					 (nano_fiber_entry_t) fiber2, 0, 0, 7, 0);

	data = nano_task_stack_pop_wait(&nanoStackObj);
	TC_PRINT("TASK STACK Pop from queue1: %d\n", data);
	/* Verify results */
	if (data != myData[1]) {
		retCode = TC_FAIL;
		TCERR2;
		return;
	}

	data = myData[2];
	TC_PRINT("TASK STACK Push to queue2: %d\n", data);
	nano_task_stack_push(&nanoStackObj2, data);

	TC_END_RESULT(retCode);
}  /* testTaskStackPopW */

/*******************************************************************************
*
* fiber3 - a fiber to help test nano_task_stack_pop_wait()
*
* This fiber blocks for one second before pushing an item onto the stack.
* The main task, which was waiting for item from the stack then unblocks.
*
* RETURNS: N/A
*/

void fiber3(void)
{
	nano_fiber_timer_start(&timer, SECONDS(1));
	nano_fiber_timer_wait(&timer);
	nano_fiber_stack_push(&nanoStackObj, myData[0]);
}

/*******************************************************************************
*
* initNanoObjects - initialize nanokernel objects
*
* This routine initializes the nanokernel objects used in the STACK tests.
*
* RETURNS: N/A
*/

void initNanoObjects(void)
{
	struct isrInitInfo i = {
	{isr_stack_push, isr_stack_pop},
	{&isrStackInfo, &isrStackInfo},
	};

	(void)initIRQ(&i);

	nano_stack_init(&nanoStackObj,  stack1);
	nano_stack_init(&nanoStackObj2, stack2);
	nano_sem_init(&nanoSemObj);
	nano_timer_init(&timer, timerData);
} /* initNanoObjects */

/*******************************************************************************
*
* main - entry point to STACK tests
*
* This is the entry point to the STACK tests.
*
* RETURNS: N/A
*/

void main(void)
{
	int         count = 0;  /* counter */
	uint32_t    data;       /* data used to put and get from the stack queue */

	TC_START("Test Nanokernel STACK");

	/* Initialize data */
	initData();

	/* Initialize the queues and semaphore */
	initNanoObjects();

	/* Start fiber3 */
	task_fiber_start(&fiberStack3[0], STACKSIZE, (nano_fiber_entry_t) fiber3,
					 0, 0, 7, 0);
	/*
	 * While fiber3 blocks (for one second), wait for an item to be pushed
	 * onto the stack so that it can be popped.  This will put the nanokernel
	 * into an idle state.
	 */

	data = nano_task_stack_pop_wait(&nanoStackObj);
	if (data != myData[0]) {
		TC_ERROR("nano_task_stack_pop_wait() expected 0x%x, but got 0x%x\n",
				 myData[0], data);
		retCode = TC_FAIL;
		goto exit;
	}

	/* Put data */
	TC_PRINT("Test Task STACK Push\n");
	TC_PRINT("\nTASK STACK Put Order: ");
	for (int i=0; i<NUM_STACK_ELEMENT; i++) {
		nano_task_stack_push(&nanoStackObj, myData[i]);
		TC_PRINT(" %d,", myData[i]);
	}
	TC_PRINT("\n");

	PRINT_LINE;

	/* Start fiber */
	task_fiber_start(&fiberStack1[0], STACKSIZE,
					 (nano_fiber_entry_t) fiber1, 0, 0, 7, 0);

	if (retCode == TC_FAIL) {
		goto exit;
	}

	/*
	 * Wait for fiber1 to complete execution. (Using a semaphore gives
	 * the fiber the freedom to do blocking-type operations if it wants to.)
	 *
	 */
	nano_task_sem_take_wait(&nanoSemObj);
	TC_PRINT("Test Task STACK Pop\n");

	/* Get all data */
	while (nano_task_stack_pop(&nanoStackObj, &data) != 0) {
		TC_PRINT("TASK STACK Pop: count = %d, data is %d\n", count, data);
		if ((count >= NUM_STACK_ELEMENT) || (data != myData[count])) {
			TCERR1(count);
			retCode = TC_FAIL;
			goto exit;
		}
		count++;
	}

	/* Test Task Stack Pop Wait interfaces*/
	testTaskStackPopW();

	if (retCode == TC_FAIL) {
		goto exit;
	}

	PRINT_LINE;

	/* Test ISR interfaces */
	testIsrStackFromTask();
	PRINT_LINE;

exit:
	TC_END_RESULT(retCode);
	TC_END_REPORT(retCode);
}
