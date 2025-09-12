/**
 * @file
 * @brief Clock Management Devicetree macro public API header file.
 */

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_CLOCK_MANAGEMENT_H_
#define ZEPHYR_INCLUDE_DEVICETREE_CLOCK_MANAGEMENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/devicetree.h>

/**
 * @defgroup devicetree-clock-management Devicetree Clock Management API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get index of clock output name
 * @param node_id Node ID with clock-output-names property
 * @param name Name in the clock-output-names property to get the index of
 */
#define DT_CLOCK_OUTPUT_NAME_IDX(node_id, name) \
	DT_CAT4(node_id, _CLOCK_OUTPUT_NAME_, name, _IDX)

/**
 * @brief Get index of clock state
 * @param node_id Node ID with clock-state-names property
 * @param name Name in the clock-state-names states to get the index of
 */
#define DT_CLOCK_STATE_NAME_IDX(node_id, name) \
	DT_CAT4(node_id, _CLOCK_STATE_NAME_, name, _IDX)

/**
 * @brief Get a list of dependency ordinals of clocks that depend on a node
 *
 * This differs from `DT_SUPPORTS_DEP_ORDS` in that clock nodes that
 * reference the clock via the clock-state-n property will not be present
 * in this list.
 *
 * There is a comma after each ordinal in the expansion, **including**
 * the last one:
 *
 *     DT_SUPPORTS_CLK_ORDS(my_node) // supported_ord_1, ..., supported_ord_n,
 *
 * DT_SUPPORTS_CLK_ORDS() may expand to nothing. This happens when @p node_id
 * refers to a leaf node that nothing else depends on.
 *
 * @param node_id Node identifier
 * @return a list of dependency ordinals, with each ordinal followed
 *         by a comma (<tt>,</tt>), or an empty expansion
 */
#define DT_SUPPORTS_CLK_ORDS(node_id) DT_CAT(node_id, _SUPPORTS_CLK_ORDS)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_CLOCK_MANAGEMENT_H_ */
