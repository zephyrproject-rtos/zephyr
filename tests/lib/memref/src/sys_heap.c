/*
 * Copyright (c) 2025, Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memref_sys_heap, LOG_LEVEL_DBG);

#include "memref_testcases.h"

void *k_malloc_wrapper(void *ctx, size_t size)
{
	(void)ctx;

	return k_malloc(size);
}

void k_free_wrapper(void *ctx, void *mem)
{
	(void)ctx;

	k_free(mem);
}

MEMREF_BACKEND_DEFINE(sys_heap, k_malloc_wrapper, k_free_wrapper, NULL);

MEMREF_TESTCASES(memref_sys_heap, &sys_heap);
