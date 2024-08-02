/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_H_

#include <zephyr/sys/util_macro.h>

/*
 * Generic memory attributes.
 *
 * Generic memory attributes that should be common to all architectures.
 */
#define DT_MEM_ATTR_MASK		GENMASK(15, 0)
#define DT_MEM_ATTR_GET(x)		((x) & DT_MEM_ATTR_MASK)
#define DT_MEM_ATTR_SHIFT		(0)

#define  DT_MEM_CACHEABLE		BIT(0)  /* cacheable */
#define  DT_MEM_NON_VOLATILE		BIT(1)  /* non-volatile */
#define  DT_MEM_OOO			BIT(2)  /* out-of-order */
#define  DT_MEM_DMA			BIT(3)	/* DMA-able */
#define  DT_MEM_UNKNOWN			BIT(15) /* must be last */
/* to be continued */

/*
 * Software specific memory attributes.
 *
 * Software can define their own memory attributes if needed using the
 * provided mask.
 */
#define DT_MEM_SW_ATTR_MASK		GENMASK(19, 16)
#define DT_MEM_SW_ATTR_GET(x)		((x) & DT_MEM_SW_ATTR_MASK)
#define DT_MEM_SW_ATTR_SHIFT		(16)
#define DT_MEM_SW_ATTR_UNKNOWN		BIT(19)

/*
 * Architecture specific memory attributes.
 *
 * Architectures can define their own memory attributes if needed using the
 * provided mask.
 *
 * See for example `include/zephyr/dt-bindings/memory-attr/memory-attr-arm.h`
 */
#define DT_MEM_ARCH_ATTR_MASK		GENMASK(31, 20)
#define DT_MEM_ARCH_ATTR_GET(x)		((x) & DT_MEM_ARCH_ATTR_MASK)
#define DT_MEM_ARCH_ATTR_SHIFT		(20)
#define DT_MEM_ARCH_ATTR_UNKNOWN	BIT(31)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MEM_ATTR_H_ */
