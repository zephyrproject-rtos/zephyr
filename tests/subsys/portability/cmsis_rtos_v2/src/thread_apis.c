/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/portability/cmsis_os2.h>
#include <zephyr/portability/cmsis_types.h>

#define STACKSZ CONFIG_CMSIS_V2_THREAD_MAX_STACK_SIZE

/* This is used to check the thread yield functionality between 2 threads */
static int thread_yield_check;
static int thread_yield_check_dynamic;

static K_THREAD_STACK_DEFINE(test_stack1, STACKSZ);
static osThreadAttr_t os_thread1_attr = {
	.name = "Thread1",
	.stack_mem = &test_stack1,
	.stack_size = STACKSZ,
	.priority = osPriorityHigh,
};

static K_THREAD_STACK_DEFINE(test_stack2, STACKSZ);
static osThreadAttr_t os_thread2_attr = {
	.name = "Thread2",
	.stack_mem = &test_stack2,
	.stack_size = STACKSZ,
	.priority = osPriorityHigh,
};

struct thread1_args {
	int *yield_check;
	const char *name;
};

static void thread1(void *argument)
{
	osStatus_t status;
	osThreadId_t thread_id;
	const char *name;
	struct thread1_args *args = (struct thread1_args *)argument;

	thread_id = osThreadGetId();
	zassert_true(thread_id != NULL, "Failed getting Thread ID");

	name = osThreadGetName(thread_id);
	zassert_str_equal(args->name, name, "Failed getting Thread name");

	/* This thread starts off at a high priority (same as thread2) */
	(*args->yield_check)++;
	zassert_equal(*args->yield_check, 1);

	/* Yield to thread2 which is of same priority */
	status = osThreadYield();
	zassert_true(status == osOK, "Error doing thread yield");

	/* thread_yield_check should now be 2 as it was incremented
	 * in thread2.
	 */
	zassert_equal(*args->yield_check, 2);

	osThreadExit();
}

static void thread2(void *argument)
{
	uint32_t i, num_threads, max_num_threads = 5U;
	osThreadId_t *thread_array;
	int *yield_check = (int *)argument;

	/* By now thread1 would have set thread_yield_check to 1 and would
	 * have yielded the CPU. Incrementing it over here would essentially
	 * confirm that the yield was indeed executed.
	 */
	(*yield_check)++;

	thread_array = k_calloc(max_num_threads, sizeof(osThreadId_t));
	num_threads = osThreadEnumerate(thread_array, max_num_threads);
	zassert_equal(num_threads, 2, "Incorrect number of cmsis rtos v2 threads");

	for (i = 0U; i < num_threads; i++) {
		uint32_t size = osThreadGetStackSize(thread_array[i]);
		uint32_t space = osThreadGetStackSpace(thread_array[i]);

		zassert_true(space < size, "stack size remaining is not what is expected");
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

static void thread_apis_common(int *yield_check, const char *thread1_name,
			       osThreadAttr_t *thread1_attr, osThreadAttr_t *thread2_attr)
{
	osThreadId_t id1;
	osThreadId_t id2;

	struct thread1_args args = {.yield_check = yield_check, .name = thread1_name};

	id1 = osThreadNew(thread1, &args, thread1_attr);
	zassert_true(id1 != NULL, "Failed creating thread1");

	id2 = osThreadNew(thread2, yield_check, thread2_attr);
	zassert_true(id2 != NULL, "Failed creating thread2");

	zassert_equal(osThreadGetCount(), 2, "Incorrect number of cmsis rtos v2 threads");

	do {
		osDelay(100);
	} while (*yield_check != 2);
}

ZTEST(cmsis_thread_apis, test_thread_apis_dynamic)
{
	thread_apis_common(&thread_yield_check_dynamic, "ZephyrThread", NULL, NULL);
}

ZTEST(cmsis_thread_apis, test_thread_apis)
{
	thread_apis_common(&thread_yield_check, os_thread1_attr.name, &os_thread1_attr,
			   &os_thread2_attr);
}

static osPriority_t OsPriorityInvalid = 60;

/* This is used to indicate the completion of processing for thread3 */
static int thread3_state;
static int thread3_state_dynamic;

static K_THREAD_STACK_DEFINE(test_stack3, STACKSZ);
static osThreadAttr_t thread3_attr = {
	.name = "Thread3",
	.stack_mem = &test_stack3,
	.stack_size = STACKSZ,
	.priority = osPriorityNormal,
};

static void thread3(void *argument)
{
	osStatus_t status;
	osPriority_t rv;
	osThreadId_t id = osThreadGetId();
	osPriority_t prio = osThreadGetPriority(id);
	int *state = (int *)argument;

	/* Lower the priority of the current thread */
	osThreadSetPriority(id, osPriorityBelowNormal);
	rv = osThreadGetPriority(id);
	zassert_equal(rv, osPriorityBelowNormal, "Expected priority to be changed to %d, not %d",
		      (int)osPriorityBelowNormal, (int)rv);

	/* Increase the priority of the current thread */
	osThreadSetPriority(id, osPriorityAboveNormal);
	rv = osThreadGetPriority(id);
	zassert_equal(rv, osPriorityAboveNormal, "Expected priority to be changed to %d, not %d",
		      (int)osPriorityAboveNormal, (int)rv);

	/* Restore the priority of the current thread */
	osThreadSetPriority(id, prio);
	rv = osThreadGetPriority(id);
	zassert_equal(rv, prio, "Expected priority to be changed to %d, not %d", (int)prio,
		      (int)rv);

	/* Try to set unsupported priority and assert failure */
	status = osThreadSetPriority(id, OsPriorityInvalid);
	zassert_true(status == osErrorParameter, "Something's wrong with osThreadSetPriority!");

	/* Indication that thread3 is done with its processing */
	*state = 1;

	/* Keep looping till it gets killed */
	do {
		osDelay(100);
	} while (1);
}

static void thread_prior_common(int *state, osThreadAttr_t *attr)
{
	osStatus_t status;
	osThreadId_t id3;

	id3 = osThreadNew(thread3, state, attr);
	zassert_true(id3 != NULL, "Failed creating thread3");

	/* Keep delaying 10 milliseconds to ensure thread3 is done with
	 * its execution. It loops at the end and is terminated here.
	 */
	do {
		osDelay(10);
	} while (*state == 0);

	status = osThreadTerminate(id3);
	zassert_true(status == osOK, "Error terminating thread3");

	/* Try to set priority to inactive thread and assert failure */
	status = osThreadSetPriority(id3, osPriorityNormal);
	zassert_true(status == osErrorResource, "Something's wrong with osThreadSetPriority!");

	/* Try to terminate inactive thread and assert failure */
	status = osThreadTerminate(id3);
	zassert_true(status == osErrorResource, "Something's wrong with osThreadTerminate!");

	*state = 0;
}

ZTEST(cmsis_thread_apis, test_thread_prio_dynamic)
{
	thread_prior_common(&thread3_state_dynamic, NULL);
}

ZTEST(cmsis_thread_apis, test_thread_prio)
{
	thread_prior_common(&thread3_state, &thread3_attr);
}

#define DELAY_MS 1000
#define DELTA_MS 500

static void thread5(void *argument)
{
	printk(" * Thread B started.\n");
	osDelay(k_ms_to_ticks_ceil32(DELAY_MS));
	printk(" * Thread B joining...\n");
}

static void thread4(void *argument)
{
	osThreadId_t tB = argument;
	osStatus_t status;

	printk(" + Thread A started.\n");
	status = osThreadJoin(tB);
	zassert_equal(status, osOK, "osThreadJoin thread B failed!");
	printk(" + Thread A joining...\n");
}

ZTEST(cmsis_thread_apis, test_thread_join)
{
	osThreadAttr_t attr = {0};
	int64_t time_stamp;
	int64_t milliseconds_spent;
	osThreadId_t tA, tB;
	osStatus_t status;

	attr.attr_bits = osThreadJoinable;

	time_stamp = k_uptime_get();

	printk(" - Creating thread B...\n");
	tB = osThreadNew(thread5, NULL, &attr);
	zassert_not_null(tB, "Failed to create thread B with osThreadNew!");

	printk(" - Creating thread A...\n");
	attr.priority = osPriorityLow;
	tA = osThreadNew(thread4, tB, &attr);
	zassert_not_null(tA, "Failed to create thread A with osThreadNew!");

	printk(" - Waiting for thread B to join...\n");
	status = osThreadJoin(tB);
	zassert_equal(status, osOK, "osThreadJoin thread B failed!");

	if (status == osOK) {
		printk(" - Thread B joined.\n");
	}

	milliseconds_spent = k_uptime_delta(&time_stamp);
	zassert_true((milliseconds_spent >= DELAY_MS - DELTA_MS) &&
			     (milliseconds_spent <= DELAY_MS + DELTA_MS),
		     "Join completed but was too fast or too slow.");

	printk(" - Waiting for thread A to join...\n");
	status = osThreadJoin(tA);
	zassert_equal(status, osOK, "osThreadJoin thread A failed!");

	if (status == osOK) {
		printk(" - Thread A joined.\n");
	}
}

ZTEST(cmsis_thread_apis, test_thread_detached)
{
	osThreadId_t thread;
	osStatus_t status;

	thread = osThreadNew(thread5, NULL, NULL); /* osThreadDetached */
	zassert_not_null(thread, "Failed to create thread with osThreadNew!");

	osDelay(k_ms_to_ticks_ceil32(DELAY_MS - DELTA_MS));

	status = osThreadJoin(thread);
	zassert_equal(status, osErrorResource, "Incorrect status returned from osThreadJoin!");

	osDelay(k_ms_to_ticks_ceil32(DELTA_MS));
}

void thread6(void *argument)
{
	osThreadId_t thread = argument;
	osStatus_t status;

	status = osThreadJoin(thread);
	zassert_equal(status, osErrorResource, "Incorrect status returned from osThreadJoin!");
}

ZTEST(cmsis_thread_apis, test_thread_joinable_detach)
{
	osThreadAttr_t attr = {0};
	osThreadId_t tA, tB;
	osStatus_t status;

	attr.attr_bits = osThreadJoinable;

	tA = osThreadNew(thread5, NULL, &attr);
	zassert_not_null(tA, "Failed to create thread with osThreadNew!");

	tB = osThreadNew(thread6, tA, &attr);
	zassert_not_null(tB, "Failed to create thread with osThreadNew!");

	osDelay(k_ms_to_ticks_ceil32(DELAY_MS - DELTA_MS));

	status = osThreadDetach(tA);
	zassert_equal(status, osOK, "osThreadDetach failed.");

	osDelay(k_ms_to_ticks_ceil32(DELTA_MS));
}

ZTEST(cmsis_thread_apis, test_thread_joinable_terminate)
{
	osThreadAttr_t attr = {0};
	osThreadId_t tA, tB;
	osStatus_t status;

	attr.attr_bits = osThreadJoinable;

	tA = osThreadNew(thread5, NULL, &attr);
	zassert_not_null(tA, "Failed to create thread with osThreadNew!");

	tB = osThreadNew(thread6, tA, &attr);
	zassert_not_null(tB, "Failed to create thread with osThreadNew!");

	osDelay(k_ms_to_ticks_ceil32(DELAY_MS - DELTA_MS));

	status = osThreadTerminate(tA);
	zassert_equal(status, osOK, "osThreadTerminate failed.");

	osDelay(k_ms_to_ticks_ceil32(DELTA_MS));
}

static K_THREAD_STACK_DEFINE(test_stack7, STACKSZ);
static struct cmsis_rtos_thread_cb test_cb7;
static const osThreadAttr_t os_thread7_attr = {
	.name = "Thread7",
	.cb_mem = &test_cb7,
	.cb_size = sizeof(test_cb7),
	.stack_mem = &test_stack7,
	.stack_size = STACKSZ,
	.priority = osPriorityNormal,
};
static void thread7(void *argument)
{
	printf("Thread 7 ran\n");
}
ZTEST(cmsis_thread_apis, test_thread_apis_static_allocation)
{
	osThreadId_t id;

	id = osThreadNew(thread7, NULL, &os_thread7_attr);
	zassert_not_null(id, "Failed to create thread with osThreadNew using static cb/stack");
}
ZTEST_SUITE(cmsis_thread_apis, NULL, NULL, NULL, NULL, NULL);
