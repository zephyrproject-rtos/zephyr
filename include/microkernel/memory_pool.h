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
 * @cond internal
 */

extern int _task_mem_pool_alloc(struct k_block *b, kmemory_pool_t p,
								int size, int32_t ticks);

/**
 * @endcond
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
 * @brief Allocate memory pool block or fail
 *
 * This routine allocates a block of at least @a s bytes from memory pool
 * @a p, and saves its information in block descriptor @a b. If no such
 * block is available the routine immediately returns a failure indication.
 *
 * @param b Pointer to block descriptor.
 * @param p Memory pool name.
 * @param s Requested block size, in bytes.
 *
 * @return RC_OK on success, or RC_FAIL on failure.
 */
#define task_mem_pool_alloc(b, p, s) \
	_task_mem_pool_alloc(b, p, s, TICKS_NONE)

/**
 * @brief Allocate memory pool block or wait
 *
 * This routine allocates a block of at least @a s bytes from memory pool
 * @a p, and saves its information in block descriptor @a b. If no such block
 * is available the routine waits until one can be allocated.
 *
 * @param b Pointer to block descriptor.
 * @param p Memory pool name.
 * @param s Requested block size, in bytes.
 *
 * @return RC_OK.
 */
#define task_mem_pool_alloc_wait(b, p, s) \
	_task_mem_pool_alloc(b, p, s, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS
/**
 * @brief Allocate memory pool block or timeout
 *
 * This routine allocates a block of at least @a s bytes from memory pool
 * @a p, and saves its information in block descriptor @a b. If no such block
 * is available the routine waits until one can be allocated, or until
 * the specified time limit is reached.
 *
 * @param b Pointer to block descriptor.
 * @param p Memory pool name.
 * @param s Requested block size, in bytes.
 * @param t Maximum number of ticks to wait.
 *
 * @return RC_OK on success, or RC_TIME on timeout.
 */
#define task_mem_pool_alloc_wait_timeout(b, p, s, t) \
	_task_mem_pool_alloc(b, p, s, t)
#endif

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* _MEMORY_POOL_H */
