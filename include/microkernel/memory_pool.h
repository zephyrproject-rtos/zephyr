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

/**
 * @file
 * @brief Memory Pools
 */

#ifndef _MEMORY_POOL_H
#define _MEMORY_POOL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Memory Pools
 * @defgroup microkernel_memorypool Microkernel Memory Pools
 * @ingroup microkernel_services
 * @{
 */

/**
 * @brief Return memory pool block.
 *
 * This routine returns a block to the memory pool from which it was allocated.
 *
 * @param b Pointer to block descriptor.
 *
 * @return N/A
 */
extern void task_mem_pool_free(struct k_block *b);

/**
 * @brief Defragment memory pool.
 *
 * This routine concatenates unused blocks that can be merged in memory pool
 * @a p.
 *
 * Doing a full defragmentation of a memory pool before allocating a set
 * of blocks may be more efficient than having the pool do an implicit
 * partial defragmentation each time a block is allocated.
 *
 * @param p Memory pool name.
 *
 * @return N/A
 */
extern void task_mem_pool_defragment(kmemory_pool_t p);


/**
 * @brief Allocate memory pool block.
 *
 * This routine allocates a block of at least @a reqsize bytes from memory pool
 * @a pool_id, and saves its information in block descriptor @a blockptr. When no
 * such block is available, the routine waits either until one can be allocated,
 * or until the specified time limit is reached.
 *
 * @param blockptr Pointer to block descriptor.
 * @param pool_id Memory pool name.
 * @param reqsize Requested block size, in bytes.
 * @param timeout Determines the action to take when the memory pool is exhausted.
 *   For TICKS_NONE, return immediately.
 *   For TICKS_UNLIMITED, wait as long as necessary.
 *   Otherwise, wait up to the specified number of ticks before timing out.
 *
 * @retval RC_OK Successfully allocated memory block
 * @retval RC_TIME Timed out while waiting for memory block
 * @retval RC_FAIL Failed to immediately allocate memory block when
 * @a timeout = TICKS_NONE
 * @sa TICKS_NONE, TICKS_UNLIMITED
 */
extern int task_mem_pool_alloc(struct k_block *blockptr, kmemory_pool_t pool_id,
							int reqsize, int32_t timeout);

/**
 * @brief Allocate memory
 *
 * This routine  provides traditional malloc semantics and is a wrapper on top
 * of microkernel pool alloc API.
 * It returns an aligned memory address which points to the start of a memory
 * block of at least \p size bytes.
 * This memory comes from heap memory pool, consequently the app should
 * specify its intention to use a heap pool via the HEAP_SIZE keyword in
 * MDEF file, if it uses this API.
 * When not enough free memory is available in the heap pool, it returns NULL
 *
 * @param size Size of memory requested by the caller.
 *
 * @retval address of the block if successful otherwise returns NULL
 */
extern void *task_malloc(uint32_t size);

/**
 * @brief Free memory allocated through task_malloc
 *
 * This routine  provides traditional free semantics and is intended to free
 * memory allocated using task_malloc API.
 *
 * @param ptr pointer to be freed
 *
 * @return NA
 */
extern void task_free(void *ptr);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* _MEMORY_POOL_H */
