/*
 * Copyright (c) 2021 Carlo Caione, <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for Shared Multi-Heap framework
 */

#ifndef ZEPHYR_INCLUDE_MULTI_HEAP_MANAGER_SMH_H_
#define ZEPHYR_INCLUDE_MULTI_HEAP_MANAGER_SMH_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Heap Management
 * @defgroup heaps Heap Management
 * @ingroup os_services
 * @{
 * @}
 */

/**
 * @brief Shared Multi-Heap (SMH) interface
 * @defgroup shared_multi_heap Shared multi-heap interface
 * @ingroup heaps
 * @{
 *
 * The shared multi-heap manager uses the multi-heap allocator to manage a set
 * of memory regions with different capabilities / attributes (cacheable,
 * non-cacheable, etc...).
 *
 * All the different regions can be added at run-time to the shared multi-heap
 * pool providing an opaque "attribute" value (an integer or enum value) that
 * can be used by drivers or applications to request memory with certain
 * capabilities.
 *
 * This framework is commonly used as follow:
 *
 *  - At boot time some platform code initialize the shared multi-heap
 *    framework using @ref shared_multi_heap_pool_init and add the memory
 *    regions to the pool with @ref shared_multi_heap_add, possibly gathering
 *    the needed information for the regions from the DT.
 *
 *  - Each memory region encoded in a @ref shared_multi_heap_region structure.
 *    This structure is also carrying an opaque and user-defined integer value
 *    that is used to define the region capabilities (for example:
 *    cacheability, cpu affinity, etc...)
 *
 *  - When a driver or application needs some dynamic memory with a certain
 *    capability, it can use @ref shared_multi_heap_alloc (or the aligned
 *    version) to request the memory by using the opaque parameter to select
 *    the correct set of attributes for the needed memory. The framework will
 *    take care of selecting the correct heap (thus memory region) to carve
 *    memory from, based on the opaque parameter and the runtime state of the
 *    heaps (available memory, heap state, etc...)
 */

/**
 * @brief SMH region attributes enumeration type.
 *
 * Enumeration type for some common memory region attributes.
 *
 */
enum shared_multi_heap_attr {
	/** cacheable */
	SMH_REG_ATTR_CACHEABLE,

	/** non-cacheable */
	SMH_REG_ATTR_NON_CACHEABLE,

	/** external Memory */
	SMH_REG_ATTR_EXTERNAL,

	/** must be the last item */
	SMH_REG_ATTR_NUM,
};

/** Maximum number of standard attributes. */
#define MAX_SHARED_MULTI_HEAP_ATTR SMH_REG_ATTR_NUM

/**
 * @brief SMH region struct
 *
 * This struct is carrying information about the memory region to be added in
 * the multi-heap pool.
 */
struct shared_multi_heap_region {
	/** Memory heap attribute */
	uint32_t attr;

	/** Memory heap starting virtual address */
	uintptr_t addr;

	/** Memory heap size in bytes */
	size_t size;
};

/**
 * @brief Init the pool
 *
 * This must be the first function to be called to initialize the shared
 * multi-heap pool. All the individual heaps must be added later with @ref
 * shared_multi_heap_add.
 *
 * @note As for the generic multi-heap allocator the expectation is that this
 * function will be called at soc- or board-level.
 *
 * @retval 0		on success.
 * @retval -EALREADY	when the pool was already inited.
 * @retval other	errno codes
 */
int shared_multi_heap_pool_init(void);

/**
 * @brief Allocate memory from the memory shared multi-heap pool
 *
 * Allocates a block of memory of the specified size in bytes and with a
 * specified capability / attribute. The opaque attribute parameter is used
 * by the backend to select the correct heap to allocate memory from.
 *
 * @param attr		capability / attribute requested for the memory block.
 * @param bytes		requested size of the allocation in bytes.
 *
 * @retval ptr		a valid pointer to heap memory.
 * @retval err		NULL if no memory is available.
 */
void *shared_multi_heap_alloc(enum shared_multi_heap_attr attr, size_t bytes);

/**
 * @brief Allocate aligned memory from the memory shared multi-heap pool
 *
 * Allocates a block of memory of the specified size in bytes and with a
 * specified capability / attribute. Takes an additional parameter specifying a
 * power of two alignment in bytes.
 *
 * @param attr		capability / attribute requested for the memory block.
 * @param align		power of two alignment for the returned pointer, in bytes.
 * @param bytes		requested size of the allocation in bytes.
 *
 * @retval ptr		a valid pointer to heap memory.
 * @retval err		NULL if no memory is available.
 */
void *shared_multi_heap_aligned_alloc(enum shared_multi_heap_attr attr,
				      size_t align, size_t bytes);

/**
 * @brief Free memory from the shared multi-heap pool
 *
 * Used to free the passed block of memory that must be the return value of a
 * previously call to @ref shared_multi_heap_alloc or @ref
 * shared_multi_heap_aligned_alloc.
 *
 * @param block		block to free, must be a pointer to a block allocated
 *			by shared_multi_heap_alloc or
 *			shared_multi_heap_aligned_alloc.
 */
void shared_multi_heap_free(void *block);

/**
 * @brief Add an heap region to the shared multi-heap pool
 *
 * This adds a shared multi-heap region to the multi-heap pool.
 *
 * @param user_data	pointer to any data for the heap.
 * @param region	pointer to the memory region to be added.
 *
 * @retval 0		on success.
 * @retval -EINVAL	when the region attribute is out-of-bound.
 * @retval -ENOMEM	when there are no more heaps available.
 * @retval other	errno codes
 */
int shared_multi_heap_add(struct shared_multi_heap_region *region, void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MULTI_HEAP_MANAGER_SMH_H_ */
