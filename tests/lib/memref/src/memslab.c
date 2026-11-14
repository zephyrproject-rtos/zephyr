/*
 * Copyright (c) 2026, Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(memref_memslab, LOG_LEVEL_DBG);

#include "memref_testcases.h"

MEMREF_K_MEM_SLAB_DEFINE(memslab_backend, 128, 10, sizeof(void *));

static void *memslab_alloc(void)
{
	return memref_block_alloc(&memslab_backend);
}

#ifdef CONFIG_MEMREF_DESTROY_CB
static void *memslab_alloc_cb(memref_destroy_cb_t destroy)
{
	return memref_block_alloc_with_destroy(&memslab_backend, destroy);
}
#endif

MEMREF_BLOCK_TESTCASES(memref_memslab, memslab_alloc, memslab_alloc_cb, 10);
