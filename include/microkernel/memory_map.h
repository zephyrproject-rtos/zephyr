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

/* @file
 * @brief Memory map kernel services.
 */

#ifndef _MEMORY_MAP_H
#define _MEMORY_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

extern int _task_mem_map_alloc(kmemory_map_t mmap, void **mptr, int32_t time);
extern void _task_mem_map_free(kmemory_map_t mmap, void **mptr);

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

#ifdef __cplusplus
}
#endif

#endif /* _MEMORY_MAP_H */
