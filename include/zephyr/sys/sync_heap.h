/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_SYNC_HEAP_H_
#define ZEPHYR_INCLUDE_SYS_SYNC_HEAP_H_

#include <zephyr/sys/sys_heap.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup sync_user_heap_allocator Synchronized User Heap Allocator
 * @ingroup heaps
 * @{
 */

 /**
  * @brief Synchronized heap struct
  */
struct sys_sync_heap {
	/** @cond INTERNAL_HIDDEN */
	struct sys_heap heap;
	/* Replace this with sys_mutex once this works purely in user
	 * and can be created dynamically
	 * See https://github.com/zephyrproject-rtos/zephyr/issues/15138
	 */
	struct k_mutex *lock;
	/** @endcond */
};

/** @brief Initialize sys_sync_heap
 *
 * See @ref sys_heap_init.
 *
 * @param heap Heap to initialize
 * @param mem Untyped pointer to unused memory
 * @param bytes Size of region pointed to by @a mem
 * @return 0 if successful
 */
int sys_sync_heap_init(struct sys_sync_heap *heap, void *mem, size_t bytes);

/** @brief Allocate memory from a heap
 *
 * See @ref sys_heap_alloc.
 *
 * @param heap Heap from which to allocate
 * @param bytes Number of bytes requested
 * @return Pointer to memory the caller can now use
 */
void *sys_sync_heap_alloc(struct sys_sync_heap *heap, size_t bytes);

/** @brief Allocate aligned memory from a heap
 *
 * See @ref sys_heap_aligned_alloc.
 *
 * @param heap Heap from which to allocate
 * @param align Alignment in bytes, must be a power of two
 * @param bytes Number of bytes requested
 * @return Pointer to memory the caller can now use
 */
void *sys_sync_heap_aligned_alloc(struct sys_sync_heap *heap, size_t align, size_t bytes);

/** @brief Allocate memory from a heap
 *
 * See @ref sys_heap_noalign_alloc.
 *
 * @param heap Heap from which to allocate
 * @param align Ignored placeholder
 * @param bytes Number of bytes requested
 * @return Pointer to memory the caller can now use
 */
void *sys_sync_heap_noalign_alloc(struct sys_sync_heap *heap, size_t align, size_t bytes);

/** @brief Free memory into a heap
 *
 * See @ref sys_heap_free.
 *
 * @param heap Heap to which to return the memory
 * @param mem A pointer previously returned from sys_heap_alloc()
 */
void sys_sync_heap_free(struct sys_sync_heap *heap, void *mem);

/** @brief Expand the size of an existing allocation
 *
 * See @ref sys_heap_realloc.
 *
 * @param heap Heap from which to allocate
 * @param ptr Original pointer returned from a previous allocation
 * @param bytes Number of bytes requested for the new block
 * @return Pointer to memory the caller can now use, or NULL
 */
void *sys_sync_heap_realloc(struct sys_sync_heap *heap, void *ptr, size_t bytes);

/** @brief Expand the size of an existing allocation
 *
 * See @ref sys_heap_aligned_realloc.
 *
 * @param heap Heap from which to allocate
 * @param ptr Original pointer returned from a previous allocation
 * @param align Alignment in bytes, must be a power of two
 * @param bytes Number of bytes requested for the new block
 * @return Pointer to memory the caller can now use, or NULL
 */
void *sys_sync_heap_aligned_realloc(struct sys_sync_heap *heap, void *ptr, size_t align,
				    size_t bytes);

/**
 * @brief Get the runtime statistics of a sys_heap
 *
 * See @ref sys_heap_runtime_stats_get.
 *
 * @param heap Pointer to heap
 * @param stats Pointer to struct to copy statistics into
 * @return -EINVAL if null pointers, otherwise 0
 */
int sys_sync_heap_runtime_stats_get(struct sys_sync_heap *heap, struct sys_memory_stats *stats);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_SYNC_HEAP_H_ */
