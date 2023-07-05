/*
 * Copyright (c) 2023 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MEMORY_ATTR_H_
#define ZEPHYR_INCLUDE_MEMORY_ATTR_H_

#include <zephyr/devicetree.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/sys/util.h>

/**
 * @file
 * @brief Memory-attr helpers
 */

/**
 * @defgroup devicetree-memory-attr Memory attributes
 * @ingroup devicetree
 * @{
 */

/** @cond INTERNAL_HIDDEN */

#define _DT_MEM_ATTR	zephyr_memory_attr
#define _DT_ATTR(token)	UTIL_CAT(UTIL_CAT(REGION_, token), _ATTR)

#define _UNPACK(node_id, fn)						\
	fn(COND_CODE_1(DT_NODE_HAS_PROP(node_id, zephyr_memory_region),	\
		       (LINKER_DT_NODE_REGION_NAME(node_id)),		\
		       (DT_NODE_FULL_NAME(node_id))),			\
	   DT_REG_ADDR(node_id),					\
	   DT_REG_SIZE(node_id),					\
	   _DT_ATTR(DT_STRING_TOKEN(node_id, _DT_MEM_ATTR))),

#define _APPLY(node_id, fn)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, _DT_MEM_ATTR),	\
		    (_UNPACK(node_id, fn)),			\
		    ())


#define _FILTER(node_id, fn)					\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, _DT_MEM_ATTR),	\
		    (fn(node_id)),				\
		    ())

/** @endcond */

/**
 * @brief Invokes @p fn for every node in the tree with property
 *        `zephyr,memory-attr`
 *
 * The macro @p fn must take one parameter, which will be a node identifier
 * with the `zephyr,memory-attr` property. The macro is expanded once for each
 * node in the tree. The order that nodes are visited in is not specified.
 *
 * @param fn macro to invoke
 */
#define DT_MEMORY_ATTR_FOREACH_NODE(fn) \
	DT_FOREACH_STATUS_OKAY_NODE_VARGS(_FILTER, fn)

/**
 * @brief Invokes @p fn for MPU/MMU regions generation from the device tree
 *	  nodes with `zephyr,memory-attr` property.
 *
 * Helper macro to invoke a @p fn macro on all the memory regions declared
 * using the `zephyr,memory-attr` property
 *
 * The macro @p fn must take the form:
 *
 * @code{.c}
 *   #define MPU_FN(name, base, size, attr) ...
 * @endcode
 *
 * The @p name, @p base and @p size parameters are retrieved from the DT node.
 * When the `zephyr,memory-region` property is present in the node, the @p name
 * parameter is retrived from there, otherwise the full node name is used.
 *
 * The `zephyr,memory-attr` enum property is passed as an extended token
 * to the @p fn macro using the @p attr parameter in the form of a macro
 * REGION_{attr}_ATTR.
 *
 * The following enums are supported for the `zephyr,memory-attr` property (see
 * `zephyr,memory-attr.yaml` for a complete list):
 *
 *  - RAM
 *  - RAM_NOCACHE
 *  - FLASH
 *  - PPB
 *  - IO
 *  - EXTMEM
 *
 * This means that usually the user code would provide some macros or defines
 * with the same name of the extended property, that is:
 *
 *  - REGION_RAM_ATTR
 *  - REGION_RAM_NOCACHE_ATTR
 *  - REGION_FLASH_ATTR
 *  - REGION_PPB_ATTR
 *  - REGION_IO_ATTR
 *  - REGION_EXTMEM_ATTR
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     / {
 *             soc {
 *                     res0: memory@20000000 {
 *                         reg = <0x20000000 0x4000>;
 *                         zephyr,memory-region = "MY_NAME";
 *                         zephyr,memory-attr = "RAM_NOCACHE";
 *                     };
 *
 *                     res1: memory@30000000 {
 *                         reg = <0x30000000 0x2000>;
 *                         zephyr,memory-attr = "RAM";
 *                     };

 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *   #define REGION_RAM_NOCACHE_ATTR 0xAAAA
 *   #define REGION_RAM_ATTR         0xBBBB
 *   #define REGION_FLASH_ATTR       0xCCCC
 *
 *   #define MPU_FN(p_name, p_base, p_size, p_attr) \
 *       {                                          \
 *           .name = p_name,                        \
 *           .base = p_base,                        \
 *           .size = p_size,                        \
 *           .attr = p_attr,                        \
 *       }
 *
 *   static const struct arm_mpu_region mpu_regions[] = {
 *       DT_MEMORY_ATTR_APPLY(MPU_FN)
 *   };
 * @endcode
 *
 * This expands to:
 *
 * @code{.c}
 *   static const struct arm_mpu_region mpu_regions[] = {
 *       { "MY_NAME",         0x20000000, 0x4000, 0xAAAA },
 *       { "memory@30000000", 0x30000000, 0x2000, 0xBBBB },
 *   };
 * @endcode
 *
 * @param fn macro to invoke
 */
#define DT_MEMORY_ATTR_APPLY(fn) \
	DT_FOREACH_STATUS_OKAY_NODE_VARGS(_APPLY, fn)

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MEMORY_ATTR_H_ */
