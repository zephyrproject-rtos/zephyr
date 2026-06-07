/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/osal/osal.h>
#include <zephyr/logging/log.h>

#include "osal_priv.h"

LOG_MODULE_DECLARE(osal, CONFIG_OSAL_LOG_LEVEL);

/*
 * The k_msgq control block and its ring storage are allocated as one block so
 * a single handle round-trips through the caller's void* queue type. The
 * storage immediately follows the control block.
 */
struct osal_queue_cb {
	struct k_msgq msgq;
	char buffer[];
};

osal_queue_t osal_queue_create(uint32_t queue_len, uint32_t item_size)
{
	struct osal_queue_cb *q;
	size_t storage = (size_t)queue_len * item_size;

	if (queue_len == 0 || item_size == 0) {
		return NULL;
	}

	q = osal_cb_alloc(sizeof(struct osal_queue_cb) + storage);
	if (q == NULL) {
		LOG_ERR("queue alloc failed");
		return NULL;
	}

	k_msgq_init(&q->msgq, q->buffer, item_size, queue_len);

	return (osal_queue_t)q;
}

int osal_queue_send(osal_queue_t queue, const void *item, uint32_t timeout_ms)
{
	int ret;

	if (queue == NULL || item == NULL) {
		return OSAL_INVAL;
	}

	ret = k_msgq_put(&((struct osal_queue_cb *)queue)->msgq, item,
			 osal_ms_to_timeout(timeout_ms));
	if (ret == 0) {
		return OSAL_OK;
	}

	/* -ENOMSG: full with no wait. -EAGAIN: timed out. Both are "not enqueued". */
	return (ret == -EAGAIN || ret == -ENOMSG) ? OSAL_TIMEOUT : OSAL_ERR;
}

int osal_queue_send_to_front(osal_queue_t queue, const void *item, uint32_t timeout_ms)
{
	int ret;

	ARG_UNUSED(timeout_ms);

	if (queue == NULL || item == NULL) {
		return OSAL_INVAL;
	}

	/* k_msgq_put_front never blocks; the timeout is accepted for API symmetry. */
	ret = k_msgq_put_front(&((struct osal_queue_cb *)queue)->msgq, item);
	if (ret == 0) {
		return OSAL_OK;
	}

	/* -ENOMSG: full. Map to TIMEOUT like osal_queue_send for consistency. */
	return (ret == -ENOMSG) ? OSAL_TIMEOUT : OSAL_ERR;
}

int osal_queue_send_from_isr(osal_queue_t queue, const void *item)
{
	int ret;

	if (queue == NULL || item == NULL) {
		return OSAL_INVAL;
	}

	/* Non-blocking put, safe from ISR context. */
	ret = k_msgq_put(&((struct osal_queue_cb *)queue)->msgq, item, K_NO_WAIT);
	if (ret == 0) {
		return OSAL_OK;
	}

	return (ret == -ENOMSG) ? OSAL_TIMEOUT : OSAL_ERR;
}

int osal_queue_recv(osal_queue_t queue, void *item, uint32_t timeout_ms)
{
	int ret;

	if (queue == NULL || item == NULL) {
		return OSAL_INVAL;
	}

	ret = k_msgq_get(&((struct osal_queue_cb *)queue)->msgq, item,
			 osal_ms_to_timeout(timeout_ms));
	if (ret == 0) {
		return OSAL_OK;
	}

	return (ret == -EAGAIN || ret == -ENOMSG) ? OSAL_TIMEOUT : OSAL_ERR;
}

int osal_queue_peek(osal_queue_t queue, void *item, uint32_t timeout_ms)
{
	ARG_UNUSED(timeout_ms);

	if (queue == NULL || item == NULL) {
		return OSAL_INVAL;
	}

	/* k_msgq_peek is non-blocking; the timeout is accepted for API symmetry. */
	return (k_msgq_peek(&((struct osal_queue_cb *)queue)->msgq, item) == 0)
		       ? OSAL_OK : OSAL_TIMEOUT;
}

uint32_t osal_queue_count(osal_queue_t queue)
{
	if (queue == NULL) {
		return 0;
	}

	return k_msgq_num_used_get(&((struct osal_queue_cb *)queue)->msgq);
}

void osal_queue_delete(osal_queue_t queue)
{
	if (queue == NULL) {
		return;
	}

	k_msgq_purge(&((struct osal_queue_cb *)queue)->msgq);
	osal_cb_free(queue);
}
