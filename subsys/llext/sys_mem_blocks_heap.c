/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/llext/llext.h>
#include <zephyr/llext/sys_mem_blocks_heap.h>

#include "llext_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(llext, CONFIG_LLEXT_LOG_LEVEL);

#ifdef CONFIG_HARVARD
BUILD_ASSERT(CONFIG_LLEXT_INSTR_HEAP_SIZE * KB(1) % CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE == 0,
	     "CONFIG_LLEXT_INSTR_HEAP_SIZE must be multiple of "
	     "CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE");
BUILD_ASSERT(CONFIG_LLEXT_DATA_HEAP_SIZE * KB(1) % CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE == 0,
	     "CONFIG_LLEXT_DATA_HEAP_SIZE must be multiple of "
	     "CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE");
BUILD_ASSERT(CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE % LLEXT_PAGE_SIZE == 0,
	     "CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE must be multiple of LLEXT_PAGE_SIZE");
uint8_t llext_instr_heap_buf[CONFIG_LLEXT_INSTR_HEAP_SIZE * KB(1)]
	__aligned(CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE)
	__attribute__((section(CONFIG_LLEXT_INSTR_HEAP_SECTION)));
uint8_t llext_data_heap_buf[CONFIG_LLEXT_DATA_HEAP_SIZE * KB(1)]
	__aligned(CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE)
	__attribute__((section(CONFIG_LLEXT_DATA_HEAP_SECTION)));
SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(llext_instr_heap, CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE,
				   CONFIG_LLEXT_INSTR_HEAP_SIZE * KB(1) /
					   CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE,
				   llext_instr_heap_buf);
SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(llext_data_heap, CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE,
				   CONFIG_LLEXT_DATA_HEAP_SIZE * KB(1) /
					   CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE,
				   llext_data_heap_buf);
K_HEAP_DEFINE(llext_metadata_heap, CONFIG_LLEXT_METADATA_HEAP_SIZE * KB(1));
#else
BUILD_ASSERT(CONFIG_LLEXT_EXT_HEAP_SIZE * KB(1) % CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE == 0,
	     "CONFIG_LLEXT_EXT_HEAP_SIZE must be multiple of "
	     "CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE");
BUILD_ASSERT(CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE % LLEXT_PAGE_SIZE == 0,
	     "CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE must be multiple of LLEXT_PAGE_SIZE");
SYS_MEM_BLOCKS_DEFINE(llext_ext_heap, CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE,
		      CONFIG_LLEXT_EXT_HEAP_SIZE * KB(1) /
			      CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE,
		      CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE);
K_HEAP_DEFINE(llext_metadata_heap, CONFIG_LLEXT_METADATA_HEAP_SIZE * KB(1));
#define llext_instr_heap llext_ext_heap
#define llext_data_heap  llext_ext_heap
#endif

/* sys_mem_blocks is statically allocated, and doesn't support dynamic heap */
int llext_sys_mem_blocks_heap_init_harvard(struct llext_heap *h, void *instr_mem,
					   size_t instr_bytes, void *data_mem, size_t data_bytes)
{
	return -ENOSYS;
}

int llext_sys_mem_blocks_heap_init(struct llext_heap *h, void *mem, size_t bytes)
{
	return -ENOSYS;
}

int llext_sys_mem_blocks_heap_uninit(struct llext_heap *h)
{
	return -ENOSYS;
}

bool llext_sys_mem_blocks_heap_is_inited(struct llext_heap *h)
{
	return true;
}

void llext_sys_mem_blocks_heap_reset(struct llext_heap *h, struct llext *ext)
{
	ext->mem_alloc_map.idx = 0;
}

void *llext_sys_mem_blocks_heap_alloc_metadata(struct llext_heap *h, size_t bytes)
{
	return k_heap_alloc(&llext_metadata_heap, bytes, K_NO_WAIT);
}

void *llext_sys_mem_blocks_heap_aligned_alloc_data(struct llext_heap *h, struct llext *ext,
						   size_t align, size_t bytes)
{
	if (ext->mem_alloc_map.idx >= LLEXT_MEM_COUNT) {
		return NULL;
	}

	if (CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE % align != 0) {
		LOG_ERR("Requested alignment %zu not possible with block alignment %d", align,
			CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE);
		return NULL;
	}

	int num = bytes / CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE +
		  ((bytes % CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE) ? 1 : 0);
	void **blocks = k_heap_alloc(&llext_metadata_heap, sizeof(void *) * num, K_NO_WAIT);

	if (blocks == NULL) {
		return NULL;
	}

	int ret = sys_mem_blocks_alloc_contiguous(&llext_data_heap, num, blocks);

	if (ret != 0) {
		return NULL;
	}

	struct llext_alloc *mem_alloc = &ext->mem_alloc_map.map[ext->mem_alloc_map.idx];

	mem_alloc->sys_mem_blocks_ptr = blocks;
	mem_alloc->alloc_ptr = blocks[0];
	mem_alloc->num_blocks = num;
	ext->mem_alloc_map.idx++;

	return mem_alloc->alloc_ptr;
}

void *llext_sys_mem_blocks_heap_aligned_alloc_instr(struct llext_heap *h, struct llext *ext,
						    size_t align, size_t bytes)
{
	if (ext->mem_alloc_map.idx >= LLEXT_MEM_COUNT) {
		return NULL;
	}

	if (CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE % align != 0) {
		LOG_ERR("Requested alignment %zu not possible with block size %d", align,
			CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE);
		return NULL;
	}

	int num = bytes / CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE +
		  ((bytes % CONFIG_LLEXT_HEAP_SYS_MEM_BLOCK_BLOCK_SIZE) ? 1 : 0);
	void **blocks = k_heap_alloc(&llext_metadata_heap, sizeof(void *) * num, K_NO_WAIT);

	if (blocks == NULL) {
		return NULL;
	}

	int ret = sys_mem_blocks_alloc_contiguous(&llext_instr_heap, num, blocks);

	if (ret != 0) {
		return NULL;
	}

	struct llext_alloc *mem_alloc = &ext->mem_alloc_map.map[ext->mem_alloc_map.idx];

	mem_alloc->sys_mem_blocks_ptr = blocks;
	mem_alloc->alloc_ptr = blocks[0];
	mem_alloc->num_blocks = num;
	ext->mem_alloc_map.idx++;

	return mem_alloc->alloc_ptr;
}

static inline struct llext_alloc *get_llext_alloc(struct llext_alloc_map *map, void *alloc_ptr)
{
	for (int i = 0; i < map->idx; i++) {
		if (map->map[i].alloc_ptr == alloc_ptr) {
			return &map->map[i];
		}
	}

	return NULL;
}

void llext_sys_mem_blocks_heap_free_metadata(struct llext_heap *h, void *ptr)
{
	k_heap_free(&llext_metadata_heap, ptr);
}

void llext_sys_mem_blocks_heap_free_data(struct llext_heap *h, struct llext *ext, void *ptr)
{
	struct llext_alloc *mem_alloc = get_llext_alloc(&ext->mem_alloc_map, ptr);

	if (!mem_alloc) {
		LOG_ERR("Could not find sys_mem_blocks alloc to free pointer %p", ptr);
		return;
	}

	sys_mem_blocks_free_contiguous(&llext_data_heap, mem_alloc->sys_mem_blocks_ptr[0],
				       mem_alloc->num_blocks);
	k_heap_free(&llext_metadata_heap, mem_alloc->sys_mem_blocks_ptr);
	mem_alloc->alloc_ptr = NULL;
	mem_alloc->num_blocks = 0;
	mem_alloc->sys_mem_blocks_ptr = NULL;
}

void llext_sys_mem_blocks_heap_free_instr(struct llext_heap *h, struct llext *ext, void *ptr)
{
	struct llext_alloc *mem_alloc = get_llext_alloc(&ext->mem_alloc_map, ptr);

	if (!mem_alloc) {
		LOG_ERR("Could not find sys_mem_blocks alloc to free pointer %p", ptr);
		return;
	}

	sys_mem_blocks_free_contiguous(&llext_instr_heap, mem_alloc->sys_mem_blocks_ptr[0],
				       mem_alloc->num_blocks);
	k_heap_free(&llext_metadata_heap, mem_alloc->sys_mem_blocks_ptr);
	mem_alloc->alloc_ptr = NULL;
	mem_alloc->num_blocks = 0;
	mem_alloc->sys_mem_blocks_ptr = NULL;
}

struct llext_sys_mem_blocks_heap sys_mem_blocks_heap = Z_LLEXT_SYS_MEM_BLOCKS_HEAP();
struct llext_heap *llext_heap_inst = &sys_mem_blocks_heap.heap;
