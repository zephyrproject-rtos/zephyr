/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "mcp_common.h"

LOG_MODULE_REGISTER(mcp_common, CONFIG_MCP_LOG_LEVEL);

#if defined(CONFIG_MCP_ALLOC_SLAB)

#define MCP_SLAB_SMALL_SIZE  CONFIG_MCP_SLAB_SMALL_SIZE
#define MCP_SLAB_MEDIUM_SIZE CONFIG_MCP_SLAB_MEDIUM_SIZE
#define MCP_SLAB_LARGE_SIZE  CONFIG_MCP_MAX_MESSAGE_SIZE

#define MCP_SLAB_SMALL_COUNT  CONFIG_MCP_SLAB_SMALL_COUNT
#define MCP_SLAB_MEDIUM_COUNT CONFIG_MCP_SLAB_MEDIUM_COUNT
#define MCP_SLAB_LARGE_COUNT  CONFIG_MCP_SLAB_LARGE_COUNT

K_MEM_SLAB_DEFINE_STATIC(mcp_slab_small, MCP_SLAB_SMALL_SIZE, MCP_SLAB_SMALL_COUNT, 4);
K_MEM_SLAB_DEFINE_STATIC(mcp_slab_medium, MCP_SLAB_MEDIUM_SIZE, MCP_SLAB_MEDIUM_COUNT, 4);
K_MEM_SLAB_DEFINE_STATIC(mcp_slab_large, MCP_SLAB_LARGE_SIZE, MCP_SLAB_LARGE_COUNT, 4);

static bool ptr_in_slab(void *ptr, struct k_mem_slab *slab)
{
	char *p = (char *)ptr;
	char *start = slab->buffer;
	char *end = start + (slab->info.block_size * slab->info.num_blocks);

	return (p >= start) && (p < end);
}

__weak void *mcp_alloc(size_t size)
{
	void *ptr = NULL;
	int ret;

	if (size == 0) {
		return NULL;
	}

	if (size <= MCP_SLAB_SMALL_SIZE) {
		ret = k_mem_slab_alloc(&mcp_slab_small, &ptr, K_NO_WAIT);
		if (ret == 0) {
			return ptr;
		}
		LOG_DBG("Small slab full, trying medium");
	}

	if (size <= MCP_SLAB_MEDIUM_SIZE) {
		ret = k_mem_slab_alloc(&mcp_slab_medium, &ptr, K_NO_WAIT);
		if (ret == 0) {
			return ptr;
		}
		LOG_DBG("Medium slab full, trying large");
	}

	if (size <= MCP_SLAB_LARGE_SIZE) {
		ret = k_mem_slab_alloc(&mcp_slab_large, &ptr, K_NO_WAIT);
		if (ret == 0) {
			return ptr;
		}
		LOG_WRN("Large slab full, allocation failed for size %zu", size);
	} else {
		LOG_ERR("Requested size %zu exceeds maximum slab size %d", size,
			MCP_SLAB_LARGE_SIZE);
	}

	return NULL;
}

__weak void mcp_free(void *ptr)
{
	if (ptr == NULL) {
		return;
	}

	if (ptr_in_slab(ptr, &mcp_slab_small)) {
		k_mem_slab_free(&mcp_slab_small, ptr);
	} else if (ptr_in_slab(ptr, &mcp_slab_medium)) {
		k_mem_slab_free(&mcp_slab_medium, ptr);
	} else if (ptr_in_slab(ptr, &mcp_slab_large)) {
		k_mem_slab_free(&mcp_slab_large, ptr);
	} else {
		LOG_ERR("Attempted to free pointer %p not belonging to any MCP slab", ptr);
	}
}

#else /* CONFIG_MCP_ALLOC_HEAP */

__weak void *mcp_alloc(size_t size)
{
	return k_malloc(size);
}

__weak void mcp_free(void *ptr)
{
	k_free(ptr);
}

#endif /* CONFIG_MCP_ALLOC_SLAB */

void mcp_safe_strcpy(char *dst, size_t dst_sz, const char *src)
{
	if (dst == NULL || dst_sz == 0) {
		return;
	}

	if (src == NULL) {
		dst[0] = '\0';
		return;
	}

	(void)snprintk(dst, dst_sz, "%s", src);
}
