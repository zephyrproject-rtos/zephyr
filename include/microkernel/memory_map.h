/*
 * Copyright (c) 1997-2012, 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* @file
 * @brief Memory map kernel services.
 */

/**
 * @brief Microkernel Memory Maps
 * @defgroup microkernel_memorymap Microkernel Memory Maps
 * @ingroup microkernel_services
 * @{
 */

#ifndef _MEMORY_MAP_H
#define _MEMORY_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sections.h>

/**
 * @cond internal
 */
extern void _task_mem_map_free(kmemory_map_t mmap, void **mptr);

/**
 * @brief Initialize a memory map struct
 *
 * @param blocks Number of blocks.
 * @param block_size Block Size (in bytes).
 */
#define __K_MEM_MAP_INITIALIZER(blocks, block_size, buffer) \
	{ \
	  .Nelms = blocks, \
	  .element_size = block_size, \
	  .base = buffer, \
	}

/**
 * @endcond
 */

/**
 * @brief Read the number of used blocks in a memory map
 *
 * This routine returns the number of blocks in use for the memory map.
 *
 * @param map Memory map name.
 *
 * @return Number of used blocks.
 */
extern int task_mem_map_used_get(kmemory_map_t map);

/**
 * @brief Return memory map block
 *
 * This routine returns a block to the specified memory map.
 *
 * @param m Memory map name.
 * @param p Memory block address.
 *
 * @return N/A
 */
#define task_mem_map_free(m, p) _task_mem_map_free(m, p)

/**
 * @brief Allocate memory map block
 *
 * This routine allocates a block from memory map @a mmap, and saves the
 * block's address in the area indicated by @a mptr. If no block is available
 * the routine waits until either one can be allocated, or until the specified
 * time limit is reached.
 *
 * @param mmap Memory map name.
 * @param mptr Pointer to memory block address area.
 * @param timeout Affects the action taken should the memory map be exhausted.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as
 * long as necessary. Otherwise wait up to the specified number of ticks before
 * timing out.
 *
 * @retval RC_OK Successfully allocated memory block.
 * @retval RC_TIME Timed out while waiting for memory block.
 * @retval RC_FAIL Failed to immediately allocate memory block when
 * @a timeout = TICKS_NONE
 */
extern int task_mem_map_alloc(kmemory_map_t mmap, void **mptr, int32_t timeout);

/**
 * @brief Define a private microkernel memory map.
 *
 * @param name       Memory map name.
 * @param blocks     Number of blocks.
 * @param block_size Size of each block, in bytes.
 */
#define DEFINE_MEM_MAP(name, blocks, block_size) \
	char __noinit __mem_map_buffer_##name[(blocks * block_size)]; \
	struct _k_mem_map_struct _k_mem_map_obj_##name = \
		__K_MEM_MAP_INITIALIZER(blocks, block_size, \
		__mem_map_buffer_##name); \
	const kmemory_map_t name \
		__in_section(_k_mem_map_ptr, private, mem_map) = \
		(kmemory_map_t)&_k_mem_map_obj_##name;

#ifdef __cplusplus
}
#endif

#endif /* _MEMORY_MAP_H */
/**
 * @}
 */
