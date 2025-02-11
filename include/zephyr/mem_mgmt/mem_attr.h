/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MEM_ATTR_H_
#define ZEPHYR_INCLUDE_MEM_ATTR_H_

/**
 * @brief Memory-Attr Interface
 * @defgroup memory_attr_interface Memory-Attr Interface
 * @ingroup mem_mgmt
 * @{
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

#define __MEM_ATTR	zephyr_memory_attr

#define _FILTER(node_id, fn)						\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, __MEM_ATTR),		\
		    (fn(node_id)),					\
		    ())

/** @endcond */

/**
 * @brief Invokes @p fn for every status `okay` node in the tree with property
 *	  `zephyr,memory-attr`
 *
 * The macro @p fn must take one parameter, which will be a node identifier
 * with the `zephyr,memory-attr` property. The macro is expanded once for each
 * node in the tree with status `okay`. The order that nodes are visited in is
 * not specified.
 *
 * @param fn macro to invoke
 */
#define DT_MEMORY_ATTR_FOREACH_STATUS_OKAY_NODE(fn)			\
	DT_FOREACH_STATUS_OKAY_NODE_VARGS(_FILTER, fn)

/**
 * @brief memory-attr region structure.
 *
 * This structure represents the data gathered from DT about a memory-region
 * marked with memory attributes.
 */
struct mem_attr_region_t {
	/** Memory node full name */
	const char *dt_name;
	/** Memory region physical address */
	uintptr_t dt_addr;
	/** Memory region size */
	size_t dt_size;
	/** Memory region attributes */
	uint32_t dt_attr;
};

/**
 * @brief Get the list of memory regions.
 *
 * Get the list of enabled memory regions with their memory-attribute as
 * gathered by DT.
 *
 * @param region Pointer to pointer to the list of memory regions.
 *
 * @retval Number of memory regions returned in the parameter.
 */
size_t mem_attr_get_regions(const struct mem_attr_region_t **region);

/**
 * @brief Check if a buffer has correct size and attributes.
 *
 * This function is used to check if a given buffer with a given set of
 * attributes fully match a memory region in terms of size and attributes.
 *
 * This is usually used to verify that a buffer has the expected attributes
 * (for example the buffer is cacheable / non-cacheable or belongs to RAM /
 * FLASH, etc...) and it has been correctly allocated.
 *
 * The expected set of attributes for the buffer is and-matched against the
 * full set of attributes for the memory region it belongs to (bitmask). So the
 * buffer is considered matching when at least that set of attributes are valid
 * for the memory region (but the region can be marked also with other
 * attributes besides the one passed as parameter).
 *
 * @param addr Virtual address of the user buffer.
 * @param size Size of the user buffer.
 * @param attr Expected / desired attribute for the buffer.
 *
 * @retval 0 if the buffer has the correct size and attribute.
 * @retval -ENOSYS if the operation is not supported (for example if the MMU is enabled).
 * @retval -ENOTSUP if the wrong parameters were passed.
 * @retval -EINVAL if the buffer has the wrong set of attributes.
 * @retval -ENOSPC if the buffer is too big for the region it belongs to.
 * @retval -ENOBUFS if the buffer is entirely allocated outside a memory region.
 */
int mem_attr_check_buf(void *addr, size_t size, uint32_t attr);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_MEM_ATTR_H_ */
