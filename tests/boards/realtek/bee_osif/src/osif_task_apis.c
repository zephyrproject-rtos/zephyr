/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * Implementation based on: tests/subsys/portability/cmsis_rtos_v2/src/thread_apis.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <os_task.h>
#include <os_sched.h>

#define STACKSZ 512

/* This is used to check the thread yield functionality between 2 threads */
static int thread_yield_check;

/* This is used to indicate the completion of processing for thread3 */
static int thread3_state;

void *id4;

void thread1(void *argument)
{
	bool status;
	void *id;

	status = os_task_handle_get(&id);

	zassert_true(id != NULL, "os_task_handle_get failed");

	/* This thread starts off at a high priority (same as thread2) */
	thread_yield_check++;
	zassert_equal(thread_yield_check, 1);

	/* Yield to thread2 which is of same priority */
	status = os_task_yield();
	zassert_true(status == true, "Error doing thread yield");

	/* thread_yield_check should now be 2 as it was incremented
	 * in thread2.
	 */
	zassert_equal(thread_yield_check, 2);
}

void thread2(void *argument)
{
	/* By now thread1 would have set thread_yield_check to 1 and would
	 * have yielded the CPU. Incrementing it over here would essentially
	 * confirm that the yield was indeed executed.
	 */
	thread_yield_check++;

	/* Yield back to thread1 which is of same priority */
	os_task_yield();
}

void thread3(void *argument)
{
	bool status;
	uint16_t rv;
	void *id;
	uint16_t prio;

	status = os_task_handle_get(&id);
	status = os_task_priority_get(id, &prio);

	/* Lower the priority of the current thread */
	os_task_priority_set(id, 2);
	status = os_task_priority_get(id, &rv);
	zassert_equal(rv, 2, "Expected priority to be changed to %d, not %d", 2, rv);

	/* Increase the priority of the current thread */
	os_task_priority_set(id, 4);
	status = os_task_priority_get(id, &rv);
	zassert_equal(rv, 4, "Expected priority to be changed to %d, not %d", 4, rv);

	/* Restore the priority of the current thread */
	os_task_priority_set(id, prio);
	status = os_task_priority_get(id, &rv);
	zassert_equal(rv, prio, "Expected priority to be changed to %d, not %d", prio, rv);

	/* Try to set unsupported priority and assert failure */
	status = os_task_priority_set(id, 7);
	zassert_true(status == false, "Something's wrong with os_task_priority_set!");

	/* Indication that thread3 is done with its processing */
	thread3_state = 1;

	/* Keep looping till it gets killed */
	do {
		os_delay(100);
	} while (1);
}

void thread_delete_self(void *argument)
{
	bool status;

	id4 = NULL;
	status = os_task_delete(NULL);

	zassert_true(false, "Deleting self (NULL) would not return here!");
}

ZTEST(osif_task_apis, test_thread_prio)
{
	void *id3 = NULL;
	uint32_t task_param;
	bool status;

	status = os_task_create(&id3, "task3", thread3, &task_param, STACKSZ, 6);
	zassert_true(status != false, "Failed creating thread3");

	/* Keep delaying 10 milliseconds to ensure thread3 is done with
	 * its execution. It loops at the end and is terminated here.
	 */
	do {
		os_delay(10);
	} while (thread3_state == 0);

	status = os_task_delete(id3);
	zassert_true(status == true, "Error deleting thread3");

	/* Try to set priority to inactive thread and assert failure */
	status = os_task_priority_set(id3, 3);
	zassert_true(status == true, "Something's wrong with os_task_priority_set!");

	thread3_state = 0;
}

ZTEST(osif_task_apis, test_thread_apis)
{
	void *id1 = NULL;
	void *id2 = NULL;
	uint32_t task_param;
	bool status;

	status = os_task_create(&id1, "task1", thread1, &task_param, STACKSZ, 6);
	zassert_true(status != false, "Failed creating thread1");

	status = os_task_create(&id2, "task2", thread2, &task_param, STACKSZ, 6);
	zassert_true(status != false, "Failed creating thread2");

	do {
		os_delay(100);
	} while (thread_yield_check != 2);
}

ZTEST(osif_task_apis, test_delete_self_with_null)
{
	uint32_t task_param;
	bool status;

	status = os_task_create(&id4, "delete_self", thread_delete_self, &task_param, STACKSZ, 5);
	zassert_true(status == true, "Failed to create self-delete thread");

	os_delay(100);

	zassert_true(id4 == NULL, "Failed to delete thread!");
}

ZTEST_SUITE(osif_task_apis, NULL, NULL, NULL, NULL, NULL);
