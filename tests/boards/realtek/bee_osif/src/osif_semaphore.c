/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * Implementation based on: tests/subsys/portability/cmsis_rtos_v2/src/semaphore.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <os_task.h>
#include <os_sync.h>
#include <os_sched.h>

#define WAIT_MS    50
#define TIMEOUT_MS (100 + WAIT_MS)
#define STACKSZ    512

void thread_sema(void *arg)
{
	bool status;

	/* Try taking semaphore immediately when it is not available */
	status = os_sem_take((struct k_sem *)arg, 0);
	zassert_true(status == false, "Semaphore acquired unexpectedly!");

	/* Try taking semaphore after a TIMEOUT, but before release */
	status = os_sem_take((struct k_sem *)arg, WAIT_MS);
	zassert_true(status == false, "Semaphore acquired unexpectedly!");

	/* This delay ensures that the semaphore gets released by the other
	 * thread in the meantime
	 */
	os_delay(TIMEOUT_MS);

	/* Now that the semaphore is free, it should be possible to acquire
	 * and release it.
	 */
	status = os_sem_take((struct k_sem *)arg, 0);
	zassert_true(status == true, "Semaphore could not be acquired");

	zassert_true(os_sem_give((struct k_sem *)arg) == true, "Semaphore release failure");

	/* Try releasing when no semaphore is obtained */
	zassert_true(os_sem_give((struct k_sem *)arg) == false, "Semaphore released unexpectedly!");
}

ZTEST(osif_semaphore, test_semaphore)
{
	void *semaphore_id;
	bool status;
	void *tid;
	void *dummy_sem_id = NULL;

	status = os_sem_create(&semaphore_id, "mySemaphore", 1, 1);
	zassert_true(status != false, "semaphore creation failed");

	status = os_task_create(&tid, "Sema_check", thread_sema, semaphore_id, STACKSZ, 3);
	zassert_true(status != false, "Failed creating thread3");

	/* Acquire invalid semaphore */
	zassert_equal(os_sem_take(dummy_sem_id, 0xFFFFFFFF), false,
		      "Semaphore wait worked unexpectedly");

	status = os_sem_take(semaphore_id, 0xFFFFFFFF);
	zassert_true(status == true, "Semaphore wait failure");

	/* wait for spawn thread to take action */
	os_delay(TIMEOUT_MS);

	/* Release invalid semaphore */
	zassert_equal(os_sem_give(dummy_sem_id), false, "Semaphore release worked unexpectedly");

	/* Release the semaphore to be used by the other thread */
	status = os_sem_give(semaphore_id);
	zassert_true(status == true, "Semaphore release failure");

	os_delay(TIMEOUT_MS);

	/* Delete invalid semaphore */
	zassert_equal(os_sem_delete(dummy_sem_id), false, "Semaphore delete worked unexpectedly");

	status = os_sem_delete(semaphore_id);
	zassert_true(status == true, "semaphore delete failure");

	status = os_task_delete(tid);
	zassert_true(status == true, "Error deleting Sema_check task");
}
ZTEST_SUITE(osif_semaphore, NULL, NULL, NULL, NULL, NULL);
