/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LLEXT_MEMBLK_H_
#define ZEPHYR_SUBSYS_LLEXT_MEMBLK_H_

#include <zephyr/llext/llext.h>
#include <zephyr/sys/mem_blocks.h>

#ifdef CONFIG_HARVARD
extern sys_mem_blocks_t llext_instr_heap;
extern sys_mem_blocks_t llext_data_heap;
extern struct k_heap llext_metadata_heap;
#else
extern sys_mem_blocks_t llext_ext_heap;
extern struct k_heap llext_metadata_heap;
#define llext_instr_heap llext_ext_heap
#define llext_data_heap  llext_ext_heap
#endif

void *llext_memblk_aligned_alloc_data_instr(struct llext *ext, sys_mem_blocks_t *memblk_heap,
					    size_t align, size_t bytes);

void llext_memblk_free_data_instr(struct llext *ext, sys_mem_blocks_t *memblk_heap, void *ptr);

static inline bool llext_heap_is_inited(void)
{
	return true;
}

static inline void *llext_alloc_metadata(size_t bytes)
{
	if (bytes != 0 && llext_heap_is_inited()) {
		return k_heap_alloc(&llext_metadata_heap, bytes, K_NO_WAIT);
	}

	return NULL;
}

static inline void *llext_aligned_alloc_data(struct llext *ext, size_t align, size_t bytes)
{
	if (bytes != 0 && llext_heap_is_inited()) {
		return llext_memblk_aligned_alloc_data_instr(ext, &llext_data_heap, align, bytes);
	}

	return NULL;
}

static inline void *llext_aligned_alloc_instr(struct llext *ext, size_t align, size_t bytes)
{
	if (bytes != 0 && llext_heap_is_inited()) {
		return llext_memblk_aligned_alloc_data_instr(ext, &llext_instr_heap, align, bytes);
	}

	return NULL;
}

static inline void llext_free_metadata(void *ptr)
{
	if (llext_heap_is_inited()) {
		k_heap_free(&llext_metadata_heap, ptr);
	}
}

static inline void llext_free_data(struct llext *ext, void *ptr)
{
	if (llext_heap_is_inited()) {
		llext_memblk_free_data_instr(ext, &llext_data_heap, ptr);
	}
}

static inline void llext_free_instr(struct llext *ext, void *ptr)
{
	if (llext_heap_is_inited()) {
		llext_memblk_free_data_instr(ext, &llext_instr_heap, ptr);
	}
}

static inline void llext_heap_reset(struct llext *ext)
{
	ext->mem_alloc_map.idx = 0;
}

#endif /* ZEPHYR_SUBSYS_LLEXT_MEMBLK_H_ */
