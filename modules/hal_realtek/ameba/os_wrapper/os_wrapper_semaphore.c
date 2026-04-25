/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "os_wrapper.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(os_if_sema);

int rtos_sema_create(rtos_sema_t *pp_handle, uint32_t init_count, uint32_t max_count)
{
	/* K_SEM_DEFINE or k_malloc for sema struct */
	if (pp_handle == NULL) {
		return RTK_FAIL;
	}

#if (K_HEAP_MEM_POOL_SIZE > 0)
	*pp_handle = k_malloc(sizeof(struct k_sem));
	if (*pp_handle == NULL) {
		return RTK_FAIL;
	}
#else
	LOG_ERR("k_malloc not support. >>>");
	return RTK_FAIL;
#endif

	if (k_sem_init(*pp_handle, init_count, max_count) == 0) {
		return RTK_SUCCESS;
	}

	k_free(*pp_handle);
	return RTK_FAIL;
}

int rtos_sema_create_binary(rtos_sema_t *pp_handle)
{
	return rtos_sema_create(pp_handle, 0, 1); /* init_count = 0 to ensure sema take wait */
}

int rtos_sema_delete(rtos_sema_t p_handle)
{
	if (p_handle == NULL) {
		return RTK_FAIL;
	}

	k_free(p_handle);
	return RTK_SUCCESS;
}

int rtos_sema_take(rtos_sema_t p_handle, uint32_t wait_ms)
{
	k_timeout_t wait_ticks;

	if (p_handle == NULL) {
		return RTK_FAIL;
	}

	if (wait_ms == 0xFFFFFFFFUL) {
		wait_ticks = K_FOREVER;
	} else {
		wait_ticks = K_MSEC(wait_ms);
	}

	if (k_sem_take(p_handle, wait_ticks) == 0) {
		return RTK_SUCCESS;
	} else {
		return RTK_FAIL;
	}
}

int rtos_sema_give(rtos_sema_t p_handle)
{
	struct k_sem *sem = (struct k_sem *)p_handle;

	if (p_handle == NULL) {
		return RTK_FAIL;
	}

	if (k_sem_count_get(sem) >= sem->limit) {
		return RTK_FAIL;
	}

	k_sem_give(p_handle);
	return RTK_SUCCESS;
}

uint32_t rtos_sema_get_count(rtos_sema_t p_handle)
{
	if (p_handle == NULL) {
		return RTK_SUCCESS;
	}

	return k_sem_count_get(p_handle);
}

int rtos_sema_create_static(rtos_sema_t *pp_handle, uint32_t init_count, uint32_t max_count)
{
	return rtos_sema_create(pp_handle, init_count, max_count);
}

int rtos_sema_create_binary_static(rtos_sema_t *pp_handle)
{
	return rtos_sema_create_binary(pp_handle);
}

int rtos_sema_delete_static(rtos_sema_t p_handle)
{
	return rtos_sema_delete(p_handle);
}
