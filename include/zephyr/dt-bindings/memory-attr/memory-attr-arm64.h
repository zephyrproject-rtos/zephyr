/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_ARM64_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_ARM64_H_

#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr.h>

/*
 * Architecture specific ARM64 MMU related attributes.
 *
 * This list is to support MMU regions configuration using DT and
 * the `zephyr,memory-attr` property for ARM64 systems.
 */
#define DT_MEM_ARM64_MASK		DT_MEM_ARCH_ATTR_MASK
#define DT_MEM_ARM64_GET(x)		((x) & DT_MEM_ARM64_MASK)
#define DT_MEM_ARM64(x)			((x) << DT_MEM_ARCH_ATTR_SHIFT)

#define  ATTR_MMU_DEVICE		BIT(0)
#define  ATTR_MMU_DEVICE_nGnRE		BIT(1)
#define  ATTR_MMU_DEVICE_GRE		BIT(2)
#define  ATTR_MMU_NORMAL_NC		BIT(3)
#define  ATTR_MMU_NORMAL		BIT(4)
#define  ATTR_MMU_NORMAL_WT		BIT(5)

#define DT_MEM_ARM64_MMU_DEVICE		DT_MEM_ARM64(ATTR_MMU_DEVICE)
#define DT_MEM_ARM64_MMU_DEVICE_nGnRE	DT_MEM_ARM64(ATTR_MMU_DEVICE_nGnRE)
#define DT_MEM_ARM64_MMU_DEVICE_GRE	DT_MEM_ARM64(ATTR_MMU_DEVICE_GRE)
#define DT_MEM_ARM64_MMU_NORMAL_NC	DT_MEM_ARM64(ATTR_MMU_NORMAL_NC)
#define DT_MEM_ARM64_MMU_NORMAL		DT_MEM_ARM64(ATTR_MMU_NORMAL)
#define DT_MEM_ARM64_MMU_NORMAL_WT	DT_MEM_ARM64(ATTR_MMU_NORMAL_WT)
#define DT_MEM_ARM64_MMU_UNKNOWN	DT_MEM_ARCH_ATTR_UNKNOWN

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_ARM64_H_ */
