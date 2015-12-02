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
 * @brief Return memory pool block
 *
 * This routine returns a block to the memory pool it was allocated from.
 *
 * @param b Pointer to block descriptor.
 *
 * @return N/A
 */
extern void task_mem_pool_free(struct k_block *b);

/**
 * @brief Defragment memory pool
 *
 * This routine concatenates unused blocks that can be merged in memory pool
 * @a p.
 *
 * Doing a complete defragmentation of a memory pool before allocating a set
 * of blocks may be more efficient than having the pool do an implicit
 * partial defragmentation each time a block is allocated.
 *
 * @param p Memory pool name.
 *
 * @return N/A
 */
extern void task_mem_pool_defragment(kmemory_pool_t p);


/**
 * @brief Allocate memory pool block
 *
 * This routine allocates a block of at least @a reqsize bytes from memory pool
 * @a pool_id, and saves its information in block descriptor @a blockptr. If no
 * such block is available the routine either waits until one can be allocated,
 * or until the specified time limit is reached.
 *
 * @param blockptr Pointer to block descriptor.
 * @param pool_id Memory pool name.
 * @param reqsize Requested block size, in bytes.
 * @param timeout Affects the action taken should the memory pool be exhausted.
 * If TICKS_NONE, then return immediately. If TICKS_UNLIMITED, then wait as
 * long as necessary. Otherwise wait up to the specified number of ticks before
 * timing out.
 *
 * @retval RC_OK Successfully allocated memory block
 * @retval RC_TIME Timed out while waiting for memory block
 * @retval RC_FAIL Failed to immediately allocate memory block when
 * @a timeout = TICKS_NONE
 */
extern int task_mem_pool_alloc(struct k_block *blockptr, kmemory_pool_t pool_id,
							int reqsize, int32_t timeout);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* _MEMORY_POOL_H */
