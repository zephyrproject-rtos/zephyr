/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shim basic sl_* pool functions to Zephyr k_mem_slab functions,
 * which again will get redirected to the Zephyr sys_heap.
 */

#include <stdlib.h>

#include <zephyr/kernel.h>

#include "sl_memory_manager.h"
#include "sl_status.h"

sl_status_t sl_memory_create_pool(size_t block_size, uint32_t block_count,
				  sl_memory_pool_t *pool_handle)
{
	struct k_mem_slab *slab;
	char *slab_buffer;
	int ret;

	slab = malloc(sizeof(struct k_mem_slab));
	if (slab == NULL) {
		return SL_STATUS_NO_MORE_RESOURCE;
	}

	slab_buffer = malloc(block_size * block_count);
	if (slab_buffer == NULL) {
		free(slab);
		return SL_STATUS_NO_MORE_RESOURCE;
	}

	ret = k_mem_slab_init(slab, slab_buffer, block_size, block_count);

	if (ret == -EINVAL) {
		free(slab);
		free(slab_buffer);
		return SL_STATUS_INVALID_PARAMETER;
	} else if (ret != 0) {
		free(slab);
		free(slab_buffer);
		return SL_STATUS_FAIL;
	}
	pool_handle->block_address = slab;
	return SL_STATUS_OK;
}

sl_status_t sl_memory_delete_pool(sl_memory_pool_t *pool_handle)
{
	struct k_mem_slab *slab;

	slab = pool_handle->block_address;
	free(slab->buffer);
	free(slab);
	return SL_STATUS_OK;
}

sl_status_t sl_memory_pool_alloc(sl_memory_pool_t *pool_handle, void **block)
{
	struct k_mem_slab *slab;
	int ret;

	slab = pool_handle->block_address;
	ret = k_mem_slab_alloc(slab, block, K_NO_WAIT);

	switch (ret) {
	case -ENOMEM:
		return SL_STATUS_NO_MORE_RESOURCE;
	case -EAGAIN:
		return SL_STATUS_WOULD_BLOCK;
	case -EINVAL:
		return SL_STATUS_INVALID_PARAMETER;
	case 0:
		return SL_STATUS_OK;
	default:
		return SL_STATUS_FAIL;
	}
}

sl_status_t sl_memory_pool_free(sl_memory_pool_t *pool_handle, void *block)
{
	struct k_mem_slab *slab;

	slab = pool_handle->block_address;
	k_mem_slab_free(slab, block);
	return SL_STATUS_OK;
}
