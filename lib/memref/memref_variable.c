/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/memref.h>
#include <zephyr/sys/zassert.h>
#include <zephyr/sys/math_extras.h>

#include "memref_priv.h"

ZASSERT_MODULE(DEFAULT);

void *memref_alloc(const struct memref_backend *be, size_t size)
{
	size_t total;
	struct memref_ctrl_block *ctrl;

	ZASSERT(be != NULL && be->alloc != NULL && be->free != NULL);

	if (unlikely(size_add_overflow(size, MEMREF_CTRL_BLOCK_SIZE, &total) || size == 0)) {
		return NULL;
	}

	ctrl = be->alloc(be->ctx, total);
	if (ctrl == NULL) {
		return NULL;
	}

	return z_memref_ctrl_init(ctrl, be);
}

#ifdef CONFIG_MEMREF_DESTROY_CB
void *memref_alloc_with_destroy(const struct memref_backend *be, size_t size,
				memref_destroy_cb_t destroy)
{
	void *mem = memref_alloc(be, size);

	if (mem != NULL) {
		memref_ctrl_from_mem(mem)->destroy = destroy;
	}

	return mem;
}
#endif /* CONFIG_MEMREF_DESTROY_CB */

void *z_memref_k_heap_alloc(void *ctx, size_t size)
{
	return k_heap_alloc((struct k_heap *)ctx, size, K_NO_WAIT);
}

void z_memref_k_heap_free(void *ctx, void *mem)
{
	k_heap_free((struct k_heap *)ctx, mem);
}
