/*
 * Copyright (c) 2023 Carlo Caione, <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MEM_ATTR_HEAP_H_
#define ZEPHYR_INCLUDE_MEM_ATTR_HEAP_H_

/**
 * @brief Memory heaps based on memory attributes
 * @defgroup memory_attr_heap Memory heaps based on memory attributes
 * @ingroup mem_mgmt
 * @{
 */

#include <zephyr/mem_mgmt/mem_attr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init the memory pool
 *
 * This must be the first function to be called to initialize the memory pools
 * from all the memory regions with the a software attribute.
 *
 * @retval 0 on success.
 * @retval -EALREADY if the pool was already initialized.
 * @retval -ENOMEM too many regions already allocated.
 */
int mem_attr_heap_pool_init(void);

/**
 * @brief Allocate memory with a specified attribute and size.
 *
 * Allocates a block of memory of the specified size in bytes and with a
 * specified capability / attribute. The attribute is used to select the
 * correct memory heap to allocate memory from.
 *
 * @param attr capability / attribute requested for the memory block.
 * @param bytes requested size of the allocation in bytes.
 *
 * @retval ptr a valid pointer to the allocated memory.
 * @retval NULL if no memory is available with that attribute and size.
 */
void *mem_attr_heap_alloc(uint32_t attr, size_t bytes);

/**
 * @brief Allocate aligned memory with a specified attribute, size and alignment.
 *
 * Allocates a block of memory of the specified size in bytes and with a
 * specified capability / attribute. Takes an additional parameter specifying a
 * power of two alignment in bytes.
 *
 * @param attr capability / attribute requested for the memory block.
 * @param align power of two alignment for the returned pointer in bytes.
 * @param bytes requested size of the allocation in bytes.
 *
 * @retval ptr a valid pointer to the allocated memory.
 * @retval NULL if no memory is available with that attribute and size.
 */
void *mem_attr_heap_aligned_alloc(uint32_t attr, size_t align, size_t bytes);

/**
 * @brief Free the allocated memory
 *
 * Used to free the passed block of memory that must be the return value of a
 * previously call to @ref mem_attr_heap_alloc or @ref
 * mem_attr_heap_aligned_alloc.
 *
 * @param block block to free, must be a pointer to a block allocated by
 *	  @ref mem_attr_heap_alloc or @ref mem_attr_heap_aligned_alloc.
 */
void mem_attr_heap_free(void *block);

/**
 * @brief Get a specific memory region descriptor for a provided address
 *
 * Finds the memory region descriptor struct controlling the provided pointer.
 *
 * @param addr address to be found, must be a pointer to a block allocated by
 *	       @ref mem_attr_heap_alloc or @ref mem_attr_heap_aligned_alloc.
 *
 * @retval str pointer to a memory region structure the address belongs to.
 */
const struct mem_attr_region_t *mem_attr_heap_get_region(void *addr);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MEM_ATTR_HEAP_H_ */
