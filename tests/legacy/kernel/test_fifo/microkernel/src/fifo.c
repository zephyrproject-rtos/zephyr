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
 * @brief Test microkernel FIFO APIs
 *
 * This module tests the following FIFO routines:
 *
 *   task_fifo_put
 *   task_fifo_get
 *   task_fifo_size_get
 *   task_fifo_purge
 *
 * Scenarios tested include:
 * - Check number of elements in queue when queue is empty, full or
 *   while it is being dequeued
 * - Verify the data being dequeued are in correct order
 * - Verify the return codes are correct for the APIs
 */

#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>

#define MULTIPLIER              100     /* Used to initialize myData */
#define NUM_OF_ELEMENT          5       /* Number of elements in myData array */
#define DEPTH_OF_FIFO_QUEUE     2       /* FIFO queue depth--see prj.mdef */

#define SPECIAL_DATA            999     /* Special number to put in queue */

static int myData[NUM_OF_ELEMENT];
static int tcRC = TC_PASS;              /* test case return code */

#ifdef TEST_PRIV_FIFO
	DEFINE_FIFO(FIFOQ, 2, 4);
#endif

/**
 *
 * @brief Initialize data array
 *
 * This routine initializes the myData array used in the FIFO tests.
 *
 * @return N/A
 */

void initMyData(void)
{
	for (int i = 0; i < NUM_OF_ELEMENT; i++) {
		myData[i] = i * MULTIPLIER + 1;
	}  /* for */
}  /* initMyData */

/**
 *
 * @brief Print data array
 *
 * This routine prints myData array.
 *
 * @return N/A
 */

void printMyData(void)
{
	for (int i = 0; i < NUM_OF_ELEMENT; i++) {
		PRINT_DATA("myData[%d] = %d,\n", i, myData[i]);
	}  /* for */

} /* printMyData */

/**
 *
 * @brief Verify return value
 *
 * This routine verifies current value against expected value
 * and returns true if they are the same.
 *
 * @param expectRetValue     expect value
 * @param currentRetValue    current value
 *
 * @return  true, false
 */

bool verifyRetValue(int expectRetValue, int currentRetValue)
{
	return (expectRetValue == currentRetValue);
} /* verifyRetValue */

/**
 *
 * @brief Initialize microkernel objects
 *
 * This routine initializes the microkernel objects used in the FIFO tests.
 *
 * @return N/A
 */

void initMicroObjects(void)
{
	initMyData();
	printMyData();
} /* initMicroObjects */

/**
 *
 * @brief Fills up the FIFO queue
 *
 * This routine fills the FIFO queue with myData array.  This assumes the
 * queue is empty before we put in elements.
 *
 * @param queue          FIFO queue
 * @param numElements    Number of elements used to inserted into the queue
 *
 * @return TC_PASS, TC_FAIL
 *
 * Also updates tcRC when result is TC_FAIL.
 */

int fillFIFO(kfifo_t queue, int numElements)
{
	int result = TC_PASS;       /* TC_PASS or TC_FAIL for this function */
	int retValue;               /* return value from task_fifo_xxx APIs */

	for (int i = 0; i < numElements; i++) {
		retValue = task_fifo_put(queue, &myData[i], TICKS_NONE);
		switch (retValue) {
		case RC_OK:
			/* TC_PRINT("i=%d, successfully put in data=%d\n", i, myData[i]);  */
			if (i >= DEPTH_OF_FIFO_QUEUE) {
				TC_ERROR("Incorrect return value of RC_OK when i = %d\n", i);
				result = TC_FAIL;
				goto exitTest3;
			}
			break;
		case RC_FAIL:
			/* TC_PRINT("i=%d, FIFOQ is full. Cannot put data=%d\n", i, myData[i]); */
			if (i < DEPTH_OF_FIFO_QUEUE) {
				TC_ERROR("Incorrect return value of RC_FAIL when i = %d\n", i);
				result = TC_FAIL;
				goto exitTest3;
			}
			break;
		default:
			TC_ERROR("Incorrect return value of %d when i = %d\n", retValue, i);
			result = TC_FAIL;
			goto exitTest3;
		} /* switch */

	} /* for */

exitTest3:
	if (result == TC_FAIL) {
		tcRC = TC_FAIL;
	}

	TC_END_RESULT(result);
	return result;

} /* fillFIFO */

/**
 *
 * @brief Task to test FIFO queue
 *
 * This routine is run in three context switches:
 * - it puts an element to the FIFO queue
 * - it purges the FIFO queue
 * - it dequeues an element from the FIFO queue
 *
 * @return  N/A
 */
void MicroTestFifoTask(void)
{
	int retValue;                 /* return value of task_fifo_xxx interface */
	int locData = SPECIAL_DATA;   /* variable to pass data to and from queue */

	/* (1) Wait for semaphore: put element test */
	task_sem_take(SEMSIG_MicroTestFifoTask, TICKS_UNLIMITED);

	TC_PRINT("Starts %s\n", __func__);
	/* Put one element */
	TC_PRINT("%s: Puts element %d\n", __func__, locData);
	retValue = task_fifo_put(FIFOQ, &locData, TICKS_NONE);
	/*
	 * Execution is switched back to RegressionTask (a higher priority task)
	 * which is not block anymore.
	 */
	if (verifyRetValue(RC_OK, retValue)) {
		TC_PRINT("%s: FIFOPut OK for %d\n", __func__, locData);
	} else {
		TC_ERROR("FIFOPut failed, retValue %d\n", retValue);
		tcRC = TC_FAIL;
		goto exitTest4;
	}

	/*
	 * (2) Wait for semaphore: purge queue test.  Purge queue while another
	 * task is in task_fifo_put(TICKS_UNLIMITED).  This is to test return
	 * value of the task_fifo_put(TICKS_UNLIMITED) interface.
	 */
	task_sem_take(SEMSIG_MicroTestFifoTask, TICKS_UNLIMITED);
	/*
	 * RegressionTask is waiting to put data into FIFO queue, which is
	 * full.  We purge the queue here and the task_fifo_put(TICKS_UNLIMITED)
	 * interface will terminate the wait and return RC_FAIL.
	 */
	TC_PRINT("%s: About to purge queue\n", __func__);
	retValue = task_fifo_purge(FIFOQ);

	/*
	 * Execution is switched back to RegressionTask (a higher priority task)
	 * which is not block anymore.
	 */
	if (verifyRetValue(RC_OK, retValue)) {
		TC_PRINT("%s: Successfully purged queue\n", __func__);
	} else {
		TC_ERROR("Problem purging queue, %d\n", retValue);
		tcRC = TC_FAIL;
		goto exitTest4;
	}

	/* (3) Wait for semaphore: get element test */
	task_sem_take(SEMSIG_MicroTestFifoTask, TICKS_UNLIMITED);
	TC_PRINT("%s: About to dequeue 1 element\n", __func__);
	retValue =  task_fifo_get(FIFOQ, &locData, TICKS_NONE);
	/*
	 * Execution is switched back to RegressionTask (a higher priority task)
	 * which is not block anymore
	 */
	if ((retValue != RC_OK) || (locData != myData[0])) {
		TC_ERROR("task_fifo_get failed,\n  retValue %d OR got data %d while expect %d\n"
			, retValue, locData, myData[0]);
		tcRC = TC_FAIL;
		goto exitTest4;
	} else {
		TC_PRINT("%s: task_fifo_get got back correct data %d\n", __func__, locData);
	}

exitTest4:
	TC_END_RESULT(tcRC);

	/* Allow RegressionTask to print final result of the test */
	task_sem_give(SEM_TestDone);
}

/**
 *
 * @brief Verifies data in queue is correct
 *
 * This routine assumes that the queue is full when this function is called.
 * It counts the number of elements in the queue, dequeues elements and verifies
 * that they are in the right order. Expect the dequeue order as: myData[0],
 * myData[1].
 *
 * @param loopCnt    number of elements passed to the for loop
 *
 * @return  TC_PASS, TC_FAIL
 *
 * Also updates tcRC when result is TC_FAIL.
 */
int verifyQueueData(int loopCnt)
{
	int result = TC_PASS;       /* TC_PASS or TC_FAIL for this function */
	int retValue;               /* task_fifo_xxx interface return value */
	int locData;                /* local variable used for passing data */

	/*
	 * Counts elements using task_fifo_size_get interface.  Dequeues elements from
	 * FIFOQ.  Test for proper return code when FIFO queue is empty using
	 * task_fifo_get interface.
	 */
	for (int i = 0; i < loopCnt; i++) {
		/* Counts number of elements */
		retValue = task_fifo_size_get(FIFOQ);
		if (!verifyRetValue(DEPTH_OF_FIFO_QUEUE-i, retValue)) {
			TC_ERROR("i=%d, incorrect number of FIFO elements in queue: %d, expect %d\n"
				, i, retValue, DEPTH_OF_FIFO_QUEUE-i);
			result = TC_FAIL;
			goto exitTest2;
		} else {
			/* TC_PRINT("%s: i=%d, %d elements in queue\n", __func__, i, retValue); */
		}

		/* Dequeues element */
		retValue = task_fifo_get(FIFOQ, &locData, TICKS_NONE);

		switch (retValue) {
		case RC_OK:
			if ((i >= DEPTH_OF_FIFO_QUEUE) || (locData != myData[i])) {
				TC_ERROR("RC_OK but got wrong data %d for i=%d\n", locData, i);
				result = TC_FAIL;
				goto exitTest2;
			}

			TC_PRINT("%s: i=%d, successfully get data %d\n", __func__, i, locData);
			break;
		case RC_FAIL:
			if (i < DEPTH_OF_FIFO_QUEUE) {
				TC_ERROR("RC_FAIL but i is only %d\n", i);
				result = TC_FAIL;
				goto exitTest2;
			}
			TC_PRINT("%s: i=%d, FIFOQ is empty. No data.\n", __func__, i);
			break;
		default:
			TC_ERROR("i=%d, incorrect return value %d\n", i, retValue);
			result = TC_FAIL;
			goto exitTest2;
		}  /* switch */
	} /* for */

exitTest2:

	if (result == TC_FAIL) {
		tcRC = TC_FAIL;
	}

	TC_END_RESULT(result);
	return result;

} /* verifyQueueData */


/**
 *
 * @brief Main task to test FIFO queue
 *
 * This routine initializes data, fills the FIFO queue and verifies the
 * data in the queue is in correct order when items are being dequeued.
 * It also tests the wait (with and without timeouts) to put data into
 * queue when the queue is full.  The queue is purged at some point
 * and checked to see if the number of elements is correct.
 * The get wait interfaces (with and without timeouts) are also tested
 * and data verified.
 *
 * @return  N/A
 */

void RegressionTask(void)
{
	int retValue;               /* task_fifo_xxx interface return value */
	int locData;                /* local variable used for passing data */
	int result;                 /* result from utility functions */

	TC_START("Test Microkernel FIFO");

	initMicroObjects();

	/*
	 * Checks number of elements in queue, expect 0. Test task_fifo_size_get
	 * interface.
	 */
	retValue = task_fifo_size_get(FIFOQ);
	if (!verifyRetValue(0, retValue)) {
		TC_ERROR("Incorrect number of FIFO elements in queue: %d\n", retValue);
		tcRC = TC_FAIL;
		goto exitTest;
	}

	/*
	 * FIFOQ is only two elements deep.  Test for proper return code when
	 * FIFO queue is full.  Test task_fifo_put(TICKS_NONE) interface.
	 */
	result = fillFIFO(FIFOQ, NUM_OF_ELEMENT);
	if (result == TC_FAIL) { /* terminate test */
		TC_ERROR("Failed fillFIFO.\n");
		goto exitTest;
	}

	/*
	 * Checks number of elements in FIFO queue, should be full.  Also verifies
	 * data is in correct order.  Test task_fifo_size_get  and task_fifo_get interface.
	 */
	result = verifyQueueData(DEPTH_OF_FIFO_QUEUE + 1);
	if (result == TC_FAIL) { /* terminate test */
		TC_ERROR("Failed verifyQueueData.\n");
		goto exitTest;
	}

/*----------------------------------------------------------------------------*/

	/* Fill FIFO queue */
	result = fillFIFO(FIFOQ, NUM_OF_ELEMENT);
	if (result == TC_FAIL) { /* terminate test */
		TC_ERROR("Failed fillFIFO.\n");
		goto exitTest;
	}

	/*
	 * Put myData[4] into queue with wait, test task_fifo_put(timeout)
	 * interface. Queue is full, so this data did not make it into queue.
	 * Expect return code of RC_TIME.
	 */
	TC_PRINT("%s: About to putWT with data %d\n", __func__, myData[4]);
	retValue = task_fifo_put(FIFOQ, &myData[4], 2);  /* wait for 2 ticks */
	if (verifyRetValue(RC_TIME, retValue)) {
		TC_PRINT("%s: FIFO Put time out as expected for data %d\n"
			, __func__, myData[4]);
	} else {
		TC_ERROR("Failed task_fifo_put for data %d, retValue %d\n",
				 myData[4], retValue);
		tcRC = TC_FAIL;
		goto exitTest;
	}

	/* Queue is full at this stage.  Verify data is correct. */
	result = verifyQueueData(DEPTH_OF_FIFO_QUEUE);
	if (result == TC_FAIL) { /* terminate test */
		TC_ERROR("Failed verifyQueueData.\n");
		goto exitTest;
	}

/*----------------------------------------------------------------------------*/

	/* Fill FIFO queue.  Check number of elements in queue, should be 2. */
	result = fillFIFO(FIFOQ, NUM_OF_ELEMENT);
	if (result == TC_FAIL) { /* terminate test */
		TC_ERROR("Failed fillFIFO.\n");
		goto exitTest;
	}

	retValue = task_fifo_size_get(FIFOQ);
	if (verifyRetValue(DEPTH_OF_FIFO_QUEUE, retValue)) {
		TC_PRINT("%s: %d element in queue\n", __func__, retValue);
	} else {
		TC_ERROR("Incorrect number of FIFO elements in queue: %d\n", retValue);
		tcRC = TC_FAIL;
		goto exitTest;
	}

	/*
	 * Purge queue, check number of elements in queue.  Test task_fifo_purge
	 * interface.
	 */
	retValue = task_fifo_purge(FIFOQ);
	if (verifyRetValue(RC_OK, retValue)) {
		TC_PRINT("%s: Successfully purged queue\n", __func__);
	} else {
		TC_ERROR("Problem purging queue, %d\n", retValue);
		tcRC = TC_FAIL;
		goto exitTest;
	}

	/* Count number of elements in queue */
	retValue = task_fifo_size_get(FIFOQ);
	if (verifyRetValue(0, retValue)) {
		TC_PRINT("%s: confirm %d element in queue\n", __func__, retValue);
	} else {
		TC_ERROR("Incorrect number of FIFO elements in queue: %d\n", retValue);
		tcRC = TC_FAIL;
		goto exitTest;
	}

	PRINT_LINE;
/*----------------------------------------------------------------------------*/

	/*
	 * Semaphore to allow MicroTestFifoTask to run, but MicroTestFifoTask is lower
	 * priority, so it won't run until this current task is blocked
	 * in task_fifo_get interface later.
	 */
	task_sem_give(SEMSIG_MicroTestFifoTask);

	/*
	 * Test task_fifo_get interface.
	 * Expect MicroTestFifoTask to run and insert SPECIAL_DATA into queue.
	 */
	TC_PRINT("%s: About to GetW data\n", __func__);
	retValue = task_fifo_get(FIFOQ, &locData, TICKS_UNLIMITED);
	if ((retValue != RC_OK) || (locData != SPECIAL_DATA)) {
		TC_ERROR("Failed task_fifo_get interface for data %d, retValue %d\n"
			, locData, retValue);
		tcRC = TC_FAIL;
		goto exitTest;
	} else {
		TC_PRINT("%s: GetW get back %d\n", __func__, locData);
	}

	/* MicroTestFifoTask may have modified tcRC */
	if (tcRC == TC_FAIL) {    /* terminate test */
		TC_ERROR("tcRC failed.");
		goto exitTest;
	}

	/*
	 * Test task_fifo_get(timeout) interface.  Try to get more data, but
	 * there is none before it times out.
	 */
	retValue = task_fifo_get(FIFOQ, &locData, 2);
	if (verifyRetValue(RC_TIME, retValue)) {
		TC_PRINT("%s: GetWT timeout expected\n", __func__);
	} else {
		TC_ERROR("Failed task_fifo_get interface for retValue %d\n", retValue);
		tcRC = TC_FAIL;
		goto exitTest;
	}

/*----------------------------------------------------------------------------*/

	/* Fill FIFO queue */
	result = fillFIFO(FIFOQ, NUM_OF_ELEMENT);
	if (result == TC_FAIL) { /* terminate test */
		TC_ERROR("Failed fillFIFO.\n");
		goto exitTest;
	}

	/* Semaphore to allow MicroTestFifoTask to run */
	task_sem_give(SEMSIG_MicroTestFifoTask);

	/* MicroTestFifoTask may have modified tcRC */
	if (tcRC == TC_FAIL) {    /* terminate test */
		TC_ERROR("tcRC failed.");
		goto exitTest;
	}

	/* Queue is full */
	locData = SPECIAL_DATA;
	TC_PRINT("%s: about to putW data %d\n", __func__, locData);
	retValue = task_fifo_put(FIFOQ,  &locData, TICKS_UNLIMITED);

	/*
	 * Execution is switched to MicroTestFifoTask, which will purge the queue.
	 * When the queue is purged while other tasks are waiting to put data into
	 * queue, the return value will be RC_FAIL.
	 */
	if (verifyRetValue(RC_FAIL, retValue)) {
		TC_PRINT("%s: PutW ok when queue is purged while waiting\n", __func__);
	} else {
		TC_ERROR("Failed task_fifo_put interface when queue is purged, retValue %d\n"
			, retValue);
		tcRC = TC_FAIL;
		goto exitTest;
	}

/*----------------------------------------------------------------------------*/

	/* Fill FIFO queue */
	result = fillFIFO(FIFOQ, NUM_OF_ELEMENT);
	if (result == TC_FAIL) { /* terminate test */
		TC_ERROR("Failed fillFIFO.\n");
		goto exitTest;
	}

	/* Semaphore to allow MicroTestFifoTask to run */
	task_sem_give(SEMSIG_MicroTestFifoTask);

	/* MicroTestFifoTask may have modified tcRC */
	if (tcRC == TC_FAIL) {    /* terminate test */
		TC_ERROR("tcRC failed.");
		goto exitTest;
	}

	/* Queue is full */
	TC_PRINT("%s: about to putW data %d\n", __func__, myData[4]);
	retValue = task_fifo_put(FIFOQ,  &myData[4], TICKS_UNLIMITED);
	/* Execution is switched to MicroTestFifoTask, which will dequeue one element */
	if (verifyRetValue(RC_OK, retValue)) {
		TC_PRINT("%s: PutW success for data %d\n", __func__, myData[4]);
	} else {
		TC_ERROR("Failed task_fifo_put interface for data %d, retValue %d\n"
			, myData[4], retValue);
		tcRC = TC_FAIL;
		goto exitTest;
	}

	PRINT_LINE;
/*----------------------------------------------------------------------------*/

	/*
	 * Dequeue all data to check.  Expect data in the queue to be:
	 * myData[1], myData[4].  myData[0] was dequeued by MicroTestFifoTask.
	 */

	/* Get first data */
	retValue = task_fifo_get(FIFOQ, &locData, TICKS_NONE);
	if ((retValue != RC_OK) || (locData != myData[1])) {
		TC_ERROR("Get back data %d, retValue %d\n", locData, retValue);
		tcRC = TC_FAIL;
		goto exitTest;
	} else {
		TC_PRINT("%s: Get back data %d\n", __func__, locData);
	}

	/* Get second data */
	retValue = task_fifo_get(FIFOQ, &locData, TICKS_NONE);
	if ((retValue != RC_OK) || (locData != myData[4])) {
		TC_ERROR("Get back data %d, retValue %d\n", locData, retValue);
		tcRC = TC_FAIL;
		goto exitTest;
	} else {
		TC_PRINT("%s: Get back data %d\n", __func__, locData);
	}

	/* Queue should be empty */
	retValue = task_fifo_get(FIFOQ, &locData, TICKS_NONE);
	if (retValue != RC_FAIL) {
		TC_ERROR("%s: incorrect retValue %d\n", __func__, retValue);
		tcRC = TC_FAIL;
		goto exitTest;
	} else {
		TC_PRINT("%s: queue is empty.  Test Done!\n", __func__);
	}

	task_sem_take(SEM_TestDone, TICKS_UNLIMITED);

exitTest:

	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}  /* RegressionTask */
