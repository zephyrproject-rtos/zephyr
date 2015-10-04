/*
 * Copyright (c) 1997-2012, 2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
