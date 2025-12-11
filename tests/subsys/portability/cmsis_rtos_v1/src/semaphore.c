/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <cmsis_os.h>

#define TIMEOUT 500

osSemaphoreDef(semaphore_1);

void thread_sema(void const *arg)
{
	int tokens_available;

	/* Try taking semaphore immediately when it is not available */
	tokens_available = osSemaphoreWait((osSemaphoreId)arg, 0);
	zassert_true(tokens_available == 0,
			"Semaphore acquired unexpectedly!");

	/* Try taking semaphore after a TIMEOUT, but before release */
	tokens_available = osSemaphoreWait((osSemaphoreId)arg, TIMEOUT - 100);
	zassert_true(tokens_available == 0,
			"Semaphore acquired unexpectedly!");

	/* This delay ensures that the semaphore gets released by the other
	 * thread in the meantime
	 */
	osDelay(TIMEOUT - 100);

	/* Now that the semaphore is free, it should be possible to acquire
	 * and release it.
	 */
	tokens_available = osSemaphoreWait((osSemaphoreId)arg, 0);
	zassert_true(tokens_available > 0);

	zassert_true(osSemaphoreRelease((osSemaphoreId)arg) == osOK,
			"Semaphore release failure");

	/* Try releasing when no semaphore is obtained */
	zassert_true(osSemaphoreRelease((osSemaphoreId)arg) == osErrorResource,
			"Semaphore released unexpectedly!");
}

osThreadDef(thread_sema, osPriorityNormal, 1, 0);

ZTEST(cmsis_semaphore, test_semaphore)
{
	osThreadId id;
	osStatus status;
	osSemaphoreId semaphore_id;

	semaphore_id = osSemaphoreCreate(osSemaphore(semaphore_1), 1);
	zassert_true(semaphore_id != NULL, "semaphore creation failed");

	id = osThreadCreate(osThread(thread_sema), semaphore_id);
	zassert_true(id != NULL, "Thread creation failed");

	zassert_true(osSemaphoreWait(semaphore_id, osWaitForever) > 0,
			"Semaphore wait failure");

	/* wait for spawn thread to take action */
	osDelay(TIMEOUT);

	/* Release the semaphore to be used by the other thread */
	status = osSemaphoreRelease(semaphore_id);
	zassert_true(status == osOK, "Semaphore release failure");

	osDelay(TIMEOUT);

	status = osSemaphoreDelete(semaphore_id);
	zassert_true(status == osOK, "semaphore delete failure");
}
ZTEST_SUITE(cmsis_semaphore, NULL, NULL, NULL, NULL, NULL);
