/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <cmsis_os.h>

#define TIMEOUT 500

osMutexDef(Mutex_1);
osMutexDef(Mutex_2);

void test_mutex(void)
{
	osMutexId mutex_id;
	osStatus status;

	mutex_id = osMutexCreate(osMutex(Mutex_1));
	zassert_true(mutex_id != NULL, "Mutex1 creation failed");

	status = osMutexWait(mutex_id, 0);
	zassert_true(status == osOK, "Mutex wait failure");

	status = osMutexRelease(mutex_id);
	zassert_true(status == osOK, "Mutex release failure");

	status = osMutexDelete(mutex_id);
	zassert_true(status == osOK, "Mutex delete failure");
}

void tThread_entry_lock_timeout(void const *arg)
{
	osStatus status;

	/* Mutex cannot be acquired here as it is still held by the
	 * other thread.
	 */
	status = osMutexWait((osMutexId)arg, TIMEOUT - 100);
	zassert_true(status == osErrorTimeoutResource, NULL);

	/* This delay ensures that the mutex gets released by the other
	 * thread in the meantime
	 */
	osDelay(TIMEOUT - 100);

	/* Now that the mutex is free, it should be possible to acquire
	 * and release it.
	 */
	status = osMutexWait((osMutexId)arg, TIMEOUT);
	zassert_true(status == osOK, NULL);
	osMutexRelease((osMutexId)arg);
}

osThreadDef(tThread_entry_lock_timeout, osPriorityNormal, 1, 0);

void test_mutex_lock_timeout(void)
{
	osThreadId id;
	osMutexId mutex_id;
	osStatus status;

	mutex_id = osMutexCreate(osMutex(Mutex_2));
	zassert_true(mutex_id != NULL, "Mutex2 creation failed");

	id = osThreadCreate(osThread(tThread_entry_lock_timeout), mutex_id);
	zassert_true(id != NULL, "Thread creation failed");

	status = osMutexWait(mutex_id, osWaitForever);
	zassert_true(status == osOK, "Mutex wait failure");

	/* wait for spawn thread to take action */
	osDelay(TIMEOUT);

	/* Release the mutex to be used by the other thread */
	osMutexRelease(mutex_id);
	osDelay(TIMEOUT);

	osMutexDelete(mutex_id);
}
