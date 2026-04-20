/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "os_wrapper.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(os_if_queue);

int rtos_queue_create(rtos_queue_t *pp_handle, uint32_t msg_num, uint32_t msg_size)
{
	struct k_msgq *p_queue;

	if (pp_handle == NULL) {
		return RTK_FAIL;
	}

#if (K_HEAP_MEM_POOL_SIZE > 0)
	p_queue = (struct k_msgq *)k_malloc(sizeof(struct k_msgq));
	if (p_queue == NULL) {
		return RTK_FAIL;
	}
#else
	LOG_ERR("k_malloc not support.");
	return RTK_FAIL;
#endif

	if (k_msgq_alloc_init(p_queue, msg_size, msg_num) != 0) {
		k_free(p_queue);
		return RTK_FAIL;
	}

	*pp_handle = p_queue;
	return RTK_SUCCESS;
}

int rtos_queue_delete(rtos_queue_t p_handle)
{
	int status = RTK_SUCCESS;

	if (p_handle == NULL) {
		return RTK_FAIL;
	}

	if (rtos_queue_message_waiting(p_handle) != 0) {
		status = RTK_FAIL;
		LOG_ERR("The queue is not empty, but the queue has been deleted.");
		k_msgq_purge(p_handle);
	}

	k_msgq_cleanup(p_handle);
	k_free(p_handle);
	return status;
}

uint32_t rtos_queue_message_waiting(rtos_queue_t p_handle)
{
	if (p_handle == NULL) {
		return RTK_SUCCESS;
	}

	return k_msgq_num_used_get(p_handle);
}

int rtos_queue_send(rtos_queue_t p_handle, void *p_msg, uint32_t wait_ms)
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

	if (k_msgq_put(p_handle, p_msg, wait_ticks) == 0) {
		return RTK_SUCCESS;
	} else {
		return RTK_FAIL;
	}
}

int rtos_queue_send_to_front(rtos_queue_t p_handle, void *p_msg, uint32_t wait_ms)
{
	ARG_UNUSED(p_handle);
	ARG_UNUSED(p_msg);
	ARG_UNUSED(wait_ms);
	LOG_ERR("Not Support");
	return RTK_FAIL;
}

int rtos_queue_receive(rtos_queue_t p_handle, void *p_msg, uint32_t wait_ms)
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

	if (k_msgq_get(p_handle, p_msg, wait_ticks) == 0) {
		return RTK_SUCCESS;
	} else {
		return RTK_FAIL;
	}
}

int rtos_queue_peek(rtos_queue_t p_handle, void *p_msg, uint32_t wait_ms)
{
	if (p_handle == NULL) {
		return RTK_FAIL;
	}

	if (wait_ms != 0) {
		LOG_ERR("does not support waiting.");
	}

	if (k_msgq_peek(p_handle, p_msg) == 0) {
		return RTK_SUCCESS;
	} else {
		return RTK_FAIL;
	}
}
