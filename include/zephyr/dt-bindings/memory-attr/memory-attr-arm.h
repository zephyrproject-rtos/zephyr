/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_ARM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_ARM_H_

#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr.h>

/*
 * Architecture specific ARM MPU related attributes.
 *
 * This list is to seamlessly support the MPU regions configuration using DT and
 * the `zephyr,memory-attr` property.
 *
 * This is legacy and it should NOT be extended further. If new MPU region
 * types must be added, these must rely on the generic memory attributes.
 */
#define DT_MEM_ARM_MASK			DT_MEM_ARCH_ATTR_MASK
#define DT_MEM_ARM_GET(x)		((x) & DT_MEM_ARM_MASK)
#define DT_MEM_ARM(x)			((x) << DT_MEM_ARCH_ATTR_SHIFT)

#define  ATTR_MPU_RAM			BIT(0)
#define  ATTR_MPU_RAM_NOCACHE		BIT(1)
#define  ATTR_MPU_FLASH			BIT(2)
#define  ATTR_MPU_PPB			BIT(3)
#define  ATTR_MPU_IO			BIT(4)
#define  ATTR_MPU_EXTMEM		BIT(5)

#define DT_MEM_ARM_MPU_RAM		DT_MEM_ARM(ATTR_MPU_RAM)
#define DT_MEM_ARM_MPU_RAM_NOCACHE	DT_MEM_ARM(ATTR_MPU_RAM_NOCACHE)
#define DT_MEM_ARM_MPU_FLASH		DT_MEM_ARM(ATTR_MPU_FLASH)
#define DT_MEM_ARM_MPU_PPB		DT_MEM_ARM(ATTR_MPU_PPB)
#define DT_MEM_ARM_MPU_IO		DT_MEM_ARM(ATTR_MPU_IO)
#define DT_MEM_ARM_MPU_EXTMEM		DT_MEM_ARM(ATTR_MPU_EXTMEM)
#define DT_MEM_ARM_MPU_UNKNOWN		DT_MEM_ARCH_ATTR_UNKNOWN

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_ARM_H_ */
