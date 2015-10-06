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

extern int _task_mem_pool_alloc(struct k_block *B, kmemory_pool_t pid, int size, int32_t time);


/**
 * @endcond
 */

/**
 * @brief Return memory pool block request
 *
 * This routine returns a block to a memory pool.
 *
 * The struct k_block structure contains the block details, including the pool to
 * which it should be returned.
 *
 * @param bl Pointer to block to free
 *
 * @return N/A
 */
extern void task_mem_pool_free(struct k_block *bl);

/**
 * @brief Defragment memory pool request
 *
 * This routine concatenates unused memory in a memory pool.
 *
 * @param pid Memory pool ID
 *
 * @return N/A
 */
extern void task_mem_pool_defragment(kmemory_pool_t pid);


/**
 * @brief Allocate memory pool block request without waiting
 *
 * This routine allocates a free block from the specified memory pool, ensuring
 * that its size is at least as big as the size requested (in bytes).
 *
 * @param b Pointer to requested block
 * @param pid Pool from which to get block
 * @param s Requested block size
 *
 * @return RC_OK, RC_FAIL on success, failure respectively
 */
#define task_mem_pool_alloc(b, pid, s) _task_mem_pool_alloc(b, pid, s, TICKS_NONE)

/**
 * @brief Allocate memory pool block request and wait
 *
 * This routine allocates a free block from the specified memory pool, ensuring
 * that its size is at least as big as the size requested (in bytes).
 *
 * @param b Pointer to requested block
 * @param pid Pool from which to get block
 * @param s Requested block size
 *
 * @return RC_OK, RC_FAIL on success, failure respectively
 */
#define task_mem_pool_alloc_wait(b, pid, s) _task_mem_pool_alloc(b, pid, s, TICKS_UNLIMITED)

#ifdef CONFIG_SYS_CLOCK_EXISTS
/**
 * @brief Allocate memory pool block request and wait
 *
 * This routine allocates a free block from the specified memory pool, ensuring
 * that its size is at least as big as the size requested (in bytes).
 *
 * @param b Pointer to requested block
 * @param pid Pool from which to get block
 * @param s Requested block size
 * @param t Maximum number of ticks to wait
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
#define task_mem_pool_alloc_wait_timeout(b, pid, s, t) _task_mem_pool_alloc(b, pid, s, t)
#endif

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* _MEMORY_POOL_H */
