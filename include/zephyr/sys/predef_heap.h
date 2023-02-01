/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef ZEPHYR_INCLUDE_SYS_PREDEF_HEAP_H_
#define ZEPHYR_INCLUDE_SYS_PREDEF_HEAP_H_

#include <zephyr/types.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

/* Simple heap implementation with predefined buffers size.
 *
 * The blob of memory passed to the heap will be divided into a predefined number of buffers
 * of specified sizes. Buffers of various sizes are supported. Configuration of a memory division
 * for a buffers can be changed in runtime. The allocator tries to allocate the smallest buffer that
 * satisfies the request. Adjacent buffers are not merged, which eliminates the problem of memory
 * fragmentation. Allocate and free by user pointer and not an opaque block handle. Current
 * configuration and bookkeeping is stored in a separate memory.
 */

/* Configuration of a single bundle of buffers. Defines its count and size. */
struct predef_heap_config {
	size_t size;
	int count;
};

/* predef_heap object structure */
struct predef_heap {
	int bundles_count;
	void *start;
	size_t size;
	/* Pointer and size to memory used for configuration store and bookkeeping. */
	void *config;
	size_t config_size;
};

/* Private predef_heap structure. Used by allocator, tests and PREDEFINED_HEAP_DEFINE macro. */
struct predef_heap_bundle {
	uintptr_t first_buffer;
	size_t buffer_size;
	int buffers_count;
	int free_count;
	unsigned long *bitfield;
};

/**
 * @brief Create a predefined heap object.
 *
 * @param name Name of the predefined heap object.
 * @param max_bundles Maximum number of bundles.
 * @param additional_buffers By default, each bundle can consist of BITS_PER_LONG buffers. If more
 *			     buffers are needed, specify the number here.
 */
#define PREDEFINED_HEAP_DEFINE(name, max_bundles, additional_buffers)				\
	unsigned long _predefined_head_config_store_##name[ceiling_fraction(			\
		max_bundles * (sizeof(struct predef_heap_bundle) + sizeof(unsigned long)) +	\
		ceiling_fraction(additional_buffers, 8),					\
		sizeof(unsigned long))] = {0};							\
	struct predef_heap name = {								\
		.config_size = sizeof(_predefined_head_config_store_##name),			\
		.config = (struct predef_heap_bundle *)_predefined_head_config_store_##name	\
	}

/** @brief Initialize predef_heap
 *
 * Initializes a predef_heap struct to manage the specified memory.
 *
 * @param heap Heap to initialize
 * @param config Pointer to buffers configuration array
 * @param config_entries Count of elements in configuration array
 * @param memory Untyped pointer to unused memory
 * @param mem_size Size of region pointed to by @a memory
 */
int predef_heap_init(struct predef_heap *heap, const struct predef_heap_config *config,
		     const int config_entries, void *memory, const uint32_t mem_size);

/** @brief Reconfigure predef_heap
 *
 * Reconfigure a predef_heap struct to use a new configuration. All memory blocks must be freed
 * before reconfiguration can be performed.
 *
 * @param heap Heap to initialize
 * @param config Pointer to buffers configuration array
 * @param config_entries Count of elements in configuration array
 */
int predef_heap_reconfigure(struct predef_heap *heap, const struct predef_heap_config *config,
			    const int config_entries);

/** @brief Allocate aligned memory from a predef_heap
 *
 * Returns a pointer to a block of unused memory in the heap. This memory will not otherwise be used
 * until it is freed with predef_heap_free(). If no memory can be allocated, NULL will be returned.
 *
 * Returned memory (if available) will have a starting address in memory which is a multiple of the
 * specified power-of-two alignment value in bytes. The resulting memory can be returned to the heap
 * using predef_heap_free().
 *
 * @note The predef_heap implementation is not internally synchronized. No two predef_heap functions
 * should operate on the same heap at the same time. All locking must be provided by the user.
 *
 * @param heap Heap from which to allocate
 * @param align Alignment in bytes, must be a power of two
 * @param bytes Number of bytes requested
 * @return Pointer to memory the caller can now use
 */
void *predef_heap_aligned_alloc(struct predef_heap *heap, size_t align, size_t bytes);

/** @brief Allocate memory from a predef_heap
 *
 * Returns a pointer to a block of unused memory in the heap. This memory will not otherwise be used
 * until it is freed with predef_heap_free(). If no memory can be allocated, NULL will be returned.
 *
 * @note The predef_heap implementation is not internally synchronized. No two predef_heap functions
 * should operate on the same heap at the same time. All locking must be provided by the user.
 *
 * @param heap Heap from which to allocate
 * @param bytes Number of bytes requested
 * @return Pointer to memory the caller can now use
 */
static inline void *predef_heap_alloc(struct predef_heap *heap, const size_t bytes)
{
	return predef_heap_aligned_alloc(heap, 0, bytes);
}

/** @brief Free memory into a predef_heap
 *
 * De-allocates a pointer to memory previously returned from predef_heap_alloc such that it can be
 * used for other purposes. The caller must not use the memory region after entry to this function.
 *
 * @note The predef_heap implementation is not internally synchronized. No two predef_heap functions
 * should operate on the same heap at the same time. All locking must be provided by the user.
 *
 * @param heap Heap to which to return the memory
 * @param mem A pointer previously returned from predef_heap_alloc()
 */
int predef_heap_free(struct predef_heap *heap, void *mem);

/** @brief Checks if a buffer of the given size can be allocated from a predef_heap
 *
 * Checks if a buffer of the given size can be allocated from a predef_heap. No memory allocation is
 * made by this function. The alignment can be 0 if it is not important.
 *
 * @param heap Heap to check space
 * @param align Alignment in bytes, must be a power of two.
 * @param bytes Number of bytes requested
 * @return 0 if memory can be allocated, -ENOSPC otherwise.
 */
int predef_heap_check_space(struct predef_heap *heap, size_t align, const size_t bytes);

/** @brief Return allocated memory size
 *
 * Returns the size, in bytes, of a block returned from a successful predef_heap_alloc() or
 * predef_heap_aligned_alloc() call. The value returned is the size of the assigned buffer, which
 * may be larger than the number of bytes requested.
 *
 * @param heap Heap containing the block
 * @param mem Pointer to memory allocated from this heap
 * @return Size in bytes of the memory region
 */
size_t predef_heap_usable_size(struct predef_heap *heap, void *mem);

#endif /* ZEPHYR_INCLUDE_SYS_PREDEF_HEAP_H_ */
