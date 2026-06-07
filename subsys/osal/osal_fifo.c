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
 * Backed by k_queue (the primitive under k_fifo), which provides the prepend
 * and arbitrary-remove operations that k_fifo alone does not expose. Items are
 * caller-owned pointers; the queue uses the first word of each item as its
 * link node.
 */
osal_fifo_t osal_fifo_create(void)
{
	struct k_queue *q = osal_cb_alloc(sizeof(struct k_queue));

	if (q == NULL) {
		LOG_ERR("fifo alloc failed");
		return NULL;
	}

	k_queue_init(q);

	return (osal_fifo_t)q;
}

void osal_fifo_put(osal_fifo_t fifo, void *item)
{
	if (fifo == NULL || item == NULL) {
		return;
	}

	k_queue_append((struct k_queue *)fifo, item);
}

void osal_fifo_put_front(osal_fifo_t fifo, void *item)
{
	if (fifo == NULL || item == NULL) {
		return;
	}

	k_queue_prepend((struct k_queue *)fifo, item);
}

void *osal_fifo_get(osal_fifo_t fifo, uint32_t timeout_ms)
{
	if (fifo == NULL) {
		return NULL;
	}

	return k_queue_get((struct k_queue *)fifo, osal_ms_to_timeout(timeout_ms));
}

bool osal_fifo_remove(osal_fifo_t fifo, void *item)
{
	if (fifo == NULL || item == NULL) {
		return false;
	}

	return k_queue_remove((struct k_queue *)fifo, item);
}

void osal_fifo_delete(osal_fifo_t fifo)
{
	if (fifo == NULL) {
		return;
	}

	osal_cb_free(fifo);
}
