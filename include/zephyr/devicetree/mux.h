/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MUX Devicetree macro public API header file.
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_MUX_H_
#define ZEPHYR_INCLUDE_DEVICETREE_MUX_H_

#include <zephyr/devicetree.h>

/**
 * @brief Get the mux property string for a node
 *
 * This returns "mux-states" if the node has a mux-states property,
 * otherwise returns "mux-controls".
 *
 * @param node_id node identifier
 * @return mux-states or mux-controls property name
 */
#define DT_MUX_PROP_STRING(node_id) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, mux_states), (mux_states), (mux_controls))

/**
 * @brief Get the node identifier for the MUX controller from a
 *        mux phandle-array property at an index
 *
 * Example devicetree fragment:
 *
 *     mux1: mux@... { };
 *
 *     mux2: mux@... { };
 *
 *     n: node {
 *             mux-controls = <&mux1 0>,
 *                            <&mux2 1>;
 *     };
 *
 * Example usage:
 *
 *     DT_MUX_CTLR_BY_IDX(DT_NODELABEL(n), 1) // DT_NODELABEL(mux2)
 *
 * @param node_id node identifier
 * @param idx logical index into the mux phandle-array property
 * @return the node identifier for the mux controller referenced at
 *         index "idx"
 * @see DT_PHANDLE_BY_IDX()
 */
#define DT_MUX_CTLR_BY_IDX(node_id, idx) \
	DT_PHANDLE_BY_IDX(node_id, DT_MUX_PROP_STRING(node_id), idx)

/**
 * @brief Get the node identifier for the MUX controller from a
 *        mux phandle-array property at index 0
 *
 * Example devicetree fragment:
 *
 *     mux1: mux@... { };
 *
 *     n: node {
 *             mux-controls = <&mux1 0>;
 *     };
 *
 * Example usage:
 *
 *     DT_MUX_CTLR(DT_NODELABEL(n)) // DT_NODELABEL(mux1)
 *
 * @param node_id node identifier
 * @return the node identifier for the mux controller at index 0
 * @see DT_MUX_CTLR_BY_IDX()
 */
#define DT_MUX_CTLR(node_id) \
	DT_MUX_CTLR_BY_IDX(node_id, 0)

/**
 * @brief Get the node identifier for the MUX controller from a
 *        mux phandle-array property by name
 *
 * Example devicetree fragment:
 *
 *     mux1: mux@... { };
 *
 *     mux2: mux@... { };
 *
 *     n: node {
 *             mux-controls = <&mux1 0>, <&mux2 1>;
 *             mux-control-names = "mux-a", "mux-b";
 *     };
 *
 * Example usage:
 *
 *     DT_MUX_CTLR_BY_NAME(DT_NODELABEL(n), mux_b) // DT_NODELABEL(mux2)
 *
 * @param node_id node identifier
 * @param name lowercase-and-underscores name of the mux controller
 *        as defined in the mux-control-names property
 * @return the node identifier for the mux controller referenced by name
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_MUX_CTLR_BY_NAME(node_id, name)						\
	DT_MUX_CTLR_BY_IDX(node_id,							\
			   DT_PHA_ELEM_IDX_BY_NAME(node_id,				\
						   DT_MUX_PROP_STRING(node_id),		\
						   name))

#endif /* ZEPHYR_INCLUDE_DEVICETREE_MUX_H_ */
