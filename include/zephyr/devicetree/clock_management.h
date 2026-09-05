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

/** @cond INTERNAL_HIDDEN */


/**
 * @brief Defines a clock request from devicetree
 *
 * Defines a clock request from devicetree. Intended to be used with
 * @ref DT_CLOCK_REQUEST_FOREACH_BY_NAME to generate clock request structures
 * @param node_id Node identifier with the clock-request-n property
 * @param _allowed_states Allowed states for this clock request
 */
#define DT_GET_CLOCK_REQUEST(node_id, _allowed_states) \
	{ \
		.clk = CLOCK_DT_GET(node_id), \
		.allowed_states = _allowed_states, \
	},

/** @endcond */

/**
 * @brief Calls @p fn for every clock request in a clock-request-n array
 *
 * Calls @p fn for every clock request in a clock-request-n array, with the
 * arguments being the clock node identifier and the allowed states for that
 * clock request. The clock-request-n array is defined from the clock-request-n
 * property, but phandles that affect the same clock output are grouped together
 * to form one "allowed_state" bitmask for that clock. The @p fn macro
 * should take two arguments:
 * 1. The clock node identifier
 * 2. The allowed states for that clock request
 * @param node_id Node identifier with the clock-request-n property
 * @param request_name Name of the clock request to get for this clock output
 * @param fn Macro to call for each clock request
 */
#define DT_CLOCK_REQUEST_FOREACH_BY_NAME(node_id, request_name, fn) \
	CONCAT(node_id, _CLOCK_REQUEST_, \
		DT_CLOCK_REQUEST_NAME_IDX(node_id, request_name), _FOREACH)(fn)

/**
 * @brief Get the length of a clock-request-n array by the name
 *
 * Gets the length of a given clock-request-n array by the name, set by
 * clock-request-names property. Note that since the clock-request-n
 * phandles are grouped by clock output they affect, the length may not
 * match the number of phandles in the clock-request-n property.
 * @param node_id Node identifier with the clock-request-n property
 * @param name Name of the clock request to get the length of
 */
#define DT_CLOCK_REQUEST_LEN_BY_NAME(node_id, name) \
	CONCAT(node_id, _CLOCK_REQUEST_, DT_CLOCK_REQUEST_NAME_IDX(node_id, name), _LEN)

/**
 * @brief Get index of clock output name
 * @param node_id Node ID with clock-output-names property
 * @param name Name in the clock-output-names property to get the index of
 */
#define DT_CLOCK_OUTPUT_NAME_IDX(node_id, name) \
	DT_CAT4(node_id, _CLOCK_OUTPUT_NAME_, name, _IDX)

/**
 * @brief Get index of clock request
 * @param node_id Node ID with clock-request-names property
 * @param name Name in the clock-request-names property to get the index of
 */
#define DT_CLOCK_REQUEST_NAME_IDX(node_id, name) \
	DT_CAT4(node_id, _CLOCK_REQUEST_NAME_, name, _IDX)

/**
 * @brief Get a list of dependency ordinals of clocks that depend on a node
 *
 * This differs from `DT_SUPPORTS_DEP_ORDS` in that clock nodes that
 * reference the clock via the clock-request-n property will not be present
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

/**
 * @brief Call @p fn for every clock state of a node that is in use
 *
 * Call @p fn for every clock state of a clock-output node that is referenced
 * by a clock-request-n property. This macro will call @p fn in the order
 * that clock states are ranked, with the lowest ranked state first.
 */
#define DT_CLOCK_STATE_SORTED_FOREACH(node_id, fn) \
	CONCAT(node_id, _CLOCK_STATE_SORTED_FOREACH)(fn)

/**
 * @brief Call @p fn for every clock state of an instance that is in use
 *
 * Call @p fn for every clock state of a clock-output instance that is referenced
 * by a clock-request-n property. This macro will call @p fn in the order
 * that clock states are ranked, with the lowest ranked state first.
 * It is equivalent to DT_CLOCK_STATE_SORTED_FOREACH(DT_DRV_INST(inst), fn).
 */
#define DT_INST_CLOCK_STATE_SORTED_FOREACH(inst, fn) \
	DT_CLOCK_STATE_SORTED_FOREACH(DT_DRV_INST(inst), fn)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_CLOCK_MANAGEMENT_H_ */
