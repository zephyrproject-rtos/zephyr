/*
 * Copyright (c) 2025, Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memref_mempool, LOG_LEVEL_DBG);

#include "memref_testcases.h"

K_HEAP_DEFINE(mempool, 4096);

void *k_heap_alloc_wrapper(void *ctx, size_t size)
{
	return k_heap_alloc((struct k_heap *)ctx, size, K_NO_WAIT);
}

void k_heap_free_wrapper(void *ctx, void *mem)
{
	k_heap_free((struct k_heap *)ctx, mem);
}

MEMREF_BACKEND_DEFINE(mempool_backend, k_heap_alloc_wrapper, k_heap_free_wrapper, &mempool);

MEMREF_TESTCASES(memref_mempool, &mempool_backend);
