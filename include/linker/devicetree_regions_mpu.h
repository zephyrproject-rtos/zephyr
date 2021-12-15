/*
 * Copyright (c) 2021, Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <devicetree.h>
#include <linker/devicetree_regions.h>
#include <sys/util_macro.h>

/**
 * @cond INTERNAL_HIDDEN
 */

#define _DT_COMPATIBLE	mpu_region

/**
 * Check that the node_id has both properties:
 *  - zephyr,memory-region-mpu
 *  - zephyr,memory-region
 *
 * and call the EXPAND_MPU_FN() macro
 */
#define _CHECK_ATTR_FN(node_id, EXPAND_MPU_FN, ...)					\
	COND_CODE_1(UTIL_AND(DT_NODE_HAS_PROP(node_id, zephyr_memory_region_mpu),	\
			     DT_NODE_HAS_PROP(node_id, zephyr_memory_region)),		\
		   (EXPAND_MPU_FN(node_id, __VA_ARGS__)),				\
		   ())

/**
 * Call the user-provided MPU_FN() macro passing the expected arguments
 */
#define _EXPAND_MPU_FN(node_id, MPU_FN, ...)				\
	MPU_FN(LINKER_DT_NODE_REGION_NAME(node_id),			\
	       DT_REG_ADDR(node_id),					\
	       DT_REG_SIZE(node_id),					\
	       DT_STRING_TOKEN(node_id, zephyr_memory_region_mpu)),

/**
 * Call _CHECK_ATTR_FN() for each enabled node passing EXPAND_MPU_FN() as
 * explicit argument and the user-provided MPU_FN() macro in __VA_ARGS__
 */
#define _CHECK_APPLY_FN(compat, EXPAND_MPU_FN, ...)					\
	DT_FOREACH_STATUS_OKAY_VARGS(compat, _CHECK_ATTR_FN, EXPAND_MPU_FN, __VA_ARGS__)

/**
 * @endcond
 */

/**
 * Helper macro to apply an MPU_FN macro to all the memory regions declared
 * using the 'zephyr,memory-region-mpu' property and the 'mpu-region'
 * compatible.
 *
 * @p mpu_fn must take the form:
 *
 * @code{.c}
 *   #define MPU_FN(name, base, size, attr) ...
 * @endcode
 *
 * Example:
 *
 * @code{.c}
 *   static const struct arm_mpu_region mpu_regions[] = {
 *       ...
 *       DT_REGION_MPU(MPU_FN)
 *       ...
 *   };
 * @endcode
 */
#define DT_REGION_MPU(mpu_fn) _CHECK_APPLY_FN(_DT_COMPATIBLE, _EXPAND_MPU_FN, mpu_fn)
