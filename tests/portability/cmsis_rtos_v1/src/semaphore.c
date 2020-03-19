/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_os.h>

#define TIMEOUT 500

osSemaphoreDef(semaphore_1);

void thread_sema(void const *arg)
{
	int tokens_available;

	/* Try taking semaphore immediately when it is not available */
	tokens_available = osSemaphoreWait((osSemaphoreId)arg, 0);
	ztest_true(tokens_available == 0,
			"Semaphore acquired unexpectedly!");

	/* Try taking semaphore after a TIMEOUT, but before release */
	tokens_available = osSemaphoreWait((osSemaphoreId)arg, TIMEOUT - 100);
	ztest_true(tokens_available == 0,
			"Semaphore acquired unexpectedly!");

	/* This delay ensures that the semaphore gets released by the other
	 * thread in the meantime
	 */
	osDelay(TIMEOUT - 100);

	/* Now that the semaphore is free, it should be possible to acquire
	 * and release it.
	 */
	tokens_available = osSemaphoreWait((osSemaphoreId)arg, 0);
	ztest_true(tokens_available > 0, NULL);

	ztest_true(osSemaphoreRelease((osSemaphoreId)arg) == osOK,
			"Semaphore release failure");

	/* Try releasing when no semaphore is obtained */
	ztest_true(osSemaphoreRelease((osSemaphoreId)arg) == osErrorResource,
			"Semaphore released unexpectedly!");
}

osThreadDef(thread_sema, osPriorityNormal, 1, 0);

void test_semaphore(void)
{
	osThreadId id;
	osStatus status;
	osSemaphoreId semaphore_id;

	semaphore_id = osSemaphoreCreate(osSemaphore(semaphore_1), 1);
	ztest_true(semaphore_id != NULL, "semaphore creation failed");

	id = osThreadCreate(osThread(thread_sema), semaphore_id);
	ztest_true(id != NULL, "Thread creation failed");

	ztest_true(osSemaphoreWait(semaphore_id, osWaitForever) > 0,
			"Semaphore wait failure");

	/* wait for spawn thread to take action */
	osDelay(TIMEOUT);

	/* Release the semaphore to be used by the other thread */
	status = osSemaphoreRelease(semaphore_id);
	ztest_true(status == osOK, "Semaphore release failure");

	osDelay(TIMEOUT);

	status = osSemaphoreDelete(semaphore_id);
	ztest_true(status == osOK, "semaphore delete failure");
}
