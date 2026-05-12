/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM MPU memory attribute DT binding definitions (legacy).
 * @ingroup dt_binding_mem_attr_arm
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_ARM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_ARM_H_

#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr.h>

/**
 * @defgroup dt_binding_mem_attr_arm ARM MPU memory attributes
 * @ingroup dt_memory_attr_architecture
 *
 * @note This is legacy and should NOT be extended further. New MPU region
 *       types must rely on the generic memory attributes.
 * @{
 */

/*
 * Architecture specific ARM MPU related attributes.
 *
 * This list is to seamlessly support the MPU regions configuration using DT and
 * the `zephyr,memory-attr` property.
 *
 * This is legacy and it should NOT be extended further. If new MPU region
 * types must be added, these must rely on the generic memory attributes.
 */

/** @cond INTERNAL_HIDDEN */
#define DT_MEM_ARM_MASK   DT_MEM_ARCH_ATTR_MASK
#define DT_MEM_ARM(x)     ((x) << DT_MEM_ARCH_ATTR_SHIFT)

#define ATTR_MPU_RAM         BIT(0)
#define ATTR_MPU_RAM_NOCACHE BIT(1)
#define ATTR_MPU_FLASH       BIT(2)
#define ATTR_MPU_PPB         BIT(3)
#define ATTR_MPU_IO          BIT(4)
#define ATTR_MPU_EXTMEM      BIT(5)
#define ATTR_MPU_RAM_PXN     BIT(6)
#define ATTR_MPU_DEVICE      BIT(7)
#define ATTR_MPU_RAM_WT      BIT(8)
/** @endcond */

/**
 * @brief Extract ARM MPU-specific bits from a full <tt>zephyr,memory-attr</tt> value.
 *
 * @param x Value to extract ARM MPU-specific memory attribute bits from.
 *
 * @return ARM MPU-specific memory attribute bits.
 */
#define DT_MEM_ARM_GET(x) ((x) & DT_MEM_ARM_MASK)

/** Standard cacheable RAM region. */
#define DT_MEM_ARM_MPU_RAM         DT_MEM_ARM(ATTR_MPU_RAM)
/** Non-cacheable RAM region. */
#define DT_MEM_ARM_MPU_RAM_NOCACHE DT_MEM_ARM(ATTR_MPU_RAM_NOCACHE)
/** Flash (ROM) memory region. */
#define DT_MEM_ARM_MPU_FLASH       DT_MEM_ARM(ATTR_MPU_FLASH)
/** Private Peripheral Bus region. */
#define DT_MEM_ARM_MPU_PPB         DT_MEM_ARM(ATTR_MPU_PPB)
/** I/O (peripheral) memory region. */
#define DT_MEM_ARM_MPU_IO          DT_MEM_ARM(ATTR_MPU_IO)
/** External memory region. */
#define DT_MEM_ARM_MPU_EXTMEM      DT_MEM_ARM(ATTR_MPU_EXTMEM)
/** RAM region with Privileged Execute Never attribute. */
#define DT_MEM_ARM_MPU_RAM_PXN     DT_MEM_ARM(ATTR_MPU_RAM_PXN)
/** Device memory region. */
#define DT_MEM_ARM_MPU_DEVICE      DT_MEM_ARM(ATTR_MPU_DEVICE)
/** Write-through cacheable RAM region. */
#define DT_MEM_ARM_MPU_RAM_WT      DT_MEM_ARM(ATTR_MPU_RAM_WT)
/** Unknown or unsupported ARM MPU memory type. */
#define DT_MEM_ARM_MPU_UNKNOWN     DT_MEM_ARCH_ATTR_UNKNOWN

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_ARM_H_ */
