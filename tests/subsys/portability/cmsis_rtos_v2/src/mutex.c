/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <cmsis_os2.h>

#define TIMEOUT_TICKS   10
#define STACKSZ         CONFIG_CMSIS_V2_THREAD_MAX_STACK_SIZE

int max_mtx_cnt = CONFIG_CMSIS_V2_MUTEX_MAX_COUNT;
const osMutexAttr_t mutex_attr = {
	"myMutex",
	osMutexRecursive | osMutexPrioInherit,
	NULL,
	0U
};

void cleanup_max_mutex(osMutexId_t *mutex_ids)
{
	int mutex_count = 0;
	osStatus_t status;

	for (mutex_count = 0; mutex_count < max_mtx_cnt; mutex_count++) {
		status = osMutexDelete(mutex_ids[mutex_count]);
		zassert_true(status == osOK, "Mutex delete fail");
	}
}

void test_max_mutex(void)
{
	osMutexId_t mutex_ids[CONFIG_CMSIS_V2_MUTEX_MAX_COUNT + 1];
	int mtx_cnt = 0;

	/* Try mutex creation for more than maximum count */
	for (mtx_cnt = 0; mtx_cnt < max_mtx_cnt + 1; mtx_cnt++) {
		mutex_ids[mtx_cnt] = osMutexNew(&mutex_attr);
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
	osMutexId_t mutex_id = 0;
	osThreadId_t id;
	osStatus_t status;
	const char *name;

	/* Try deleting invalid mutex object */
	status = osMutexDelete(mutex_id);
	zassert_true(status == osErrorParameter,
		     "Invalid Mutex deleted unexpectedly!");

	mutex_id = osMutexNew(&mutex_attr);
	zassert_true(mutex_id != NULL, "Mutex1 creation failed");

	name = osMutexGetName(mutex_id);
	zassert_true(strcmp(mutex_attr.name, name) == 0,
		     "Error getting Mutex name");

	/* Try to release mutex without obtaining it */
	status = osMutexRelease(mutex_id);
	zassert_true(status == osErrorResource, "Mutex released unexpectedly!");

	/* Try figuring out the owner for a Mutex which has not been
	 * acquired by any thread yet.
	 */
	id = osMutexGetOwner(mutex_id);
	zassert_true(id == NULL, "Something wrong with MutexGetOwner!");

	status = osMutexAcquire(mutex_id, 0);
	zassert_true(status == osOK, "Mutex wait failure");

	id = osMutexGetOwner(mutex_id);
	zassert_equal(id, osThreadGetId(), "Current thread is not the owner!");

	/* Try to acquire an already acquired mutex */
	status = osMutexAcquire(mutex_id, 0);
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

void tThread_entry_lock_timeout(void *arg)
{
	osStatus_t status;
	osThreadId_t id;

	/* Mutex cannot be acquired/released here as it is still held
	 * by the other thread. Try with and without timeout.
	 */
	status = osMutexAcquire((osMutexId_t)arg, 0);
	zassert_true(status == osErrorResource);

	status = osMutexAcquire((osMutexId_t)arg, TIMEOUT_TICKS - 5);
	zassert_true(status == osErrorTimeout);

	status = osMutexRelease((osMutexId_t)arg);
	zassert_true(status == osErrorResource, "Mutex unexpectedly released");

	id = osMutexGetOwner((osMutexId_t)arg);
	zassert_not_equal(id, osThreadGetId(),
			  "Unexpectedly, current thread is the mutex owner!");

	/* This delay ensures that the mutex gets released by the other
	 * thread in the meantime
	 */
	osDelay(TIMEOUT_TICKS);

	/* Now that the mutex is free, it should be possible to acquire
	 * and release it.
	 */
	status = osMutexAcquire((osMutexId_t)arg, TIMEOUT_TICKS);
	zassert_true(status == osOK);
	osMutexRelease((osMutexId_t)arg);
}

static K_THREAD_STACK_DEFINE(test_stack, STACKSZ);
static osThreadAttr_t thread_attr = {
	.name = "Mutex_check",
	.attr_bits = osThreadDetached,
	.cb_mem = NULL,
	.cb_size = 0,
	.stack_mem = &test_stack,
	.stack_size = STACKSZ,
	.priority = osPriorityNormal,
	.tz_module = 0,
	.reserved = 0
};

ZTEST(cmsis_mutex, test_mutex_lock_timeout)
{
	osThreadId_t id;
	osMutexId_t mutex_id;
	osStatus_t status;

	mutex_id = osMutexNew(&mutex_attr);
	zassert_true(mutex_id != NULL, "Mutex2 creation failed");

	id = osThreadNew(tThread_entry_lock_timeout, mutex_id, &thread_attr);
	zassert_true(id != NULL, "Thread creation failed");

	status = osMutexAcquire(mutex_id, osWaitForever);
	zassert_true(status == osOK, "Mutex wait failure");

	/* wait for spawn thread to take action */
	osDelay(TIMEOUT_TICKS);

	/* Release the mutex to be used by the other thread */
	osMutexRelease(mutex_id);
	osDelay(TIMEOUT_TICKS);

	osMutexDelete(mutex_id);
}
ZTEST_SUITE(cmsis_mutex, NULL, NULL, NULL, NULL, NULL);
