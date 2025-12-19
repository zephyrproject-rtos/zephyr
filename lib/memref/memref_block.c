/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/memref.h>
#include <zephyr/sys/zassert.h>

#include "memref_priv.h"

ZASSERT_MODULE(DEFAULT);

void *z_memref_k_mem_slab_alloc(void *ctx, size_t size)
{
	void *mem;

	ARG_UNUSED(size);

	if (k_mem_slab_alloc((struct k_mem_slab *)ctx, &mem, K_NO_WAIT) != 0) {
		return NULL;
	}

	return mem;
}

void z_memref_k_mem_slab_free(void *ctx, void *mem)
{
	k_mem_slab_free((struct k_mem_slab *)ctx, mem);
}

void *memref_block_alloc(const struct memref_block_backend *be)
{
	struct memref_ctrl_block *ctrl;

	ZASSERT(be != NULL && be->base.alloc != NULL && be->base.free != NULL);

	ctrl = be->base.alloc(be->base.ctx, MEMREF_CTRL_BLOCK_SIZE);
	if (ctrl == NULL) {
		return NULL;
	}

	return z_memref_ctrl_init(ctrl, &be->base);
}

#ifdef CONFIG_MEMREF_DESTROY_CB
void *memref_block_alloc_with_destroy(const struct memref_block_backend *be,
				      memref_destroy_cb_t destroy)
{
	void *mem = memref_block_alloc(be);

	if (mem != NULL) {
		memref_ctrl_from_mem(mem)->destroy = destroy;
	}

	return mem;
}
#endif /* CONFIG_MEMREF_DESTROY_CB */
