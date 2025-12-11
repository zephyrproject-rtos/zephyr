/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/portability/cmsis_types.h>
#include <string.h>

K_MEM_SLAB_DEFINE(cmsis_rtos_semaphore_cb_slab, sizeof(struct cmsis_rtos_semaphore_cb),
		  CONFIG_CMSIS_V2_SEMAPHORE_MAX_COUNT, 4);

static const osSemaphoreAttr_t init_sema_attrs = {
	.name = "ZephyrSem",
	.attr_bits = 0,
	.cb_mem = NULL,
	.cb_size = 0,
};

/**
 * @brief Create and Initialize a semaphore object.
 */
osSemaphoreId_t osSemaphoreNew(uint32_t max_count, uint32_t initial_count,
			       const osSemaphoreAttr_t *attr)
{
	struct cmsis_rtos_semaphore_cb *semaphore;

	if (k_is_in_isr()) {
		return NULL;
	}

	if (attr == NULL) {
		attr = &init_sema_attrs;
	}

	if (attr->cb_mem != NULL) {
		__ASSERT(attr->cb_size == sizeof(struct cmsis_rtos_semaphore_cb),
			 "Invalid cb_size\n");
		semaphore = (struct cmsis_rtos_semaphore_cb *)attr->cb_mem;
	} else if (k_mem_slab_alloc(&cmsis_rtos_semaphore_cb_slab, (void **)&semaphore,
				    K_MSEC(100)) != 0) {
		return NULL;
	}
	(void)memset(semaphore, 0, sizeof(struct cmsis_rtos_semaphore_cb));
	semaphore->is_cb_dynamic_allocation = attr->cb_mem == NULL;

	k_sem_init(&semaphore->z_semaphore, initial_count, max_count);

	semaphore->name = (attr->name == NULL) ? init_sema_attrs.name : attr->name;

	return (osSemaphoreId_t)semaphore;
}

/**
 * @brief Wait until a semaphore becomes available.
 */
osStatus_t osSemaphoreAcquire(osSemaphoreId_t semaphore_id, uint32_t timeout)
{
	struct cmsis_rtos_semaphore_cb *semaphore = (struct cmsis_rtos_semaphore_cb *)semaphore_id;
	int status;

	if (semaphore_id == NULL) {
		return osErrorParameter;
	}

	/* Can be called from ISRs only if timeout is set to 0 */
	if (timeout > 0 && k_is_in_isr()) {
		return osErrorParameter;
	}

	if (timeout == osWaitForever) {
		status = k_sem_take(&semaphore->z_semaphore, K_FOREVER);
	} else if (timeout == 0U) {
		status = k_sem_take(&semaphore->z_semaphore, K_NO_WAIT);
	} else {
		status = k_sem_take(&semaphore->z_semaphore, K_TICKS(timeout));
	}

	if (status == -EBUSY) {
		return osErrorResource;
	} else if (status == -EAGAIN) {
		return osErrorTimeout;
	} else {
		return osOK;
	}
}

uint32_t osSemaphoreGetCount(osSemaphoreId_t semaphore_id)
{
	struct cmsis_rtos_semaphore_cb *semaphore = (struct cmsis_rtos_semaphore_cb *)semaphore_id;

	if (semaphore_id == NULL) {
		return 0;
	}

	return k_sem_count_get(&semaphore->z_semaphore);
}

/**
 * @brief Release a semaphore that was obtained by osSemaphoreWait.
 */
osStatus_t osSemaphoreRelease(osSemaphoreId_t semaphore_id)
{
	struct cmsis_rtos_semaphore_cb *semaphore = (struct cmsis_rtos_semaphore_cb *)semaphore_id;

	if (semaphore_id == NULL) {
		return osErrorParameter;
	}

	/* All tokens have already been released */
	if (k_sem_count_get(&semaphore->z_semaphore) == semaphore->z_semaphore.limit) {
		return osErrorResource;
	}

	k_sem_give(&semaphore->z_semaphore);

	return osOK;
}

/**
 * @brief Delete a semaphore that was created by osSemaphoreCreate.
 */
osStatus_t osSemaphoreDelete(osSemaphoreId_t semaphore_id)
{
	struct cmsis_rtos_semaphore_cb *semaphore = (struct cmsis_rtos_semaphore_cb *)semaphore_id;

	if (semaphore_id == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	/* The status code "osErrorResource" (the semaphore specified by
	 * parameter semaphore_id is in an invalid semaphore state) is not
	 * supported in Zephyr.
	 */
	if (semaphore->is_cb_dynamic_allocation) {
		k_mem_slab_free(&cmsis_rtos_semaphore_cb_slab, (void *)semaphore);
	}

	return osOK;
}

/**
 * @brief Get name of a semaphore.
 * This function may be called from Interrupt Service Routines.
 */
const char *osSemaphoreGetName(osSemaphoreId_t semaphore_id)
{
	struct cmsis_rtos_semaphore_cb *semaphore = (struct cmsis_rtos_semaphore_cb *)semaphore_id;

	if (semaphore == NULL) {
		return NULL;
	}
	return semaphore->name;
}
