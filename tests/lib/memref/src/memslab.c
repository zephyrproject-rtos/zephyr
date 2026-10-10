/*
 * Copyright (c) 2026, Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memref_memslab, LOG_LEVEL_DBG);

#include "memref_testcases.h"

#define BLOCK_SIZE (MEMREF_CTRL_BLOCK_SIZE + 128)
K_MEM_SLAB_DEFINE(memslab, BLOCK_SIZE, 10, 4);

void *memslab_alloc_wrapper(void *ctx, size_t size)
{
	void *mem;

	ARG_UNUSED(size);

	if (k_mem_slab_alloc(ctx, &mem, K_NO_WAIT) != 0) {
		return NULL;
	}

	return mem;
}

void memslab_free_wrapper(void *ctx, void *mem)
{
	k_mem_slab_free(ctx, mem);
}

MEMREF_BACKEND_DEFINE(memslab_backend, memslab_alloc_wrapper, memslab_free_wrapper, &memslab);

MEMREF_TESTCASES(memref_memslab, &memslab_backend);
