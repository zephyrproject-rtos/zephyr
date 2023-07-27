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

/**
 * @brief memory-attr region structure.
 *
 * This structure represents the data gathered from DT about a memory-region
 * marked with memory attribute.
 */
struct mem_attr_region_t {
	/** Memory region physical address */
	uintptr_t dt_addr;
	/** Memory region size */
	size_t dt_size;
	/** Memory region attribute */
	enum dt_memory_attr dt_attr;
};

/**
 * @brief Get the list of memory regions.
 *
 * Get the list of enabled memory regions with their memory-attribute as
 * gathered by DT.
 *
 * @retval a const struct pointer to the memory regions array.
 * @retval NULL if there are no regions.
 */
const struct mem_attr_region_t *mem_attr_get_regions(void);

/**
 * @brief Check if a buffer has correct size and attributes.
 *
 * This function is used to check if a given buffer with a given attribute
 * fully match a memory regions in terms of size and attributes. This is
 * usually used to verify that a buffer has the expected attributes (for
 * example the buffer is cacheable / non-cacheable or belongs to RAM / FLASH,
 * etc...).
 *
 * @param addr Virtual address of the user buffer.
 * @param size Size of the user buffer.
 * @param attr Expected / desired attribute for the buffer.
 *
 * @retval 0 if the buffer has the correct size and attribute.
 * @retval -ENOSYS if the operation is not supported (for example if the MMU is enabled).
 * @retval -ENOTSUP if the wrong parameters were passed.
 * @retval -EINVAL if the buffer has the wrong attribute.
 * @retval -ENOSPC if the buffer is too big for the region it belongs to.
 * @retval -ENOBUFS if the buffer is allocated outside a memory region.
 */
int mem_attr_check_buf(void *addr, size_t size, enum dt_memory_attr attr);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_MEM_ATTR_H_ */
