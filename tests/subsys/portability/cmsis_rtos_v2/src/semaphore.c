/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/portability/cmsis_os2.h>
#include <zephyr/portability/cmsis_types.h>

#define WAIT_TICKS    5
#define TIMEOUT_TICKS (10 + WAIT_TICKS)
#define STACKSZ       CONFIG_CMSIS_V2_THREAD_MAX_STACK_SIZE

void thread_sema(void *arg)
{
	osStatus_t status;

	/* Try taking semaphore immediately when it is not available */
	status = osSemaphoreAcquire((osSemaphoreId_t)arg, 0);
	zassert_true(status == osErrorResource, "Semaphore acquired unexpectedly!");

	/* Try taking semaphore after a TIMEOUT, but before release */
	status = osSemaphoreAcquire((osSemaphoreId_t)arg, WAIT_TICKS);
	zassert_true(status == osErrorTimeout, "Semaphore acquired unexpectedly!");

	/* This delay ensures that the semaphore gets released by the other
	 * thread in the meantime
	 */
	osDelay(TIMEOUT_TICKS);

	/* Now that the semaphore is free, it should be possible to acquire
	 * and release it.
	 */
	status = osSemaphoreAcquire((osSemaphoreId_t)arg, 0);
	zassert_true(status == osOK, "Semaphore could not be acquired");

	zassert_true(osSemaphoreRelease((osSemaphoreId_t)arg) == osOK, "Semaphore release failure");

	/* Try releasing when no semaphore is obtained */
	zassert_true(osSemaphoreRelease((osSemaphoreId_t)arg) == osErrorResource,
		     "Semaphore released unexpectedly!");
}

static K_THREAD_STACK_DEFINE(test_stack, STACKSZ);
static osThreadAttr_t thread_attr = {.name = "Sema_check",
				     .attr_bits = osThreadDetached,
				     .cb_mem = NULL,
				     .cb_size = 0,
				     .stack_mem = &test_stack,
				     .stack_size = STACKSZ,
				     .priority = osPriorityNormal,
				     .tz_module = 0,
				     .reserved = 0};

const osSemaphoreAttr_t sema_attr = {"mySemaphore", 0, NULL, 0U};

ZTEST(cmsis_semaphore, test_semaphore)
{
	osThreadId_t id;
	osStatus_t status;
	osSemaphoreId_t semaphore_id;
	osSemaphoreId_t dummy_sem_id = NULL;
	const char *name;

	semaphore_id = osSemaphoreNew(1, 1, &sema_attr);
	zassert_true(semaphore_id != NULL, "semaphore creation failed");

	name = osSemaphoreGetName(semaphore_id);
	zassert_str_equal(sema_attr.name, name, "Error getting Semaphore name");

	id = osThreadNew(thread_sema, semaphore_id, &thread_attr);
	zassert_true(id != NULL, "Thread creation failed");

	zassert_true(osSemaphoreGetCount(semaphore_id) == 1);

	/* Acquire invalid semaphore */
	zassert_equal(osSemaphoreAcquire(dummy_sem_id, osWaitForever), osErrorParameter,
		      "Semaphore wait worked unexpectedly");

	status = osSemaphoreAcquire(semaphore_id, osWaitForever);
	zassert_true(status == osOK, "Semaphore wait failure");

	zassert_true(osSemaphoreGetCount(semaphore_id) == 0);

	/* wait for spawn thread to take action */
	osDelay(TIMEOUT_TICKS);

	/* Release invalid semaphore */
	zassert_equal(osSemaphoreRelease(dummy_sem_id), osErrorParameter,
		      "Semaphore release worked unexpectedly");

	/* Release the semaphore to be used by the other thread */
	status = osSemaphoreRelease(semaphore_id);
	zassert_true(status == osOK, "Semaphore release failure");

	osDelay(TIMEOUT_TICKS);

	/* Delete invalid semaphore */
	zassert_equal(osSemaphoreDelete(dummy_sem_id), osErrorParameter,
		      "Semaphore delete worked unexpectedly");

	status = osSemaphoreDelete(semaphore_id);
	zassert_true(status == osOK, "semaphore delete failure");
}

static struct cmsis_rtos_semaphore_cb semaphore_cb2;
static const osSemaphoreAttr_t semaphore_attrs2 = {
	.name = "Semaphore2",
	.attr_bits = 0,
	.cb_mem = &semaphore_cb2,
	.cb_size = sizeof(semaphore_cb2),
};
ZTEST(cmsis_semaphore, test_semaphore_static_allocation)
{
	osSemaphoreId_t id;

	id = osSemaphoreNew(1, 1, &semaphore_attrs2);
	zassert_not_null(id, "Failed creating semaphores using static cb");

	zassert_true(osSemaphoreDelete(id) == osOK, "semaphore delete failure");
}
ZTEST_SUITE(cmsis_semaphore, NULL, NULL, NULL, NULL, NULL);
