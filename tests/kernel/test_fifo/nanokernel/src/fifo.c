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
 * @file
 * @brief Test nanokernel FIFO APIs
 *
 * This module tests four basic scenarios with the usage of the following FIFO
 * routines:
 *
 * nano_fiber_fifo_get, nano_fiber_fifo_put
 * nano_task_fifo_get, nano_task_fifo_put
 * nano_isr_fifo_get, nano_isr_fifo_put
 *
 * Scenario #1
 * Task enters items into a queue, starts the fiber and waits for a semaphore.
 * Fiber extracts all items from the queue and enters some items back into
 * the queue.  Fiber gives the semaphore for task to continue.  Once the control
 * is returned back to task, task extracts all items from the queue.

 * Scenario #2
 * Task enters an item into queue2, starts a fiber and extract an item from
 * queue1 once the item is there.  The fiber will extract an item from queue2
 * once the item is there and and enter an item to queue1.  The flow of control
 * goes from task to fiber and so forth.

 * Scenario #3
 * Tests the ISR interfaces.  Function testIsrFifoFromFiber gets an item from
 * the fifo queue in ISR context.  It then enters four items into the queue
 * and finishes execution. Control is returned back to function
 * testTaskFifoGetW which also finished it's execution and returned to main.
 * Finally function testIsrFifoFromTask is run and it gets all data from
 * the queue and puts and gets one last item to the queue.  All these are run
 * in ISR context.
 *
 * Scenario #4:
 * Timeout scenarios with multiple FIFOs and fibers.
 */

#include <zephyr.h>
#include <tc_util.h>
#include <misc/__assert.h>
#include <misc/util.h>
#include <irq_offload.h>
#include <sections.h>

#include <util_test_common.h>

#define FIBER_STACKSIZE		384
#define NUM_FIFO_ELEMENT	4
#define INVALID_DATA		NULL

#define TCERR1(count)  TC_ERROR("Didn't get back correct FIFO, count %d\n", count)
#define TCERR2         TC_ERROR("Didn't get back correct FIFO\n")
#define TCERR3         TC_ERROR("The queue should be empty!\n")

struct isr_fifo_info {
	struct nano_fifo *fifo_ptr;  /* FIFO */
	void *data;     /* pointer to data to add */
};

char __stack fiberStack1[FIBER_STACKSIZE];
char __stack fiberStack2[FIBER_STACKSIZE];
char __stack fiberStack3[FIBER_STACKSIZE];

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

/* for put_list tests */
struct nano_fifo fifo_list;
struct nano_sem sem_list;

struct packet_list {
	void *next;
	int n;
};

int retCode = TC_PASS;

static struct isr_fifo_info isrFifoInfo = {&nanoFifoObj, NULL};

void fiber1(void);
void fiber2(void);
void fiber3(void);

void initNanoObjects(void);
void testTaskFifoGetW(void);

extern int test_fifo_timeout(void);

/**
 *
 * @brief Add an item to a FIFO
 *
 * This routine is the ISR handler for _trigger_nano_isr_fifo_put().  It adds
 * an item to the FIFO in the context of an ISR.
 *
 * @param parameter    pointer to ISR handler parameter
 *
 * @return N/A
 */

void isr_fifo_put(void *parameter)
{
	struct isr_fifo_info *pInfo = (struct isr_fifo_info *) parameter;

	nano_isr_fifo_put(pInfo->fifo_ptr, pInfo->data);
}

static void _trigger_nano_isr_fifo_put(void)
{
	irq_offload(isr_fifo_put, &isrFifoInfo);
}


/**
 *
 * @brief Get an item from a FIFO
 *
 * This routine is the ISR handler for _trigger_nano_isr_fifo_get().  It gets
 * an item from the FIFO in the context of an ISR.
 *
 * @param parameter    pointer to ISR handler parameter
 *
 * @return N/A
 */

void isr_fifo_get(void *parameter)
{
	struct isr_fifo_info *pInfo = (struct isr_fifo_info *) parameter;

	pInfo->data = nano_isr_fifo_get(pInfo->fifo_ptr, TICKS_NONE);
}

static void _trigger_nano_isr_fifo_get(void)
{
	irq_offload(isr_fifo_get, &isrFifoInfo);
}

/**
 *
 * @brief Entry point for the first fiber
 *
 * @return N/A
 */

void fiber1(void)
{
	void   *pData;      /* pointer to FIFO object get from the queue */
	int     count = 0;  /* counter */

	/* Wait for fiber1 to be activated. */
	nano_fiber_sem_take(&nanoSemObj1, TICKS_UNLIMITED);

	/* Wait for data to be added to <nanoFifoObj> by task */
	pData = nano_fiber_fifo_get(&nanoFifoObj, TICKS_UNLIMITED);
	if (pData != pPutList1[0]) {
		TC_ERROR("fiber1 (1) - expected %p, got %p\n",
				 pPutList1[0], pData);
		retCode = TC_FAIL;
		return;
	}

	/* Wait for data to be added to <nanoFifoObj2> by fiber3 */
	pData = nano_fiber_fifo_get(&nanoFifoObj2, TICKS_UNLIMITED);
	if (pData != pPutList2[0]) {
		TC_ERROR("fiber1 (2) - expected %p, got %p\n",
				 pPutList2[0], pData);
		retCode = TC_FAIL;
		return;
	}

	/* Wait for fiber1 to be reactivated */
	nano_fiber_sem_take(&nanoSemObj1, TICKS_UNLIMITED);

	TC_PRINT("Test Fiber FIFO Get\n\n");
	/* Get all FIFOs */
	while ((pData = nano_fiber_fifo_get(&nanoFifoObj, TICKS_NONE)) != NULL) {
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
	for (int i = 0; i < NUM_FIFO_ELEMENT; i++) {
		nano_fiber_fifo_put(&nanoFifoObj, pPutList2[i]);
		TC_PRINT(" %p,", pPutList2[i]);
	}
	TC_PRINT("\n");
	PRINT_LINE;

	/* Give semaphore to allow the main task to run */
	nano_fiber_sem_give(&nanoSemObjTask);

} /* fiber1 */


/**
 *
 * @brief Test the nano_fiber_fifo_get(TICKS_UNLIMITED) interface
 *
 * This function tests the fifo put and get wait interfaces in a fiber.
 * It gets data from nanoFifoObj2 queue and puts data to nanoFifoObj queue.
 *
 * @return N/A
 */

void testFiberFifoGetW(void)
{
	void *pGetData;   /* pointer to FIFO object get from the queue */
	void *pPutData;   /* pointer to FIFO object to put to the queue */

	TC_PRINT("Test Fiber FIFO Get Wait Interfaces\n\n");
	pGetData = nano_fiber_fifo_get(&nanoFifoObj2, TICKS_UNLIMITED);
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

	pGetData = nano_fiber_fifo_get(&nanoFifoObj2, TICKS_UNLIMITED);
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


/**
 *
 * @brief Test ISR FIFO routines (triggered from fiber)
 *
 * This function tests the fifo put and get interfaces in the ISR context.
 * It is invoked from a fiber.
 *
 * We use nanoFifoObj queue to put and get data.
 *
 * @return N/A
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
	TC_PRINT("\nISR FIFO (running in fiber) Put Order:\n");
	for (int i = 0; i < NUM_FIFO_ELEMENT; i++) {
		isrFifoInfo.data = pPutList1[i];
		TC_PRINT(" %p,", pPutList1[i]);
		_trigger_nano_isr_fifo_put();
	}
	TC_PRINT("\n");

	TC_END_RESULT(retCode);

}  /* testIsrFifoFromFiber */


/**
 *
 * @brief Test ISR FIFO routines (triggered from task)
 *
 * This function tests the fifo put and get interfaces in the ISR context.
 * It is invoked from a task.
 *
 * We use nanoFifoObj queue to put and get data.
 *
 * @return N/A
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
	}
	TC_PRINT("\nTest ISR FIFO (invoked from Task) - put %p and get back %p\n",
		pPutData, pGetData);

	TC_END_RESULT(retCode);
}  /* testIsrFifoFromTask */

/**
 *
 * @brief Entry point for the second fiber
 *
 * @return N/A
 */

void fiber2(void)
{
	void *pData;    /* pointer to FIFO object from the queue */

	/* Wait for fiber2 to be activated */

	nano_fiber_sem_take(&nanoSemObj2, TICKS_UNLIMITED);

	/* Wait for data to be added to <nanoFifoObj> */
	pData = nano_fiber_fifo_get(&nanoFifoObj, TICKS_UNLIMITED);
	if (pData != pPutList1[1]) {
		TC_ERROR("fiber2 (1) - expected %p, got %p\n",
				 pPutList1[1], pData);
		retCode = TC_FAIL;
		return;
	}

	/* Wait for data to be added to <nanoFifoObj2> by fiber3 */
	pData = nano_fiber_fifo_get(&nanoFifoObj2, TICKS_UNLIMITED);
	if (pData != pPutList2[1]) {
		TC_ERROR("fiber2 (2) - expected %p, got %p\n",
				 pPutList2[1], pData);
		retCode = TC_FAIL;
		return;
	}

	/* Wait for fiber2 to be reactivated */
	nano_fiber_sem_take(&nanoSemObj2, TICKS_UNLIMITED);

	/* Fiber #2 has been reactivated by main task */
	for (int i = 0; i < 4; i++) {
		pData = nano_fiber_fifo_get(&nanoFifoObj, TICKS_UNLIMITED);
		if (pData != pPutList1[i]) {
			TC_ERROR("fiber2 (3) - iteration %d expected %p, got %p\n",
					 i, pPutList1[i], pData);
			retCode = TC_FAIL;
			return;
		}
	}

	nano_fiber_sem_give(&nanoSemObjTask); /* Wake main task */
	/* Wait for fiber2 to be reactivated */
	nano_fiber_sem_take(&nanoSemObj2, TICKS_UNLIMITED);

	testFiberFifoGetW();
	PRINT_LINE;
	testIsrFifoFromFiber();

	TC_END_RESULT(retCode);
}  /* fiber2 */

/**
 *
 * @brief Entry point for the third fiber
 *
 * @return N/A
 */

void fiber3(void)
{
	void *pData;

	/* Wait for fiber3 to be activated */
	nano_fiber_sem_take(&nanoSemObj3, TICKS_UNLIMITED);

	/* Put two items onto <nanoFifoObj2> to unblock fibers #1 and #2. */
	nano_fiber_fifo_put(&nanoFifoObj2, pPutList2[0]);    /* Wake fiber1 */
	nano_fiber_fifo_put(&nanoFifoObj2, pPutList2[1]);    /* Wake fiber2 */

	/* Wait for fiber3 to be re-activated */
	nano_fiber_sem_take(&nanoSemObj3, TICKS_UNLIMITED);

	/* Immediately get the data from <nanoFifoObj2>. */
	pData = nano_fiber_fifo_get(&nanoFifoObj2, TICKS_UNLIMITED);
	if (pData != pPutList2[0]) {
		retCode = TC_FAIL;
		TC_ERROR("fiber3 (1) - got %p from <nanoFifoObj2>, expected %p\n",
				 pData, pPutList2[0]);
	}

	/* Put three items onto the FIFO for the task to get */
	nano_fiber_fifo_put(&nanoFifoObj2, pPutList2[0]);
	nano_fiber_fifo_put(&nanoFifoObj2, pPutList2[1]);
	nano_fiber_fifo_put(&nanoFifoObj2, pPutList2[2]);

	/* Sleep for 2 seconds */
	nano_fiber_timer_start(&timer, SECONDS(2));
	nano_fiber_timer_test(&timer, TICKS_UNLIMITED);

	/* Put final item onto the FIFO for the task to get */
	nano_fiber_fifo_put(&nanoFifoObj2, pPutList2[3]);

	/* Wait for fiber3 to be re-activated (not expected to occur) */
	nano_fiber_sem_take(&nanoSemObj3, TICKS_UNLIMITED);
}


/**
 *
 * @brief Test the nano_task_fifo_get(TICKS_UNLIMITED) interface
 *
 * This is in a task.  It puts data to nanoFifoObj2 queue and gets
 * data from nanoFifoObj queue.
 *
 * @return N/A
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

	pGetData = nano_task_fifo_get(&nanoFifoObj, TICKS_UNLIMITED);
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

/**
 *
 * @brief Initialize nanokernel objects
 *
 * This routine initializes the nanokernel objects used in the FIFO tests.
 *
 * @return N/A
 */

void initNanoObjects(void)
{
	nano_fifo_init(&nanoFifoObj);
	nano_fifo_init(&nanoFifoObj2);
	nano_fifo_init(&fifo_list);

	nano_sem_init(&nanoSemObj1);
	nano_sem_init(&nanoSemObj2);
	nano_sem_init(&nanoSemObj3);
	nano_sem_init(&nanoSemObjTask);
	nano_sem_init(&sem_list);

	nano_timer_init(&timer, timerData);

} /* initNanoObjects */

/* fifo_put_list */

sys_slist_t list;

struct packet_list packets[8];

char __stack __noinit stacks_list[2][512];

void fiber_list_0(int a, int b)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);

	struct packet_list *p;

	p = nano_fiber_fifo_get(&fifo_list, TICKS_UNLIMITED);
	if (p->n != 0) {
		retCode = TC_FAIL;
		TC_ERROR(" *** %s did not get expected element %d\n",
			 __func__, 0);
		return;
	}

	printk("%s got element %d, as expected\n", __func__, 0);

	p = nano_fiber_fifo_get(&fifo_list, TICKS_UNLIMITED);
	if (p->n != 2) {
		retCode = TC_FAIL;
		TC_ERROR(" *** %s did not get expected element %d\n",
			 __func__, 2);
		return;
	}

	printk("%s got element %d, as expected\n", __func__, 2);

	sys_slist_init(&list);

	for (int ii = 3; ii < 8; ii++) {
		sys_slist_append(&list, (sys_snode_t *)&packets[ii]);
	}

	fiber_yield(); /* collegue takes 1 */

	nano_fiber_fifo_put_slist(&fifo_list, &list);

	fiber_yield(); /* collegue takes 3 */

	/* I take the rest */
	for (int ii = 4; ii < 8; ii++) {
		p = nano_fiber_fifo_get(&fifo_list, SECONDS(1));
		if (p->n != ii) {
			TC_ERROR(" *** %s did not get expected element %d\n",
				 __func__, ii);
			retCode = TC_FAIL;
			return;
		}

		printk("%s got element %d, as expected\n",
			__func__, ii);
	}

	nano_fiber_sem_give(&sem_list);
}

static void fiber_list_1(int a, int b)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);

	struct packet_list *p;

	p = nano_fiber_fifo_get(&fifo_list, TICKS_UNLIMITED);
	if (p->n != 1) {
		retCode = TC_FAIL;
		TC_ERROR(" *** %s did not get expected element %d\n",
			 __func__, 1);
		return;
	}

	printk("%s got element %d, as expected\n", __func__, 1);

	p = nano_fiber_fifo_get(&fifo_list, TICKS_UNLIMITED);
	if (p->n != 3) {
		retCode = TC_FAIL;
		TC_ERROR(" *** %s did not get expected element %d\n",
			 __func__, 3);
		return;
	}

	printk("%s got element %d, as expected\n", __func__, 3);
}

static void test_fifo_put_list(void)
{
	PRINT_LINE;
	task_fiber_start(stacks_list[0], 512, fiber_list_0, 0, 0, 7, 0);
	task_fiber_start(stacks_list[1], 512, fiber_list_1, 0, 0, 7, 0);

	for (int ii = 0; ii < 8; ii++) {
		packets[ii].n = ii;
	}

	packets[0].next = &packets[1];
	packets[1].next = &packets[2];
	packets[2].next = NULL;

	nano_task_fifo_put_list(&fifo_list, &packets[0], &packets[2]);

	nano_task_sem_take(&sem_list, SECONDS(5));

	TC_END_RESULT(retCode);
}

/**
 *
 * @brief Entry point to FIFO tests
 *
 * This is the entry point to the FIFO tests.
 *
 * @return N/A
 */

void main(void)
{
	void   *pData;      /* pointer to FIFO object get from the queue */
	int     count = 0;  /* counter */

	TC_START("Test Nanokernel FIFO");

	/* Initialize the FIFO queues and semaphore */
	initNanoObjects();

	/* Create and start the three (3) fibers. */

	task_fiber_start(&fiberStack1[0], FIBER_STACKSIZE, (nano_fiber_entry_t) fiber1,
					 0, 0, 7, 0);

	task_fiber_start(&fiberStack2[0], FIBER_STACKSIZE, (nano_fiber_entry_t) fiber2,
					 0, 0, 7, 0);

	task_fiber_start(&fiberStack3[0], FIBER_STACKSIZE, (nano_fiber_entry_t) fiber3,
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
		pData = nano_task_fifo_get(&nanoFifoObj2, TICKS_UNLIMITED);
		if (pData != pPutList2[i]) {
			TC_ERROR("nano_task_fifo_get() expected %p, got %p\n",
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
	nano_task_sem_take(&nanoSemObjTask, TICKS_UNLIMITED);

	if (retCode == TC_FAIL) {
		goto exit;
	}

	/*
	 * Entries in the FIFO queue have to be unique.
	 * Put data to queue.
	 */

	TC_PRINT("Test Task FIFO Put\n");
	TC_PRINT("\nTASK FIFO Put Order: ");
	for (int i = 0; i < NUM_FIFO_ELEMENT; i++) {
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
	nano_task_sem_take(&nanoSemObjTask, TICKS_UNLIMITED);

	TC_PRINT("Test Task FIFO Get\n");

	/* Get all FIFOs */
	while ((pData = nano_task_fifo_get(&nanoFifoObj, TICKS_NONE)) != NULL) {
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

	/* test timeouts */
	if (test_fifo_timeout() != TC_PASS) {
		retCode = TC_FAIL;
		goto exit;
	}
	PRINT_LINE;

	/* test put_list/slist */

	test_fifo_put_list();

exit:
	TC_END_RESULT(retCode);
	TC_END_REPORT(retCode);
}
