/**
 * @file
 * @brief ARM64 specific memory attribute definitions for devicetree.
 * @ingroup dt_binding_mem_attr_arm64
 *
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_ARM64_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_ARM64_H_

#include <zephyr/dt-bindings/memory-attr/memory-attr.h>

/**
 * @defgroup dt_binding_mem_attr_arm64 ARM64 memory attributes
 * @ingroup dt_memory_attr_architecture
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/** @brief Mask for ARM64 architecture-specific memory attribute bits. */
#define DT_MEM_ARM64_MASK		DT_MEM_ARCH_ATTR_MASK

/**
 * @brief Shift a raw ARM64 attribute into the architecture field.
 * @param x Raw ARM64 attribute value.
 */
#define DT_MEM_ARM64(x)		((x) << DT_MEM_ARCH_ATTR_SHIFT)

/*
 * Architecture-specific sub-attribute bits (before shifting).
 *
 * These refine the generic DT_MEM_* attributes for ARM64.  They occupy
 * bits [31:20] of the zephyr,memory-attr value via DT_MEM_ARM64().
 */

/** @brief Write-Back cache policy (only meaningful with DT_MEM_CACHEABLE). */
#define ATTR_ARM64_CACHE_WB		BIT(1)

/** @endcond */

/**
 * @brief Extract ARM64-specific bits from a full <tt>zephyr,memory-attr</tt> value.
 *
 * @param x Value to extract ARM64-specific memory attribute bits from.
 *
 * @return ARM64-specific memory attribute bits.
 */
 #define DT_MEM_ARM64_GET(x)		((x) & DT_MEM_ARM64_MASK)

/*
 * Convenience macros: combinations of generic + arch-specific attributes.
 *
 * Following the subsystem's composable-bitmask design, each macro is
 * built from the generic DT_MEM_CACHEABLE flag and architecture-specific
 * sub-attributes rather than standalone one-hot enumerations.
 */

/** @brief Normal non-cacheable memory. */
#define DT_MEM_ARM64_MMU_NORMAL_NC	(0)
/** @brief Normal write-through cacheable memory. */
#define DT_MEM_ARM64_MMU_NORMAL_WT	(DT_MEM_CACHEABLE)
/** @brief Normal write-back cacheable memory. */
#define DT_MEM_ARM64_MMU_NORMAL		(DT_MEM_CACHEABLE | \
					 DT_MEM_ARM64(ATTR_ARM64_CACHE_WB))
/** @brief DT value for unknown or unsupported memory type. */
#define DT_MEM_ARM64_MMU_UNKNOWN	DT_MEM_ARCH_ATTR_UNKNOWN

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_ARM64_H_ */
