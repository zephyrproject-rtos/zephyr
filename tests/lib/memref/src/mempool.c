/*
 * Copyright (c) 2026, Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memref_mempool, LOG_LEVEL_DBG);

#include "memref_testcases.h"

MEMREF_K_HEAP_DEFINE(mempool_backend, 4096);

static void *mempool_alloc(void)
{
	return memref_alloc(&mempool_backend, 64);
}

#ifdef CONFIG_MEMREF_DESTROY_CB
static void *mempool_alloc_cb(memref_destroy_cb_t destroy)
{
	return memref_alloc_with_destroy(&mempool_backend, 64, destroy);
}
#endif

MEMREF_VARLEN_TESTCASES(memref_mempool, &mempool_backend, mempool_alloc, mempool_alloc_cb);
