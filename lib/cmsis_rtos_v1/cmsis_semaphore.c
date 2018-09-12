/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <cmsis_os.h>

K_MEM_SLAB_DEFINE(cmsis_semaphore_slab, sizeof(struct k_sem),
		CONFIG_CMSIS_SEMAPHORE_MAX_COUNT, 4);

/**
 * @brief Create and Initialize a semaphore object.
 */
osSemaphoreId osSemaphoreCreate(const osSemaphoreDef_t *semaphore_def,
				int32_t count)
{
	struct k_sem *semaphore;

	if (semaphore_def == NULL) {
		return NULL;
	}

	if (_is_in_isr()) {
		return NULL;
	}

	if (k_mem_slab_alloc(&cmsis_semaphore_slab,
				(void **)&semaphore, 100) == 0) {
		(void)memset(semaphore, 0, sizeof(struct k_sem));
	} else {
		return NULL;
	}

	k_sem_init(semaphore, count, count);

	return (osSemaphoreId)semaphore;
}

/**
 * @brief Wait until a semaphore becomes available.
 */
int32_t osSemaphoreWait(osSemaphoreId semaphore_id, uint32_t timeout)
{
	struct k_sem *semaphore = (struct k_sem *) semaphore_id;
	int status;

	if (semaphore_id == NULL) {
		return -1;
	}

	if (_is_in_isr()) {
		return -1;
	}

	if (timeout == osWaitForever) {
		status = k_sem_take(semaphore, K_FOREVER);
	} else if (timeout == 0) {
		status = k_sem_take(semaphore, K_NO_WAIT);
	} else {
		status = k_sem_take(semaphore, timeout);
	}

	/* If k_sem_take is successful, return the number of available
	 * tokens + 1. +1 is for accounting the currently acquired token.
	 * If it has timed out, return 0 (no tokens available).
	 */
	if (status == 0) {
		return k_sem_count_get(semaphore) + 1;
	} else {
		return 0;
	}
}

/**
 * @brief Release a semaphore that was obtained by osSemaphoreWait.
 */
osStatus osSemaphoreRelease(osSemaphoreId semaphore_id)
{
	struct k_sem *semaphore = (struct k_sem *) semaphore_id;

	if (semaphore_id == NULL) {
		return osErrorParameter;
	}

	/* All tokens have already been released */
	if (k_sem_count_get(semaphore) == semaphore->limit) {
		return osErrorResource;
	}

	k_sem_give(semaphore);

	return osOK;
}

/**
 * @brief Delete a semaphore that was created by osSemaphoreCreate.
 */
osStatus osSemaphoreDelete(osSemaphoreId semaphore_id)
{
	struct k_sem *semaphore = (struct k_sem *) semaphore_id;

	if (semaphore_id == NULL) {
		return osErrorParameter;
	}

	if (_is_in_isr()) {
		return osErrorISR;
	}

	/* The status code "osErrorResource" (the semaphore object
	 * could not be deleted) is not supported in Zephyr.
	 */

	k_mem_slab_free(&cmsis_semaphore_slab, (void *) &semaphore);

	return osOK;
}
