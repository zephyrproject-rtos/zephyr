/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LLEXT_SYS_MEM_BLOCKS_HEAP_H
#define ZEPHYR_LLEXT_SYS_MEM_BLOCKS_HEAP_H

#include <zephyr/llext/heap.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief LLEXT sys_mem_blocks heap implementation
 *
 * @addtogroup llext_heap_apis
 * @{
 */

/**
 * @brief Implementation of @ref llext_heap that uses a sys_mem_blocks heap and a k_heap
 *
 * This implementation uses a sys_mem_blocks heap for allocating extension regions
 * and a k_heap for allocating metadata.
 */
struct llext_sys_mem_blocks_heap {
	/** Heap for llext subsystem */
	struct llext_heap heap;
};

/** @cond ignore */
int llext_sys_mem_blocks_heap_init_harvard(struct llext_heap *h, void *instr_mem,
					   size_t instr_bytes, void *data_mem, size_t data_bytes);
int llext_sys_mem_blocks_heap_init(struct llext_heap *h, void *mem, size_t bytes);
int llext_sys_mem_blocks_heap_uninit(struct llext_heap *h);
bool llext_sys_mem_blocks_heap_is_inited(struct llext_heap *h);
void llext_sys_mem_blocks_heap_reset(struct llext_heap *h, struct llext *ext);
void *llext_sys_mem_blocks_heap_alloc_metadata(struct llext_heap *h, size_t bytes);
void *llext_sys_mem_blocks_heap_aligned_alloc_data(struct llext_heap *h, struct llext *ext,
						   size_t align, size_t bytes);
void *llext_sys_mem_blocks_heap_aligned_alloc_instr(struct llext_heap *h, struct llext *ext,
						    size_t align, size_t bytes);
void llext_sys_mem_blocks_heap_free_metadata(struct llext_heap *h, void *ptr);
void llext_sys_mem_blocks_heap_free_data(struct llext_heap *h, struct llext *ext, void *ptr);
void llext_sys_mem_blocks_heap_free_instr(struct llext_heap *h, struct llext *ext, void *ptr);

#define Z_LLEXT_SYS_MEM_BLOCKS_HEAP()                                                              \
	{                                                                                          \
		.heap =                                                                            \
			{                                                                          \
				.init_harvard = llext_sys_mem_blocks_heap_init_harvard,            \
				.init = llext_sys_mem_blocks_heap_init,                            \
				.uninit = llext_sys_mem_blocks_heap_uninit,                        \
				.is_inited = llext_sys_mem_blocks_heap_is_inited,                  \
				.reset = llext_sys_mem_blocks_heap_reset,                          \
				.alloc_metadata = llext_sys_mem_blocks_heap_alloc_metadata,        \
				.aligned_alloc_data =                                              \
					llext_sys_mem_blocks_heap_aligned_alloc_data,              \
				.aligned_alloc_instr =                                             \
					llext_sys_mem_blocks_heap_aligned_alloc_instr,             \
				.free_metadata = llext_sys_mem_blocks_heap_free_metadata,          \
				.free_data = llext_sys_mem_blocks_heap_free_data,                  \
				.free_instr = llext_sys_mem_blocks_heap_free_instr,                \
			},                                                                         \
	}

extern struct llext_sys_mem_blocks_heap sys_mem_blocks_heap;
/** @endcond */

/**
 * @brief llext heap global instance
 *
 * This single global instance provides access to the sys_mem_blocks-based llext heap
 * used by the llext subsystem.
 */
extern struct llext_heap *llext_heap_inst;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_LLEXT_SYS_MEM_BLOCKS_HEAP_H */
