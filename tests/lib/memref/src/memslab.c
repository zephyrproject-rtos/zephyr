/*
 * Copyright (c) 2026, Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

#include "memref_testcases.h"

MEMREF_K_MEM_SLAB_DEFINE(memslab_backend, 128, 10, __alignof__(z_max_align_t));

static void *memslab_alloc(void)
{
	return memref_block_alloc(&memslab_backend);
}

static void *memslab_alloc_cb(memref_destroy_cb_t destroy)
{
#ifdef CONFIG_MEMREF_DESTROY_CB
	return memref_block_alloc_with_destroy(&memslab_backend, destroy);
#else
	ARG_UNUSED(destroy);

	return NULL;
#endif
}

MEMREF_BLOCK_TESTCASES(memref_memslab, memslab_alloc, memslab_alloc_cb, 10,
		       __alignof__(z_max_align_t));
