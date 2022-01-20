/*
 * Copyright (c) 2021, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generate memory regions from devicetree nodes.
 */

/**
 * @brief Get the linker memory-region name
 *
 * This attempts to use the zephyr,memory-region property, falling back
 * to the node path if it doesn't exist.
 *
 * Example devicetree fragment:
 *
 *     / {
 *             soc {
 *                     sram1: memory@2000000 {
 *                         zephyr,memory-region = "MY_NAME";
 *                     };
 *                     sram2: memory@2001000 { ... };
 *             };
 *     };
 *
 * Example usage:
 *
 *    LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(sram1)) // "MY_NAME"
 *    LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(sram2)) // "/soc/memory@2001000"
 *
 * @param node_id node identifier
 * @return the name of the memory memory region the node will generate
 */
#define LINKER_DT_NODE_REGION_NAME(node_id) \
	DT_PROP_OR(node_id, zephyr_memory_region, DT_NODE_PATH(node_id))

/** @cond INTERNAL_HIDDEN */

#define _DT_COMPATIBLE	zephyr_memory_region

/**
 * @brief Declare a memory region
 *
 * @param node_id devicetree node identifier
 * @param attr region attributes
 */
#define _REGION_DECLARE(node_id)	      \
	LINKER_DT_NODE_REGION_NAME(node_id) : \
	ORIGIN = DT_REG_ADDR(node_id),	      \
	LENGTH = DT_REG_SIZE(node_id)

/** @endcond */

/**
 * @brief Generate linker memory regions from the device tree nodes with
 *        compatible 'zephyr,memory-region'
 *
 * Note: for now we do not deal with MEMORY attributes since those are
 * optional, not actually used by Zephyr and they will likely conflict with the
 * MPU configuration.
 */
#define LINKER_DT_REGIONS() \
	DT_FOREACH_STATUS_OKAY(_DT_COMPATIBLE, _REGION_DECLARE)
