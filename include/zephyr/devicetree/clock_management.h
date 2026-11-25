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
 * @ref DT_CLOCK_REQUEST_FOREACH_BY_IDX to generate clock request structures
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
 * @brief Get the number of clock-request-<i> properties on a node
 *
 * @param node_id Node identifier; may or may not have any clock-request-<i>
 *                properties
 * @return Number of clock-request-<i> properties (0 if there are none)
 */
#define DT_NUM_CLOCK_REQUESTS(node_id) DT_CAT(node_id, _CLOCK_REQUEST_NUM)

/**
 * @brief Test if a node has a clock-request-<idx> property
 *
 * @param node_id Node identifier
 * @param idx Clock request index
 * @return 1 if the property exists, 0 otherwise
 */
#define DT_CLOCK_REQUEST_HAS_IDX(node_id, idx) \
	IS_ENABLED(DT_CAT4(node_id, _CLOCK_REQUEST_IDX_, idx, _EXISTS))

/**
 * @brief Test if a node has a clock request with a given name
 *
 * @param node_id Node identifier
 * @param name Lowercase-and-underscores clock request name, from the
 *             clock-request-names property
 * @return 1 if the node has a clock request with that name, 0 otherwise
 */
#define DT_CLOCK_REQUEST_HAS_NAME(node_id, name) \
	IS_ENABLED(DT_CAT4(node_id, _CLOCK_REQUEST_NAME_, name, _EXISTS))

/**
 * @brief Convert a clock request index to its name as a token
 *
 * Converts the index of a clock-request-<idx> property to the corresponding
 * entry in the clock-request-names property, as a token. The same caveats
 * that apply to DT_STRING_TOKEN() apply here.
 *
 * @param node_id Node identifier
 * @param idx Clock request index
 * @return Clock request name as a token
 */
#define DT_CLOCK_REQUEST_IDX_TO_NAME_TOKEN(node_id, idx) \
	DT_CAT4(node_id, _CLOCK_REQUEST_IDX_, idx, _TOKEN)

/**
 * @brief Convert a clock request index to its name as an uppercased token
 *
 * Like DT_CLOCK_REQUEST_IDX_TO_NAME_TOKEN(), but the token is uppercased.
 * This is used by the clock management subsystem to build the
 * CLOCK_MANAGEMENT_STATE_<NAME> identifier for the request.
 *
 * @param node_id Node identifier
 * @param idx Clock request index
 * @return Clock request name as an uppercased token
 */
#define DT_CLOCK_REQUEST_IDX_TO_NAME_UPPER_TOKEN(node_id, idx) \
	DT_CAT4(node_id, _CLOCK_REQUEST_IDX_, idx, _UPPER_TOKEN)

/**
 * @brief Calls @p fn for every clock domain in a clock-request-<idx> property
 *
 * Calls @p fn for every clock domain affected by a clock-request-<idx>
 * property, with the arguments being the clock domain node identifier and
 * the allowed states for that domain. Each entry in the clock-request-<idx>
 * property is a clock domain phandle followed by a bitmask selecting the
 * domain's clock-state children in declaration order. Entries that reference
 * the same clock domain are merged into one bitmask. The bitmask passed to
 * @p fn is translated so that bit n refers to the n-th entry of
 * DT_CLOCK_STATE_SORTED_FOREACH() for that domain, i.e. the n-th used state
 * in rank order, which is the indexing the clock framework uses at runtime.
 * The @p fn macro should take two arguments:
 * 1. The clock domain node identifier
 * 2. The bitmask of allowed states for that clock domain
 * @param node_id Node identifier with the clock-request-<idx> property
 * @param idx Clock request index
 * @param fn Macro to call for each clock domain
 */
#define DT_CLOCK_REQUEST_FOREACH_BY_IDX(node_id, idx, fn) \
	DT_CAT4(node_id, _CLOCK_REQUEST_, idx, _FOREACH)(fn)

/**
 * @brief Get the number of clock domains in a clock-request-<idx> property
 *
 * Gets the number of clock domains affected by a clock-request-<idx>
 * property. Since entries referencing the same clock domain are merged,
 * this may not match the number of entries in the property.
 * @param node_id Node identifier with the clock-request-<idx> property
 * @param idx Clock request index
 */
#define DT_CLOCK_REQUEST_LEN_BY_IDX(node_id, idx) \
	DT_CAT4(node_id, _CLOCK_REQUEST_, idx, _LEN)

/**
 * @brief Get index of clock output name
 * @param node_id Node ID with clock-output-names property
 * @param name Name in the clock-output-names property to get the index of
 */
#define DT_CLOCK_OUTPUT_NAME_IDX(node_id, name) \
	DT_CAT4(node_id, _CLOCK_OUTPUT_NAME_, name, _IDX)

/**
 * @brief Get index of clock request by name
 * @param node_id Node ID with clock-request-names property
 * @param name Lowercase-and-underscores name in the clock-request-names
 *             property to get the index of
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
 * Call @p fn for every clock state of a clock-domain node that is selected
 * by a clock-request-n property. This macro will call @p fn in the order
 * that clock states are ranked, with the lowest ranked state first. States
 * with equal rank keep their declaration order.
 */
#define DT_CLOCK_STATE_SORTED_FOREACH(node_id, fn) \
	CONCAT(node_id, _CLOCK_STATE_SORTED_FOREACH)(fn)

/**
 * @brief Call @p fn for every clock state of an instance that is in use
 *
 * Call @p fn for every clock state of a clock-domain instance that is selected
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
