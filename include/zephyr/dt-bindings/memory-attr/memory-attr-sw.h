/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Software-defined memory attribute DT binding definitions.
 * @ingroup dt_binding_mem_attr_software
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_SW_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_SW_H_

#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr.h>

/**
 * @defgroup dt_binding_mem_attr_software Software-defined memory attributes
 * @ingroup dt_memory_attr_software
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define DT_MEM_SW_MASK			DT_MEM_SW_ATTR_MASK
#define DT_MEM_SW(x)			((x) << DT_MEM_SW_ATTR_SHIFT)

#define  ATTR_SW_ALLOC_CACHE		BIT(0)
#define  ATTR_SW_ALLOC_NON_CACHE	BIT(1)
#define  ATTR_SW_ALLOC_DMA		BIT(2)
/** @endcond */

/**
 * @brief Extract software-specific bits from a full <tt>zephyr,memory-attr</tt> value.
 *
 * @param x Value to extract software-specific memory attribute bits from.
 *
 * @return Software-specific memory attribute bits.
 */

#define DT_MEM_SW_GET(x)		((x) & DT_MEM_SW_ATTR_MASK)
/** Allocate from cached memory. */
#define DT_MEM_SW_ALLOC_CACHE		DT_MEM_SW(ATTR_SW_ALLOC_CACHE)
/** Allocate from non-cached memory. */
#define DT_MEM_SW_ALLOC_NON_CACHE	DT_MEM_SW(ATTR_SW_ALLOC_NON_CACHE)
/** Allocate from DMA-capable memory. */
#define DT_MEM_SW_ALLOC_DMA		DT_MEM_SW(ATTR_SW_ALLOC_DMA)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_SW_H_ */
