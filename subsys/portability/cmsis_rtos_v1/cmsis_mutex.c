/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <cmsis_os.h>
#include <string.h>

K_MEM_SLAB_DEFINE(cmsis_mutex_slab, sizeof(struct k_mutex),
		CONFIG_CMSIS_MUTEX_MAX_COUNT, 4);

/**
 * @brief Create and Initialize a Mutex object.
 */
osMutexId osMutexCreate(const osMutexDef_t *mutex_def)
{
	struct k_mutex *mutex;

	if (mutex_def == NULL) {
		return NULL;
	}

	if (k_is_in_isr()) {
		return NULL;
	}

	if (k_mem_slab_alloc(&cmsis_mutex_slab, (void **)&mutex, K_MSEC(100)) == 0) {
		(void)memset(mutex, 0, sizeof(struct k_mutex));
	} else {
		return NULL;
	}

	k_mutex_init(mutex);

	return (osMutexId)mutex;
}

/**
 * @brief Wait until a Mutex becomes available.
 */
osStatus osMutexWait(osMutexId mutex_id, uint32_t timeout)
{
	struct k_mutex *mutex = (struct k_mutex *) mutex_id;
	int status;

	if (mutex_id == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (timeout == osWaitForever) {
		status = k_mutex_lock(mutex, K_FOREVER);
	} else if (timeout == 0U) {
		status = k_mutex_lock(mutex, K_NO_WAIT);
	} else {
		status = k_mutex_lock(mutex, K_MSEC(timeout));
	}

	if (timeout != 0 && (status == -EAGAIN || status == -EBUSY)) {
		return osErrorTimeoutResource;
	} else if (status == 0) {
		return osOK;
	} else {
		return osErrorResource;
	}
}

/**
 * @brief Release a Mutex that was obtained by osMutexWait.
 */
osStatus osMutexRelease(osMutexId mutex_id)
{
	struct k_mutex *mutex = (struct k_mutex *) mutex_id;

	if (mutex_id == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (k_mutex_unlock(mutex) != 0) {
		return osErrorResource;
	}

	return osOK;
}

/**
 * @brief Delete a Mutex that was created by osMutexCreate.
 */
osStatus osMutexDelete(osMutexId mutex_id)
{
	struct k_mutex *mutex = (struct k_mutex *) mutex_id;

	if (mutex_id == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	/* The status code "osErrorResource" (mutex object could
	 * not be deleted) is not supported in Zephyr.
	 */

	k_mem_slab_free(&cmsis_mutex_slab, (void *)mutex);

	return osOK;
}
