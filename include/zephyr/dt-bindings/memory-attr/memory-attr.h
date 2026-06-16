/**
 * @file
 * @brief Generic devicetree memory attribute definitions.
 * @ingroup dt_memory_attr
 *
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_H_

#include <zephyr/sys/util_macro.h>

/**
 * @defgroup dt_memory_attr Devicetree memory attributes
 * @ingroup devicetree
 *
 * Devicetree macros for memory attributes.
 *
 * Memory attributes are descriptive properties that can be assigned to memory regions in Devicetree
 * using the @c zephyr,memory-attr property, and that describe their capabilities or characteristics
 * (e.g. cacheability, non-volatility, out-of-order access, DMA capability, etc.).
 *
 * @{
 *
 * @defgroup dt_memory_attr_generic Generic memory attributes
 *
 * Generic memory attributes that should be common to all architectures.
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define DT_MEM_ATTR_MASK		GENMASK(15, 0)
#define DT_MEM_ATTR_SHIFT		(0)
/** @endcond */

/**
 * Extract generic memory attribute bits from a full <tt>zephyr,memory-attr</tt> value.
 *
 * @param x Value to extract generic memory attribute bits from.
 *
 * @return Generic memory attribute bits.
 */
#define DT_MEM_ATTR_GET(x)		((x) & DT_MEM_ATTR_MASK)

/** @brief Memory is cacheable. */
#define  DT_MEM_CACHEABLE		BIT(0)
/** @brief Memory is non-volatile. */
#define  DT_MEM_NON_VOLATILE		BIT(1)
/** @brief Memory supports out-of-order access. */
#define  DT_MEM_OOO			BIT(2)
/** @brief Memory is usable for DMA. */
#define  DT_MEM_DMA			BIT(3)
/** @brief Generic memory attributes are unknown. */
#define  DT_MEM_UNKNOWN			BIT(15)
/* to be continued */

/** @} */

/**
 * @defgroup dt_memory_attr_software Software-specific memory attributes
 *
 * Software-specific memory attributes. They must reside in the section of the @c zephyr,memory-attr
 * value corresponding to DT_MEM_ARCH_ATTR_MASK.
 *
 * @{
 */

/** Mask for software-specific memory attribute bits. */
#define DT_MEM_SW_ATTR_MASK                GENMASK(19, 16)

/** Extract software-specific memory attribute bits from a full <tt>zephyr,memory-attr</tt> value.
 *
 * @param x Value to extract software-specific memory attribute bits from.
 *
 * @return Software-specific memory attribute bits.
 */
#define DT_MEM_SW_ATTR_GET(x) ((x) & DT_MEM_SW_ATTR_MASK)

/** Shift for software-specific memory attribute bits. */
#define DT_MEM_SW_ATTR_SHIFT (16)

/** Software-specific memory attributes are unknown. */
#define DT_MEM_SW_ATTR_UNKNOWN BIT(19)

/** @} */

/**
 * @defgroup dt_memory_attr_architecture Architecture-specific memory attributes
 *
 * Architecture-specific memory attributes. They must reside in the section of the
 * @c zephyr,memory-attr value corresponding to DT_MEM_ARCH_ATTR_MASK.
 *
 * @{
 */

/** Mask for architecture-specific memory attribute bits. */
#define DT_MEM_ARCH_ATTR_MASK		GENMASK(31, 20)

/**
 * Extract architecture-specific memory attribute bits from a full <tt>zephyr,memory-attr</tt>
 * value.
 *
 * @param x Value to extract architecture-specific memory attribute bits from.
 *
 * @return Architecture-specific memory attribute bits.
 */
#define DT_MEM_ARCH_ATTR_GET(x)		((x) & DT_MEM_ARCH_ATTR_MASK)

/** Shift for architecture-specific memory attribute bits. */
#define DT_MEM_ARCH_ATTR_SHIFT		(20)

/** Architecture-specific memory attributes are unknown. */
#define DT_MEM_ARCH_ATTR_UNKNOWN	BIT(31)

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_H_ */
