/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_os2.h>

#define STACKSZ		512

/* This is used to check the thread yield functionality between 2 threads */
static int thread_yield_check;

static K_THREAD_STACK_DEFINE(test_stack1, STACKSZ);
static osThreadAttr_t thread1_attr = {
	.name       = "Thread1",
	.stack_mem  = &test_stack1,
	.stack_size = STACKSZ,
	.priority   = osPriorityHigh,
};

static K_THREAD_STACK_DEFINE(test_stack2, STACKSZ);
static osThreadAttr_t thread2_attr = {
	.name       = "Thread2",
	.stack_mem  = &test_stack2,
	.stack_size = STACKSZ,
	.priority   = osPriorityHigh,
};

static void thread1(void *argument)
{
	osStatus_t status;
	osThreadId_t thread_id;
	const char *name;

	thread_id = osThreadGetId();
	zassert_true(thread_id != NULL, "Failed getting Thread ID");

	name = osThreadGetName(thread_id);
	zassert_true(strcmp(thread1_attr.name, name) == 0,
			"Failed getting Thread name");

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

	osThreadExit();
}

static void thread2(void *argument)
{
	u32_t i, num_threads, max_num_threads = 5;
	osThreadId_t *thread_array;

	/* By now thread1 would have set thread_yield_check to 1 and would
	 * have yielded the CPU. Incrementing it over here would essentially
	 * confirm that the yield was indeed executed.
	 */
	thread_yield_check++;

	thread_array = k_calloc(max_num_threads, sizeof(osThreadId_t));
	num_threads = osThreadEnumerate(thread_array, max_num_threads);
	zassert_equal(num_threads, 2,
			"Incorrect number of cmsis rtos v2 threads");

	for (i = 0; i < num_threads; i++) {
		zassert_true(
			osThreadGetStackSize(thread_array[i]) <= STACKSZ,
			"stack size allocated is not what is expected");

		zassert_true(
			osThreadGetStackSpace(thread_array[i]) <= STACKSZ - 4,
			"stack size remaining is not what is expected");
	}

	zassert_equal(osThreadGetState(thread_array[1]), osThreadReady,
			"Thread not in ready state");
	zassert_equal(osThreadGetState(thread_array[0]), osThreadRunning,
			"Thread not in running state");

	zassert_equal(osThreadSuspend(thread_array[1]), osOK, "");
	zassert_equal(osThreadGetState(thread_array[1]), osThreadBlocked,
			"Thread not in blocked state");

	zassert_equal(osThreadResume(thread_array[1]), osOK, "");
	zassert_equal(osThreadGetState(thread_array[1]), osThreadReady,
			"Thread not in ready state");

	k_free(thread_array);

	/* Yield back to thread1 which is of same priority */
	osThreadYield();
}

void test_thread_apis(void)
{
	osThreadId_t id1;
	osThreadId_t id2;

	id1 = osThreadNew(thread1, NULL, &thread1_attr);
	zassert_true(id1 != NULL, "Failed creating thread1");

	id2 = osThreadNew(thread2, NULL, &thread2_attr);
	zassert_true(id2 != NULL, "Failed creating thread2");

	zassert_equal(osThreadGetCount(), 2,
			"Incorrect number of cmsis rtos v2 threads");

	do {
		osDelay(100);
	} while (thread_yield_check != 2);
}

static osPriority_t OsPriorityInvalid = 60;

/* This is used to indicate the completion of processing for thread3 */
static int thread3_state;

static K_THREAD_STACK_DEFINE(test_stack3, STACKSZ);
static osThreadAttr_t thread3_attr = {
	.name       = "Thread3",
	.stack_mem  = &test_stack3,
	.stack_size = STACKSZ,
	.priority   = osPriorityNormal,
};

static void thread3(void *argument)
{
	osStatus_t status;
	osPriority_t rv;
	osThreadId_t id = osThreadGetId();
	osPriority_t prio = osThreadGetPriority(id);

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
	status = osThreadSetPriority(id, OsPriorityInvalid);
	zassert_true(status == osErrorParameter,
			"Something's wrong with osThreadSetPriority!");

	/* Indication that thread3 is done with its processing */
	thread3_state = 1;

	/* Keep looping till it gets killed */
	do {
		osDelay(100);
	} while (1);
}

void test_thread_prio(void)
{
	osStatus_t status;
	osThreadId_t id3;

	id3 = osThreadNew(thread3, NULL, &thread3_attr);
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
