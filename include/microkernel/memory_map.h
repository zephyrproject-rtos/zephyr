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
 * @defgroup microkernel_memorymap Microkernel Memomry Maps
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
extern int _task_mem_map_alloc(kmemory_map_t mmap, void **mptr, int32_t time);
extern void _task_mem_map_free(kmemory_map_t mmap, void **mptr);

/**
 * @brief Initialize a memory map struct.
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
 * @param map Memory map.
 *
 * @return number of used blocks
 */
extern int task_mem_map_used_get(kmemory_map_t map);

/**
 * @brief Return memory map block request
 *
 * This routine returns a block to the specified memory map. If a higher
 * priority task is waiting for a block from the same map a task switch
 * takes place.
 *
 * @param m Memory map.
 * @param p Block of memory to return.
 *
 * @return N/A
 */
#define task_mem_map_free(m, p) _task_mem_map_free(m, p)

/**
 * @brief Allocate memory map block request
 *
 * This routine is used to request a block of memory from the memory map.
 *
 * @param m Memory map from which to request block.
 * @param p Pointer to requested block of memory.
 *
 * @return RC_OK on success or RC_FAIL on error.
 */
#define task_mem_map_alloc(m, p) _task_mem_map_alloc(m, p, TICKS_NONE)

/**
 * @brief Allocate memory map block request
 *
 * This routine is used to request a block of memory from the memory map.
 * The task will wait if the memory block is not available.
 *
 * @param m Memory map from which to request block.
 * @param p Pointer to requested block of memory.
 *
 * @return RC_OK on success or RC_FAIL on error.
 */
#define task_mem_map_alloc_wait(m, p) _task_mem_map_alloc(m, p, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS

/**
 * @brief Allocate memory map block request with timeout
 *
 * This routine is used to request a block of memory from the memory map.
 * The task will wait if the memory block is not available until the timeout
 * expire or the memory is available.
 *
 * @param m Memory map from which to request block.
 * @param p Pointer to requested block of memory.
 * @param t Maximum number of ticks for which to wait.
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, error, timeout respectively
 */
#define task_mem_map_alloc_wait_timeout(m, p, t) _task_mem_map_alloc(m, p, t)
#endif

/**
 * @brief Define a private microkernel memory map.
 *
 * @param name Name of the memory map.
 * @param blocks Number of blocks.
 * @param block_size Size of each blocks (in bytes).
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
