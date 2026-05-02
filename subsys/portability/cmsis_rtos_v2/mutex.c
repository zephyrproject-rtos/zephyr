/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/portability/cmsis_types.h>
#include <string.h>
#include "wrapper.h"

K_MEM_SLAB_DEFINE(cmsis_rtos_mutex_cb_slab, sizeof(struct cmsis_rtos_mutex_cb),
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
	struct cmsis_rtos_mutex_cb *mutex;

	if (k_is_in_isr()) {
		return NULL;
	}

	if (attr == NULL) {
		attr = &init_mutex_attrs;
	}

	__ASSERT(attr->attr_bits & osMutexPrioInherit,
		 "Zephyr supports osMutexPrioInherit by default. Do not unselect it\n");

	__ASSERT(!(attr->attr_bits & osMutexRobust), "Zephyr does not support osMutexRobust.\n");

	if (attr->cb_mem != NULL) {
		__ASSERT(attr->cb_size == sizeof(struct cmsis_rtos_mutex_cb), "Invalid cb_size\n");
		mutex = (struct cmsis_rtos_mutex_cb *)attr->cb_mem;
	} else if (k_mem_slab_alloc(&cmsis_rtos_mutex_cb_slab, (void **)&mutex, K_MSEC(100)) != 0) {
		return NULL;
	}
	memset(mutex, 0, sizeof(struct cmsis_rtos_mutex_cb));
	mutex->is_cb_dynamic_allocation = attr->cb_mem == NULL;

	k_mutex_init(&mutex->z_mutex);
	mutex->state = attr->attr_bits;

	mutex->name = (attr->name == NULL) ? init_mutex_attrs.name : attr->name;

	return (osMutexId_t)mutex;
}

/**
 * @brief Wait until a Mutex becomes available.
 */
osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout)
{
	struct cmsis_rtos_mutex_cb *mutex = (struct cmsis_rtos_mutex_cb *)mutex_id;
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
		status = k_mutex_lock(&mutex->z_mutex, K_TICKS(timeout));
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
	struct cmsis_rtos_mutex_cb *mutex = (struct cmsis_rtos_mutex_cb *)mutex_id;

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
	struct cmsis_rtos_mutex_cb *mutex = (struct cmsis_rtos_mutex_cb *)mutex_id;

	if (mutex_id == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	/* The status code "osErrorResource" (mutex specified by parameter
	 * mutex_id is in an invalid mutex state) is not supported in Zephyr.
	 */
	if (mutex->is_cb_dynamic_allocation) {
		k_mem_slab_free(&cmsis_rtos_mutex_cb_slab, (void *)mutex);
	}

	return osOK;
}

osThreadId_t osMutexGetOwner(osMutexId_t mutex_id)
{
	struct cmsis_rtos_mutex_cb *mutex = (struct cmsis_rtos_mutex_cb *)mutex_id;

	if (k_is_in_isr() || (mutex == NULL)) {
		return NULL;
	}

	/* Mutex was not obtained before */
	if (mutex->z_mutex.lock_count == 0U) {
		return NULL;
	}

	return get_cmsis_thread_id(mutex->z_mutex.owner);
}

/**
 * @brief Get name of a mutex.
 * This function may be called from Interrupt Service Routines.
 */
const char *osMutexGetName(osMutexId_t mutex_id)
{
	struct cmsis_rtos_mutex_cb *mutex = (struct cmsis_rtos_mutex_cb *)mutex_id;

	if (mutex == NULL) {
		return NULL;
	}
	return mutex->name;
}
