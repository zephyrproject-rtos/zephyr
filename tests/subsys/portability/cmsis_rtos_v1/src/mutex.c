/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <cmsis_os.h>

#define TIMEOUT 500

osMutexDef(Mutex_1);
osMutexDef(Mutex_2);
osMutexDef(Mutex_multi);

int max_mtx_cnt = CONFIG_CMSIS_MUTEX_MAX_COUNT;

void cleanup_max_mutex(osMutexId *mutex_ids)
{
	int mutex_count = 0;
	osStatus status;

	for (mutex_count = 0; mutex_count < max_mtx_cnt; mutex_count++) {
		status = osMutexDelete(mutex_ids[mutex_count]);
		zassert_true(status == osOK, "Mutex delete fail");
	}
}

void test_max_mutex(void)
{
	osMutexId mutex_ids[CONFIG_CMSIS_MUTEX_MAX_COUNT + 1];
	int mtx_cnt = 0;

	/* Try mutex creation for more than maximum count */
	for (mtx_cnt = 0; mtx_cnt < max_mtx_cnt + 1; mtx_cnt++) {
		mutex_ids[mtx_cnt] = osMutexCreate(osMutex(Mutex_multi));
		if (mtx_cnt == max_mtx_cnt) {
			zassert_true(mutex_ids[mtx_cnt] == NULL,
			  "Mutex creation pass unexpectedly after max count");
			cleanup_max_mutex(mutex_ids);
		} else {
			zassert_true(mutex_ids[mtx_cnt] != NULL,
			  "Multiple mutex creation failed before max count");
		}
	}
}

ZTEST(cmsis_mutex, test_mutex)
{
	osMutexId mutex_id = 0;
	osStatus status;

	/* Try deleting invalid mutex object */
	status = osMutexDelete(mutex_id);
	zassert_true(status == osErrorParameter,
		     "Invalid Mutex deleted unexpectedly!");

	mutex_id = osMutexCreate(osMutex(Mutex_1));
	zassert_true(mutex_id != NULL, "Mutex1 creation failed");

	/* Try to release mutex without obtaining it */
	status = osMutexRelease(mutex_id);
	zassert_true(status == osErrorResource, "Mutex released unexpectedly!");

	status = osMutexWait(mutex_id, 0);
	zassert_true(status == osOK, "Mutex wait failure");

	/* Try to acquire an already acquired mutex */
	status = osMutexWait(mutex_id, 0);
	zassert_true(status == osOK, "Mutex wait failure");

	status = osMutexRelease(mutex_id);
	zassert_true(status == osOK, "Mutex release failure");

	/* Release mutex again as it was acquired twice */
	status = osMutexRelease(mutex_id);
	zassert_true(status == osOK, "Mutex release failure");

	/* Try to release mutex that was already released */
	status = osMutexRelease(mutex_id);
	zassert_true(status == osErrorResource, "Mutex released unexpectedly!");

	status = osMutexDelete(mutex_id);
	zassert_true(status == osOK, "Mutex delete failure");

	/* Try mutex creation for more than maximum allowed count */
	test_max_mutex();
}

void tThread_entry_lock_timeout(void const *arg)
{
	osStatus status;

	/* Mutex cannot be acquired here as it is still held by the
	 * other thread. Try with and without timeout.
	 */
	status = osMutexWait((osMutexId)arg, 0);
	zassert_true(status == osErrorResource);

	status = osMutexWait((osMutexId)arg, TIMEOUT - 100);
	zassert_true(status == osErrorTimeoutResource);

	/* At this point, mutex is held by the other thread.
	 * Trying to release it here should fail.
	 */
	status = osMutexRelease((osMutexId)arg);
	zassert_true(status == osErrorResource, "Mutex unexpectedly released");

	/* This delay ensures that the mutex gets released by the other
	 * thread in the meantime
	 */
	osDelay(TIMEOUT - 100);

	/* Now that the mutex is free, it should be possible to acquire
	 * and release it.
	 */
	status = osMutexWait((osMutexId)arg, TIMEOUT);
	zassert_true(status == osOK);
	osMutexRelease((osMutexId)arg);
}

osThreadDef(tThread_entry_lock_timeout, osPriorityNormal, 1, 0);

ZTEST(cmsis_mutex, test_mutex_lock_timeout)
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
ZTEST_SUITE(cmsis_mutex, NULL, NULL, NULL, NULL, NULL);
