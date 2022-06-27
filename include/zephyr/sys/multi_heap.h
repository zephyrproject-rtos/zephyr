/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_MULTI_HEAP_H_
#define ZEPHYR_INCLUDE_SYS_MULTI_HEAP_H_

#include <zephyr/types.h>

#define MAX_MULTI_HEAPS 8

/**
 * @brief Multi-heap allocator
 *
 * A sys_multi_heap represents a single allocator made from multiple,
 * separately managed pools of memory that must be accessed via a
 * unified API.  They can be discontiguous, and in many cases will be
 * expected to have different capabilities (for example: latency,
 * cacheability, cpu affinity, etc...)
 *
 * Allocation from the multiheap provides an opaque "configuration"
 * value to specify requirements and heuristics to assist the choice
 * in backend, which is then provided to a user-specified "choice"
 * function whose job it is to select a heap based on information in
 * the config specifier and runtime state (heap full state, etc...)
 */
struct sys_multi_heap;

/**
 * @brief Multi-heap choice function
 *
 * This is a user-provided functions whose responsibility is selecting
 * a specific sys_heap backend based on the opaque cfg value, which is
 * specified by the user as an argument to sys_multi_heap_alloc(), and
 * performing the allocation on behalf of the caller.  The callback is
 * free to choose any registered heap backend to perform the
 * allocation, and may choose to pad the user-provided values as
 * needed, and to use an aligned allocation where required by the
 * specified configuration.
 *
 * NULL may be returned, which will cause the
 * allocation to fail and a NULL reported to the calling code.
 *
 * @param mheap Multi-heap structure.
 * @param cfg An opaque user-provided value.  It may be interpreted in
 *            any way by the application
 * @param align Alignment of requested memory (or zero for no alignment)
 * @param size The user-specified allocation size in bytes
 * @return A pointer to the allocated memory
 */
typedef void *(*sys_multi_heap_fn_t)(struct sys_multi_heap *mheap, void *cfg,
				     size_t align, size_t size);


struct sys_multi_heap_rec {
	struct sys_heap *heap;
	void *user_data;
};

struct sys_multi_heap {
	int nheaps;
	sys_multi_heap_fn_t choice;
	struct sys_multi_heap_rec heaps[MAX_MULTI_HEAPS];
};

/**
 * @brief Initialize multi-heap
 *
 * Initialize a sys_multi_heap struct with the specified choice
 * function.  Note that individual heaps must be added later with
 * sys_multi_heap_add_heap so that the heap bounds can be tracked by
 * the multi heap code.
 *
 * @note In general a multiheap is likely to be instantiated
 * semi-statically from system configuration (for example, via
 * linker-provided bounds on available memory in different regions, or
 * from devicetree definitions of hardware-provided addressable
 * memory, etc...).  The general expectation is that a soc- or
 * board-level platform device will be initialized at system boot from
 * these upstream configuration sources and not that an application
 * will assemble a multi-heap on its own.
 *
 * @param heap A sys_multi_heap to initialize
 * @param choice_fn A sys_multi_heap_fn_t callback used to select
 *                  heaps at allocation time
 */
void sys_multi_heap_init(struct sys_multi_heap *heap,
			 sys_multi_heap_fn_t choice_fn);

/**
 * @brief Add sys_heap to multi heap
 *
 * This adds a known sys_heap backend to an existing multi heap,
 * allowing the multi heap internals to track the bounds of the heap
 * and determine which heap (if any) from which a freed block was
 * allocated.
 *
 * @param mheap A sys_multi_heap to which to add a heap
 * @param heap The heap to add
 * @param user_data pointer to any data for the heap
 */
void sys_multi_heap_add_heap(struct sys_multi_heap *mheap, struct sys_heap *heap, void *user_data);

/**
 * @brief Allocate memory from multi heap
 *
 * Just as for sys_heap_alloc(), allocates a block of memory of the
 * specified size in bytes.  Takes an opaque configuration pointer
 * passed to the multi heap choice function, which is used by
 * integration code to choose a heap backend.
 *
 * @param mheap Multi heap pointer
 * @param cfg Opaque configuration parameter, as for sys_multi_heap_fn_t
 * @param bytes Requested size of the allocation, in bytes
 * @return A valid pointer to heap memory, or NULL if no memory is available
 */
void *sys_multi_heap_alloc(struct sys_multi_heap *mheap, void *cfg, size_t bytes);

/**
 * @brief Allocate aligned memory from multi heap
 *
 * Just as for sys_multi_heap_alloc(), allocates a block of memory of
 * the specified size in bytes.  Takes an additional parameter
 * specifying a power of two alignment, in bytes.
 *
 * @param mheap Multi heap pointer
 * @param cfg Opaque configuration parameter, as for sys_multi_heap_fn_t
 * @param align Power of two alignment for the returned pointer, in bytes
 * @param bytes Requested size of the allocation, in bytes
 * @return A valid pointer to heap memory, or NULL if no memory is available
 */
void *sys_multi_heap_aligned_alloc(struct sys_multi_heap *mheap,
				   void *cfg, size_t align, size_t bytes);

/**
 * @brief Get a specific heap for provided address
 *
 * Finds a single system heap (with user_data)
 * controlling the provided pointer
 *
 * @param mheap Multi heap pointer
 * @param addr address to be found, must be a pointer to a block allocated by sys_multi_heap_alloc
 * @return 0 multi_heap_rec pointer to a structure to be filled with return data
 *			 or NULL if the heap has not been found
 */
const struct sys_multi_heap_rec *sys_multi_heap_get_heap(const struct sys_multi_heap *mheap,
							 void *addr);

/**
 * @brief Free memory allocated from multi heap
 *
 * Returns the specified block, which must be the return value of a
 * previously successful sys_multi_heap_alloc() or
 * sys_multi_heap_aligned_alloc() call, to the heap backend from which
 * it was allocated.
 *
 * Accepts NULL as a block parameter, which is specified to have no
 * effect.
 *
 * @param mheap Multi heap pointer
 * @param block Block to free, must be a pointer to a block allocated by sys_multi_heap_alloc
 */
void sys_multi_heap_free(struct sys_multi_heap *mheap, void *block);

#endif /* ZEPHYR_INCLUDE_SYS_MULTI_HEAP_H_ */
