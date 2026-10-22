/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * Implementation based on: tests/subsys/portability/cmsis_rtos_v2/src/mutex.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <os_task.h>
#include <os_sched.h>
#include <os_sync.h>

#define WAIT_MS    50
#define TIMEOUT_MS (100 + WAIT_MS)
#define STACKSZ    512

ZTEST(osif_mutex, test_mutex)
{
	void *mutex_id = NULL;
	bool status;

	/* Try deleting invalid mutex object */
	status = os_mutex_delete(mutex_id);
	zassert_true(status == false, "Invalid Mutex deleted unexpectedly!");

	status = os_mutex_create(&mutex_id);
	zassert_true(status != false, "Mutex1 creation failed");

	/* Try to release mutex without obtaining it */
	status = os_mutex_give(mutex_id);
	zassert_true(status == false, "Mutex released unexpectedly!");

	status = os_mutex_take(mutex_id, 0);
	zassert_true(status == true, "Mutex wait failure");

	/* Try to acquire an already acquired mutex */
	status = os_mutex_take(mutex_id, 0);
	zassert_true(status == true, "Mutex wait failure");

	status = os_mutex_give(mutex_id);
	zassert_true(status == true, "Mutex release failure");

	/* Release mutex again as it was acquired twice */
	status = os_mutex_give(mutex_id);
	zassert_true(status == true, "Mutex release failure");

	/* Try to release mutex that was already released */
	status = os_mutex_give(mutex_id);
	zassert_true(status == false, "Mutex released unexpectedly!");

	status = os_mutex_delete(mutex_id);
	zassert_true(status == true, "Mutex delete failure");
}

void tThread_entry_lock_timeout(void *arg)
{
	bool status;

	/* Mutex cannot be acquired/released here as it is still held
	 * by the other thread. Try with and without timeout.
	 */
	status = os_mutex_take((struct k_mutex *)arg, 0);
	zassert_true(status == false);

	status = os_mutex_take((struct k_mutex *)arg, WAIT_MS);
	zassert_true(status == false);

	status = os_mutex_give((struct k_mutex *)arg);
	zassert_true(status == false, "Mutex unexpectedly released");

	/* This delay ensures that the mutex gets released by the other
	 * thread in the meantime
	 */
	os_delay(TIMEOUT_MS);

	/* Now that the mutex is free, it should be possible to acquire
	 * and release it.
	 */
	status = os_mutex_take((struct k_mutex *)arg, TIMEOUT_MS);
	zassert_true(status == true);
	os_mutex_give((struct k_mutex *)arg);
}

ZTEST(osif_mutex, test_mutex_lock_timeout)
{
	void *tid;
	void *mutex_id;
	bool status;

	status = os_mutex_create(&mutex_id);
	zassert_true(status != false, "Mutex2 creation failed");

	status = os_task_create(&tid, "Mutex_check", tThread_entry_lock_timeout, mutex_id, STACKSZ,
				3);
	zassert_true(status != false, "Thread creation failed");

	status = os_mutex_take(mutex_id, 0xFFFFFFFF);
	zassert_true(status == true, "Mutex wait failure");

	/* wait for spawn thread to take action */
	os_delay(TIMEOUT_MS);

	/* Release the mutex to be used by the other thread */
	os_mutex_give(mutex_id);
	os_delay(TIMEOUT_MS);

	status = os_mutex_delete(mutex_id);
	zassert_true(status == true, "Mutex delete failure");

	status = os_task_delete(tid);
	zassert_true(status == true, "Error deleting Mutex_check task");
}
ZTEST_SUITE(osif_mutex, NULL, NULL, NULL, NULL, NULL);
