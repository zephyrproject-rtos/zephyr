/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/llext/llext.h>
#include <zephyr/sys/mem_blocks.h>

#include "llext_mem.h"
#include "llext_memblk.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(llext, CONFIG_LLEXT_LOG_LEVEL);

#ifdef CONFIG_HARVARD
BUILD_ASSERT(CONFIG_LLEXT_INSTR_HEAP_SIZE * KB(1) % CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE == 0,
	     "CONFIG_LLEXT_INSTR_HEAP_SIZE must be multiple of "
	     "CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE");
BUILD_ASSERT(CONFIG_LLEXT_DATA_HEAP_SIZE * KB(1) % CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE == 0,
	     "CONFIG_LLEXT_DATA_HEAP_SIZE must be multiple of "
	     "CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE");
BUILD_ASSERT(CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE % LLEXT_PAGE_SIZE == 0,
	     "CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE must be multiple of LLEXT_PAGE_SIZE");
uint8_t llext_instr_heap_buf[CONFIG_LLEXT_INSTR_HEAP_SIZE * KB(1)]
	__aligned(CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE)
	__attribute__((section(".rodata.llext_instr_heap_buf")));
uint8_t llext_data_heap_buf[CONFIG_LLEXT_DATA_HEAP_SIZE * KB(1)]
	__aligned(CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE)
	__attribute__((section(".data.llext_data_heap_buf")));
SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(llext_instr_heap, CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE,
				   CONFIG_LLEXT_INSTR_HEAP_SIZE * KB(1) /
					   CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE,
				   llext_instr_heap_buf);
SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(llext_data_heap, CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE,
				   CONFIG_LLEXT_DATA_HEAP_SIZE * KB(1) /
					   CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE,
				   llext_data_heap_buf);
K_HEAP_DEFINE(llext_metadata_heap, CONFIG_LLEXT_METADATA_HEAP_SIZE * KB(1));
#else  /* !CONFIG_HARVARD */
BUILD_ASSERT(CONFIG_LLEXT_EXT_HEAP_SIZE * KB(1) % CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE == 0,
	     "CONFIG_LLEXT_EXT_HEAP_SIZE must be multiple of "
	     "CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE");
BUILD_ASSERT(CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE % LLEXT_PAGE_SIZE == 0,
	     "CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE must be multiple of LLEXT_PAGE_SIZE");
SYS_MEM_BLOCKS_DEFINE(llext_ext_heap, CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE,
		      CONFIG_LLEXT_EXT_HEAP_SIZE * KB(1) / CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE,
		      CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE);
K_HEAP_DEFINE(llext_metadata_heap, CONFIG_LLEXT_METADATA_HEAP_SIZE * KB(1));
#endif /* CONFIG_HARVARD */

static inline struct llext_alloc *get_llext_alloc(struct llext_alloc_map *map, void *alloc_ptr)
{
	for (int i = 0; i < map->idx; i++) {
		if (map->map[i].memblk_ptr == alloc_ptr) {
			return &map->map[i];
		}
	}
	return NULL;
}

void *llext_memblk_aligned_alloc_data_instr(struct llext *ext, sys_mem_blocks_t *memblk_heap,
					    size_t align, size_t bytes)
{
	if (ext->mem_alloc_map.idx >= LLEXT_MEM_COUNT) {
		return NULL;
	}

	if (CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE % align != 0) {
		LOG_ERR("Requested alignment %zu not possible with block alignment %d", align,
			CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE);
		return NULL;
	}

	struct llext_alloc *mem_alloc = &ext->mem_alloc_map.map[ext->mem_alloc_map.idx];

	mem_alloc->num_blocks = DIV_ROUND_UP(bytes, CONFIG_LLEXT_HEAP_MEMBLK_BLOCK_SIZE);
	int ret = sys_mem_blocks_alloc_contiguous(memblk_heap, mem_alloc->num_blocks,
						  &mem_alloc->memblk_ptr);

	if (ret != 0) {
		return NULL;
	}

	ext->mem_alloc_map.idx++;

	return mem_alloc->memblk_ptr;
}

void llext_memblk_free_data_instr(struct llext *ext, sys_mem_blocks_t *memblk_heap, void *ptr)
{
	struct llext_alloc *mem_alloc = get_llext_alloc(&ext->mem_alloc_map, ptr);

	if (!mem_alloc) {
		LOG_ERR("Could not find sys_mem_blocks alloc to free pointer %p", ptr);
		return;
	}

	sys_mem_blocks_free_contiguous(memblk_heap, mem_alloc->memblk_ptr, mem_alloc->num_blocks);
	mem_alloc->num_blocks = 0;
	mem_alloc->memblk_ptr = NULL;
}
