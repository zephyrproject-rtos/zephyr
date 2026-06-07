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
 * Fixed-block pool backed by k_mem_slab. k_mem_slab requires the block size to
 * be a multiple of the pointer alignment and the buffer to be aligned, so the
 * block size is rounded up and the backing buffer is allocated with that
 * alignment. The slab control block and buffer are tracked together.
 */
struct osal_mempool_cb {
	struct k_mem_slab slab;
	void *buffer;
};

osal_mempool_t osal_mempool_create(uint32_t block_count, uint32_t block_size)
{
	struct osal_mempool_cb *p;
	size_t aligned = ROUND_UP(block_size, sizeof(void *));

	if (block_count == 0 || block_size == 0) {
		return NULL;
	}

	p = osal_cb_alloc(sizeof(*p));
	if (p == NULL) {
		LOG_ERR("mempool cb alloc failed");
		return NULL;
	}

	p->buffer = k_aligned_alloc(sizeof(void *), aligned * block_count);
	if (p->buffer == NULL) {
		LOG_ERR("mempool buffer alloc failed");
		osal_cb_free(p);
		return NULL;
	}

	if (k_mem_slab_init(&p->slab, p->buffer, aligned, block_count) != 0) {
		k_free(p->buffer);
		osal_cb_free(p);
		return NULL;
	}

	return (osal_mempool_t)p;
}

void *osal_mempool_alloc(osal_mempool_t pool, uint32_t timeout_ms)
{
	struct osal_mempool_cb *p = pool;
	void *block = NULL;

	if (p == NULL) {
		return NULL;
	}

	if (k_mem_slab_alloc(&p->slab, &block, osal_ms_to_timeout(timeout_ms)) != 0) {
		return NULL;
	}

	return block;
}

void osal_mempool_free(osal_mempool_t pool, void *block)
{
	struct osal_mempool_cb *p = pool;

	if (p == NULL || block == NULL) {
		return;
	}

	k_mem_slab_free(&p->slab, block);
}

uint32_t osal_mempool_used(osal_mempool_t pool)
{
	struct osal_mempool_cb *p = pool;

	if (p == NULL) {
		return 0;
	}

	return k_mem_slab_num_used_get(&p->slab);
}

void osal_mempool_delete(osal_mempool_t pool)
{
	struct osal_mempool_cb *p = pool;

	if (p == NULL) {
		return;
	}

	k_free(p->buffer);
	osal_cb_free(p);
}
