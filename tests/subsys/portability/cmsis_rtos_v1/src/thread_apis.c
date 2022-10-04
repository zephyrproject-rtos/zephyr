/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/kernel.h>
#include <cmsis_os.h>

#define STACKSZ CONFIG_CMSIS_THREAD_MAX_STACK_SIZE

static osPriority osPriorityDeadline = 10;

/* This is used to check the thread yield functionality between 2 threads */
static int thread_yield_check;

/* This is used to indicate the completion of processing for thread3 */
static int thread3_state;

void thread1(void const *argument)
{
	osStatus status;
	osThreadId id = osThreadGetId();

	zassert_true(id != NULL, "Failed getting Thread ID");

	/* This thread starts off at a high priority (same as thread2) */
	thread_yield_check++;
	zassert_equal(thread_yield_check, 1, NULL);

	/* Yield to thread2 which is of same priority */
	status = osThreadYield();
	zassert_true(status == osOK, "Error doing thread yield");

	/* thread_yield_check should now be 2 as it was incremented
	 * in thread2.
	 */
	zassert_equal(thread_yield_check, 2, NULL);
}

void thread2(void const *argument)
{
	/* By now thread1 would have set thread_yield_check to 1 and would
	 * have yielded the CPU. Incrementing it over here would essentially
	 * confirm that the yield was indeed executed.
	 */
	thread_yield_check++;

	/* Yield back to thread1 which is of same priority */
	osThreadYield();
}

void thread3(void const *argument)
{
	osStatus status;
	osPriority rv;
	osThreadId id = osThreadGetId();
	osPriority prio = osThreadGetPriority(id);

	/* Lower the priority of the current thread */
	osThreadSetPriority(id, osPriorityBelowNormal);
	rv = osThreadGetPriority(id);
	zassert_equal(rv, osPriorityBelowNormal,
			"Expected priority to be changed to %d, not %d",
			(int)osPriorityBelowNormal, (int)rv);

	/* Increase the priority of the current thread */
	osThreadSetPriority(id, osPriorityAboveNormal);
	rv = osThreadGetPriority(id);
	zassert_equal(rv, osPriorityAboveNormal,
			"Expected priority to be changed to %d, not %d",
			(int)osPriorityAboveNormal, (int)rv);

	/* Restore the priority of the current thread */
	osThreadSetPriority(id, prio);
	rv = osThreadGetPriority(id);
	zassert_equal(rv, prio,
			"Expected priority to be changed to %d, not %d",
			(int)prio, (int)rv);

	/* Try to set unsupported priority and assert failure */
	status = osThreadSetPriority(id, osPriorityDeadline);
	zassert_true(status == osErrorValue,
			"Something's wrong with osThreadSetPriority!");

	/* Indication that thread3 is done with its processing */
	thread3_state = 1;

	/* Keep looping till it gets killed */
	do {
		osDelay(100);
	} while (1);
}

osThreadDef(thread1, osPriorityHigh, 1, STACKSZ);
osThreadDef(thread2, osPriorityHigh, 1, STACKSZ);
osThreadDef(thread3, osPriorityNormal, 1, STACKSZ);

void test_thread_prio(void)
{
	osStatus status;
	osThreadId id3;

	id3 = osThreadCreate(osThread(thread3), NULL);
	zassert_true(id3 != NULL, "Failed creating thread3");

	/* Keep delaying 10 milliseconds to ensure thread3 is done with
	 * its execution. It loops at the end and is terminated here.
	 */
	do {
		osDelay(10);
	} while (thread3_state == 0);

	status = osThreadTerminate(id3);
	zassert_true(status == osOK, "Error terminating thread3");

	/* Try to set priority to inactive thread and assert failure */
	status = osThreadSetPriority(id3, osPriorityNormal);
	zassert_true(status == osErrorResource,
			"Something's wrong with osThreadSetPriority!");

	/* Try to terminate inactive thread and assert failure */
	status = osThreadTerminate(id3);
	zassert_true(status == osErrorResource,
			"Something's wrong with osThreadTerminate!");

	thread3_state = 0;
}

void test_thread_apis(void)
{
	osThreadId id1;
	osThreadId id2;

	id1 = osThreadCreate(osThread(thread1), NULL);
	zassert_true(id1 != NULL, "Failed creating thread1");

	id2 = osThreadCreate(osThread(thread2), NULL);
	zassert_true(id2 != NULL, "Failed creating thread2");

	do {
		osDelay(100);
	} while (thread_yield_check != 2);
}
