/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SYS_SYS_HEAP_H_
#define ZEPHYR_INCLUDE_SYS_SYS_HEAP_H_

#include <stddef.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <zephyr/sys/mem_stats.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Simple, fast heap implementation.
 *
 * A more or less conventional segregated fit allocator with
 * power-of-two buckets.
 *
 * Excellent space efficiency.  Chunks can be split arbitrarily in 8
 * byte units.  Overhead is only four bytes per allocated chunk (eight
 * bytes for heaps >256kb or on 64 bit systems), plus a log2-sized
 * array of 2-word bucket headers.  No coarse alignment restrictions
 * on blocks, they can be split and merged (in units of 8 bytes)
 * arbitrarily.
 *
 * Simple API.  Initialize at runtime with any blob of memory and not
 * a macro-generated, carefully aligned static array.  Allocate and
 * free by user pointer and not an opaque block handle.
 *
 * Good fragmentation resistance.  Freed blocks are always immediately
 * merged with adjacent free blocks.  Allocations are attempted from a
 * sample of the smallest bucket that might fit, falling back rapidly
 * to the smallest block guaranteed to fit.  Split memory remaining in
 * the chunk is always returned immediately to the heap for other
 * allocation.
 *
 * Excellent performance with firmly bounded runtime.  All operations
 * are constant time (though there is a search of the smallest bucket
 * that has a compile-time-configurable upper bound, setting this to
 * extreme values results in an effectively linear search of the
 * list), objectively fast (~hundred instructions) and amenable to
 * locked operation.
 */

/* Note: the init_mem/bytes fields are for the static initializer to
 * have somewhere to put the arguments.  The actual heap metadata at
 * runtime lives in the heap memory itself and this struct simply
 * functions as an opaque pointer.  Would be good to clean this up and
 * put the two values somewhere else, though it would make
 * SYS_HEAP_DEFINE a little hairy to write.
 */
struct sys_heap {
	struct z_heap *heap;
	void *init_mem;
	size_t init_bytes;
};

struct z_heap_stress_result {
	uint32_t total_allocs;
	uint32_t successful_allocs;
	uint32_t total_frees;
	uint64_t accumulated_in_use_bytes;
};

/**
 * @defgroup low_level_heap_allocator Low Level Heap Allocator
 * @ingroup heaps
 * @{
 */

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS

/**
 * @brief Get the runtime statistics of a sys_heap
 *
 * @param heap Pointer to specified sys_heap
 * @param stats_t Pointer to struct to copy statistics into
 * @return -EINVAL if null pointers, otherwise 0
 */
int sys_heap_runtime_stats_get(struct sys_heap *heap,
		struct sys_memory_stats *stats);

/**
 * @brief Reset the maximum heap usage.
 *
 * Set the statistic measuring the maximum number of allocated bytes to the
 * current number of allocated bytes.
 *
 * @param heap Pointer to sys_heap
 * @return -EINVAL if null pointer was passed, otherwise 0
 */
int sys_heap_runtime_stats_reset_max(struct sys_heap *heap);

#endif

/** @brief Initialize sys_heap
 *
 * Initializes a sys_heap struct to manage the specified memory.
 *
 * @param heap Heap to initialize
 * @param mem Untyped pointer to unused memory
 * @param bytes Size of region pointed to by @a mem
 */
void sys_heap_init(struct sys_heap *heap, void *mem, size_t bytes);

/** @brief Allocate memory from a sys_heap
 *
 * Returns a pointer to a block of unused memory in the heap.  This
 * memory will not otherwise be used until it is freed with
 * sys_heap_free().  If no memory can be allocated, NULL will be
 * returned.  The allocated memory is guaranteed to have a starting
 * address which is a multiple of sizeof(void *).  If a bigger alignment
 * is necessary then sys_heap_aligned_alloc() should be used instead.
 *
 * @note The sys_heap implementation is not internally synchronized.
 * No two sys_heap functions should operate on the same heap at the
 * same time.  All locking must be provided by the user.
 *
 * @param heap Heap from which to allocate
 * @param bytes Number of bytes requested
 * @return Pointer to memory the caller can now use
 */
void *sys_heap_alloc(struct sys_heap *heap, size_t bytes);

/** @brief Allocate aligned memory from a sys_heap
 *
 * Behaves in all ways like sys_heap_alloc(), except that the returned
 * memory (if available) will have a starting address in memory which
 * is a multiple of the specified power-of-two alignment value in
 * bytes.  With align=0 this behaves exactly like sys_heap_alloc().
 * The resulting memory can be returned to the heap using sys_heap_free().
 *
 * @param heap Heap from which to allocate
 * @param align Alignment in bytes, must be a power of two
 * @param bytes Number of bytes requested
 * @return Pointer to memory the caller can now use
 */
void *sys_heap_aligned_alloc(struct sys_heap *heap, size_t align, size_t bytes);

/** @brief Free memory into a sys_heap
 *
 * De-allocates a pointer to memory previously returned from
 * sys_heap_alloc such that it can be used for other purposes.  The
 * caller must not use the memory region after entry to this function.
 *
 * @note The sys_heap implementation is not internally synchronized.
 * No two sys_heap functions should operate on the same heap at the
 * same time.  All locking must be provided by the user.
 *
 * @param heap Heap to which to return the memory
 * @param mem A pointer previously returned from sys_heap_alloc()
 */
void sys_heap_free(struct sys_heap *heap, void *mem);

/** @brief Expand the size of an existing allocation
 *
 * Returns a pointer to a new memory region with the same contents,
 * but a different allocated size.  If the new allocation can be
 * expanded in place, the pointer returned will be identical.
 * Otherwise the data will be copies to a new block and the old one
 * will be freed as per sys_heap_free().  If the specified size is
 * smaller than the original, the block will be truncated in place and
 * the remaining memory returned to the heap.  If the allocation of a
 * new block fails, then NULL will be returned and the old block will
 * not be freed or modified.
 *
 * @param heap Heap from which to allocate
 * @param ptr Original pointer returned from a previous allocation
 * @param align Alignment in bytes, must be a power of two
 * @param bytes Number of bytes requested for the new block
 * @return Pointer to memory the caller can now use, or NULL
 */
void *sys_heap_aligned_realloc(struct sys_heap *heap, void *ptr,
			       size_t align, size_t bytes);

#define sys_heap_realloc(heap, ptr, bytes) \
	sys_heap_aligned_realloc(heap, ptr, 0, bytes)

/** @brief Return allocated memory size
 *
 * Returns the size, in bytes, of a block returned from a successful
 * sys_heap_alloc() or sys_heap_alloc_aligned() call.  The value
 * returned is the size of the heap-managed memory, which may be
 * larger than the number of bytes requested due to allocation
 * granularity.  The heap code is guaranteed to make no access to this
 * region of memory until a subsequent sys_heap_free() on the same
 * pointer.
 *
 * @param heap Heap containing the block
 * @param mem Pointer to memory allocated from this heap
 * @return Size in bytes of the memory region
 */
size_t sys_heap_usable_size(struct sys_heap *heap, void *mem);

/** @brief Validate heap integrity
 *
 * Validates the internal integrity of a sys_heap.  Intended for unit
 * test and validation code, though potentially useful as a user API
 * for applications with complicated runtime reliability requirements.
 * Note: this cannot catch every possible error, but if it returns
 * true then the heap is in a consistent state and can correctly
 * handle any sys_heap_alloc() request and free any live pointer
 * returned from a previous allocation.
 *
 * @param heap Heap to validate
 * @return true, if the heap is valid, otherwise false
 */
bool sys_heap_validate(struct sys_heap *heap);

/** @brief sys_heap stress test rig
 *
 * Test rig for heap allocation validation.  This will loop for @a
 * op_count cycles, in each iteration making a random choice to
 * allocate or free a pointer of randomized (power law) size based on
 * heuristics designed to keep the heap in a state where it is near @a
 * target_percent full.  Allocation and free operations are provided
 * by the caller as callbacks (i.e. this can in theory test any heap).
 * Results, including counts of frees and successful/unsuccessful
 * allocations, are returned via the @a result struct.
 *
 * @param alloc_fn Callback to perform an allocation.  Passes back the @a
 *              arg parameter as a context handle.
 * @param free_fn Callback to perform a free of a pointer returned from
 *             @a alloc.  Passes back the @a arg parameter as a
 *             context handle.
 * @param arg Context handle to pass back to the callbacks
 * @param total_bytes Size of the byte array the heap was initialized in
 * @param op_count How many iterations to test
 * @param scratch_mem A pointer to scratch memory to be used by the
 *                    test.  Should be about 1/2 the size of the heap
 *                    for tests that need to stress fragmentation.
 * @param scratch_bytes Size of the memory pointed to by @a scratch_mem
 * @param target_percent Percentage fill value (1-100) to which the
 *                       random allocation choices will seek.  High
 *                       values will result in significant allocation
 *                       failures and a very fragmented heap.
 * @param result Struct into which to store test results.
 */
void sys_heap_stress(void *(*alloc_fn)(void *arg, size_t bytes),
		     void (*free_fn)(void *arg, void *p),
		     void *arg, size_t total_bytes,
		     uint32_t op_count,
		     void *scratch_mem, size_t scratch_bytes,
		     int target_percent,
		     struct z_heap_stress_result *result);

/** @brief Print heap internal structure information to the console
 *
 * Print information on the heap structure such as its size, chunk buckets,
 * chunk list and some statistics for debugging purpose.
 *
 * @param heap Heap to print information about
 * @param dump_chunks True to print the entire heap chunk list
 */
void sys_heap_print_info(struct sys_heap *heap, bool dump_chunks);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_SYS_HEAP_H_ */
