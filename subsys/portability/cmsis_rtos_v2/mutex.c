/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include "wrapper.h"

K_MEM_SLAB_DEFINE(cv2_mutex_slab, sizeof(struct cv2_mutex),
		  CONFIG_CMSIS_V2_MUTEX_MAX_COUNT, 4);

static const osMutexAttr_t init_mutex_attrs = {
	.name = "ZephyrMutex",
	.attr_bits = osMutexPrioInherit,
	.cb_mem = NULL,
	.cb_size = 0,
};

/**
 * @brief Create and Initialize a Mutex object.
 */
osMutexId_t osMutexNew(const osMutexAttr_t *attr)
{
	struct cv2_mutex *mutex;

	if (k_is_in_isr()) {
		return NULL;
	}

	if (attr == NULL) {
		attr = &init_mutex_attrs;
	}

	__ASSERT(attr->attr_bits & osMutexPrioInherit,
		 "Zephyr supports osMutexPrioInherit by default. Do not unselect it\n");

	__ASSERT(!(attr->attr_bits & osMutexRobust),
		 "Zephyr does not support osMutexRobust.\n");

	if (k_mem_slab_alloc(&cv2_mutex_slab, (void **)&mutex, K_MSEC(100)) == 0) {
		memset(mutex, 0, sizeof(struct cv2_mutex));
	} else {
		return NULL;
	}

	k_mutex_init(&mutex->z_mutex);
	mutex->state = attr->attr_bits;

	if (attr->name == NULL) {
		strncpy(mutex->name, init_mutex_attrs.name,
			sizeof(mutex->name) - 1);
	} else {
		strncpy(mutex->name, attr->name, sizeof(mutex->name) - 1);
	}

	return (osMutexId_t)mutex;
}

/**
 * @brief Wait until a Mutex becomes available.
 */
osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout)
{
	struct cv2_mutex *mutex = (struct cv2_mutex *) mutex_id;
	int status;

	if (mutex_id == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (timeout == osWaitForever) {
		status = k_mutex_lock(&mutex->z_mutex, K_FOREVER);
	} else if (timeout == 0U) {
		status = k_mutex_lock(&mutex->z_mutex, K_NO_WAIT);
	} else {
		status = k_mutex_lock(&mutex->z_mutex,
				      K_TICKS(timeout));
	}

	if (timeout != 0 && (status == -EAGAIN || status == -EBUSY)) {
		return osErrorTimeout;
	} else if (status != 0) {
		return osErrorResource;
	} else {
		return osOK;
	}
}

/**
 * @brief Release a Mutex that was obtained by osMutexWait.
 */
osStatus_t osMutexRelease(osMutexId_t mutex_id)
{
	struct cv2_mutex *mutex = (struct cv2_mutex *) mutex_id;

	if (mutex_id == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (k_mutex_unlock(&mutex->z_mutex) != 0) {
		return osErrorResource;
	}

	return osOK;
}

/**
 * @brief Delete a Mutex that was created by osMutexCreate.
 */
osStatus_t osMutexDelete(osMutexId_t mutex_id)
{
	struct cv2_mutex *mutex = (struct cv2_mutex *)mutex_id;

	if (mutex_id == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	/* The status code "osErrorResource" (mutex specified by parameter
	 * mutex_id is in an invalid mutex state) is not supported in Zephyr.
	 */

	k_mem_slab_free(&cv2_mutex_slab, (void *)mutex);

	return osOK;
}


osThreadId_t osMutexGetOwner(osMutexId_t mutex_id)
{
	struct cv2_mutex *mutex = (struct cv2_mutex *)mutex_id;

	if (k_is_in_isr() || (mutex == NULL)) {
		return NULL;
	}

	/* Mutex was not obtained before */
	if (mutex->z_mutex.lock_count == 0U) {
		return NULL;
	}

	return get_cmsis_thread_id(mutex->z_mutex.owner);
}

const char *osMutexGetName(osMutexId_t mutex_id)
{
	struct cv2_mutex *mutex = (struct cv2_mutex *)mutex_id;

	if (k_is_in_isr() || (mutex == NULL)) {
		return NULL;
	}

	return mutex->name;
}
