/*
 * Copyright (c) 2026, Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include "memref_testcases.h"

MEMREF_K_HEAP_DEFINE(mempool_backend, 4096);

static void *mempool_alloc(void)
{
	return memref_alloc(&mempool_backend, 64);
}

static void *mempool_alloc_cb(memref_destroy_cb_t destroy)
{
#ifdef CONFIG_MEMREF_DESTROY_CB
	return memref_alloc_with_destroy(&mempool_backend, 64, destroy);
#else
	ARG_UNUSED(destroy);

	return NULL;
#endif
}

MEMREF_VARLEN_TESTCASES(memref_mempool, &mempool_backend, mempool_alloc, mempool_alloc_cb,
			sizeof(void *));
