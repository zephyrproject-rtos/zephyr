/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_SW_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_SW_H_

#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/memory-attr/memory-attr.h>

/*
 * Software specific memory attributes.
 */
#define DT_MEM_SW_MASK			DT_MEM_SW_ATTR_MASK
#define DT_MEM_SW_GET(x)		((x) & DT_MEM_SW_ATTR_MASK)
#define DT_MEM_SW(x)			((x) << DT_MEM_SW_ATTR_SHIFT)

#define  ATTR_SW_ALLOC_CACHE		BIT(0)
#define  ATTR_SW_ALLOC_NON_CACHE	BIT(1)
#define  ATTR_SW_ALLOC_DMA		BIT(2)

#define DT_MEM_SW_ALLOC_CACHE		DT_MEM_SW(ATTR_SW_ALLOC_CACHE)
#define DT_MEM_SW_ALLOC_NON_CACHE	DT_MEM_SW(ATTR_SW_ALLOC_NON_CACHE)
#define DT_MEM_SW_ALLOC_DMA		DT_MEM_SW(ATTR_SW_ALLOC_DMA)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_SW_H_ */
