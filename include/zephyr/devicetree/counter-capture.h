/**
 * @file
 * @brief Counter capture Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_COUNTER_CAPTURE_H_
#define ZEPHYR_INCLUDE_DEVICETREE_COUNTER_CAPTURE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-counter-captures Devicetree Counter Captures API
 * @ingroup devicetree
 * @ingroup counter_interface
 * @{
 */

/**
 * @brief Get the node identifier for the counter controller from a
 *        counter-captures property at an index
 *
 * Example devicetree fragment:
 *
 *     counter1: counter-controller@... { ... };
 *
 *     counter2: counter-controller@... { ... };
 *
 *     n: node {
 *             counter-captures = <&counter1 1 COUNTER_CAPTURE_RISING_EDGE>,
 *                                <&counter2 3 COUNTER_CAPTURE_BOTH_EDGES>;
 *     };
 *
 * Example usage:
 *
 *     DT_COUNTER_CAPTURES_CTLR_BY_IDX(DT_NODELABEL(n), counter_captures, 0)
 *         // DT_NODELABEL(counter1)
 *     DT_COUNTER_CAPTURES_CTLR_BY_IDX(DT_NODELABEL(n), counter_captures, 1)
 *         // DT_NODELABEL(counter2)
 *
 * @param node_id node identifier
 * @param counter_capture_pha lowercase-and-underscores counter capture
 *        property with type "phandle-array"
 * @param idx logical index into @p counter_capture_pha
 * @return the node identifier for the counter controller referenced at
 *         index @p idx
 * @see DT_PROP_BY_PHANDLE_IDX()
 */
#define DT_COUNTER_CAPTURES_CTLR_BY_IDX(node_id, counter_capture_pha, idx) \
	DT_PHANDLE_BY_IDX(node_id, counter_capture_pha, idx)

/**
 * @brief Get the node identifier for the counter controller from a
 *        counter-captures property by name
 *
 * Example devicetree fragment:
 *
 *     counter1: counter-controller@... { ... };
 *
 *     counter2: counter-controller@... { ... };
 *
 *     n: node {
 *             counter-captures = <&counter1 1 COUNTER_CAPTURE_RISING_EDGE>,
 *                                <&counter2 3 COUNTER_CAPTURE_BOTH_EDGES>;
 *             counter-capture-names = "alpha", "beta";
 *     };
 *
 * Example usage:
 *
 *     DT_COUNTER_CAPTURES_CTLR_BY_NAME(DT_NODELABEL(n), counter_captures, alpha)
 *         // DT_NODELABEL(counter1)
 *     DT_COUNTER_CAPTURES_CTLR_BY_NAME(DT_NODELABEL(n), counter_captures, beta)
 *         // DT_NODELABEL(counter2)
 *
 * @param node_id node identifier
 * @param counter_capture_pha lowercase-and-underscores counter capture
 *        property with type "phandle-array"
 * @param name lowercase-and-underscores name of a <tt>counter-captures</tt> element
 *             as defined by the node's <tt>counter-capture-names</tt> property
 * @return the node identifier for the counter controller in the named element
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_COUNTER_CAPTURES_CTLR_BY_NAME(node_id, counter_capture_pha, name) \
	DT_PHANDLE_BY_NAME(node_id, counter_capture_pha, name)

/**
 * @brief Equivalent to DT_COUNTER_CAPTURES_CTLR_BY_IDX(node_id, counter_capture_pha, 0)
 * @param node_id node identifier
 * @param counter_capture_pha lowercase-and-underscores counter capture
 *        property with type "phandle-array"
 * @return the node identifier for the counter controller at index 0
 *         in @p counter_capture_pha
 * @see DT_COUNTER_CAPTURES_CTLR_BY_IDX()
 */
#define DT_COUNTER_CAPTURES_CTLR(node_id, counter_capture_pha) \
	DT_COUNTER_CAPTURES_CTLR_BY_IDX(node_id, counter_capture_pha, 0)

/**
 * @brief Get counter capture specifier's cell value at an index
 *
 * Example devicetree fragment:
 *
 *     counter1: counter-controller@... {
 *             compatible = "vnd,counter-capture";
 *             #counter-capture-cells = <2>;
 *     };
 *
 *     counter2: counter-controller@... {
 *             compatible = "vnd,counter-capture";
 *             #counter-capture-cells = <2>;
 *     };
 *
 *     n: node {
 *             counter-captures = <&counter1 1 COUNTER_CAPTURE_RISING_EDGE>,
 *                                <&counter2 3 COUNTER_CAPTURE_BOTH_EDGES>;
 *     };
 *
 * Bindings fragment for the "vnd,counter-capture" compatible:
 *
 *     counter-capture-cells:
 *       - channel
 *       - flags
 *
 * Example usage:
 *
 *     DT_COUNTER_CAPTURES_CELL_BY_IDX(DT_NODELABEL(n), counter_captures, 0, channel) // 1
 *     DT_COUNTER_CAPTURES_CELL_BY_IDX(DT_NODELABEL(n), counter_captures, 1, channel) // 3
 *     DT_COUNTER_CAPTURES_CELL_BY_IDX(DT_NODELABEL(n), counter_captures, 0, flags)
 *         // COUNTER_CAPTURE_RISING_EDGE
 *     DT_COUNTER_CAPTURES_CELL_BY_IDX(DT_NODELABEL(n), counter_captures, 1, flags)
 *         // COUNTER_CAPTURE_BOTH_EDGES
 *
 * @param node_id node identifier
 * @param counter_capture_pha lowercase-and-underscores counter capture
 *        property with type "phandle-array"
 * @param idx logical index into @p counter_capture_pha
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index @p idx
 * @see DT_PHA_BY_IDX()
 */
#define DT_COUNTER_CAPTURES_CELL_BY_IDX(node_id, counter_capture_pha, idx, cell) \
	DT_PHA_BY_IDX(node_id, counter_capture_pha, idx, cell)

/**
 * @brief Get a counter capture specifier's cell value by name
 *
 * Example devicetree fragment:
 *
 *     counter1: counter-controller@... {
 *             compatible = "vnd,counter-capture";
 *             #counter-capture-cells = <2>;
 *     };
 *
 *     counter2: counter-controller@... {
 *             compatible = "vnd,counter-capture";
 *             #counter-capture-cells = <2>;
 *     };
 *
 *     n: node {
 *             counter-captures = <&counter1 1 COUNTER_CAPTURE_RISING_EDGE>,
 *                                <&counter2 3 COUNTER_CAPTURE_BOTH_EDGES>;
 *             counter-capture-names = "alpha", "beta";
 *     };
 *
 * Bindings fragment for the "vnd,counter-capture" compatible:
 *
 *     counter-capture-cells:
 *       - channel
 *       - flags
 *
 * Example usage:
 *
 *     DT_COUNTER_CAPTURES_CELL_BY_NAME(DT_NODELABEL(n), counter_captures, alpha, channel) // 1
 *     DT_COUNTER_CAPTURES_CELL_BY_NAME(DT_NODELABEL(n), counter_captures, beta, channel)  // 3
 *     DT_COUNTER_CAPTURES_CELL_BY_NAME(DT_NODELABEL(n), counter_captures, alpha, flags)
 *         // COUNTER_CAPTURE_RISING_EDGE
 *     DT_COUNTER_CAPTURES_CELL_BY_NAME(DT_NODELABEL(n), counter_captures, beta, flags)
 *         // COUNTER_CAPTURE_BOTH_EDGES
 *
 * @param node_id node identifier
 * @param counter_capture_pha lowercase-and-underscores counter capture
 *        property with type "phandle-array"
 * @param name lowercase-and-underscores name of a <tt>counter-captures</tt> element
 *             as defined by the node's <tt>counter-capture-names</tt> property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_PHA_BY_NAME()
 */
#define DT_COUNTER_CAPTURES_CELL_BY_NAME(node_id, counter_capture_pha, name, cell) \
	DT_PHA_BY_NAME(node_id, counter_capture_pha, name, cell)

/**
 * @brief Equivalent to
 * DT_COUNTER_CAPTURES_CELL_BY_IDX(node_id, counter_capture_pha, 0, cell)
 * @param node_id node identifier
 * @param counter_capture_pha lowercase-and-underscores counter capture
 *        property with type "phandle-array"
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index 0
 * @see DT_COUNTER_CAPTURES_CELL_BY_IDX()
 */
#define DT_COUNTER_CAPTURES_CELL(node_id, counter_capture_pha, cell) \
	DT_COUNTER_CAPTURES_CELL_BY_IDX(node_id, counter_capture_pha, 0, cell)

/**
 * @brief Get a counter capture specifier's channel cell value at an index
 *
 * This macro only works for counter capture specifiers with cells named
 * "channel". Refer to the node's binding to check if necessary.
 *
 * This is equivalent to:
 *
 *     DT_COUNTER_CAPTURES_CELL_BY_IDX(node_id, counter_capture_pha, idx, channel)
 *
 * @param node_id node identifier
 * @param counter_capture_pha lowercase-and-underscores counter capture
 *        property with type "phandle-array"
 * @param idx logical index into @p counter_capture_pha
 * @return the channel cell value at index @p idx
 * @see DT_COUNTER_CAPTURES_CELL_BY_IDX()
 */
#define DT_COUNTER_CAPTURES_CHANNEL_BY_IDX(node_id, counter_capture_pha, idx) \
	DT_COUNTER_CAPTURES_CELL_BY_IDX(node_id, counter_capture_pha, idx, channel)

/**
 * @brief Get a counter capture specifier's channel cell value by name
 *
 * This macro only works for counter capture specifiers with cells named
 * "channel". Refer to the node's binding to check if necessary.
 *
 * This is equivalent to:
 *
 *     DT_COUNTER_CAPTURES_CELL_BY_NAME(node_id, counter_capture_pha, name, channel)
 *
 * @param node_id node identifier
 * @param counter_capture_pha lowercase-and-underscores counter capture
 *        property with type "phandle-array"
 * @param name lowercase-and-underscores name of a <tt>counter-captures</tt> element
 *             as defined by the node's <tt>counter-capture-names</tt> property
 * @return the channel cell value in the specifier at the named element
 * @see DT_COUNTER_CAPTURES_CELL_BY_NAME()
 */
#define DT_COUNTER_CAPTURES_CHANNEL_BY_NAME(node_id, counter_capture_pha, name) \
	DT_COUNTER_CAPTURES_CELL_BY_NAME(node_id, counter_capture_pha, name, channel)

/**
 * @brief Equivalent to DT_COUNTER_CAPTURES_CHANNEL_BY_IDX(node_id, counter_capture_pha, 0)
 * @param node_id node identifier
 * @param counter_capture_pha lowercase-and-underscores counter capture
 *        property with type "phandle-array"
 * @return the channel cell value at index 0
 * @see DT_COUNTER_CAPTURES_CHANNEL_BY_IDX()
 */
#define DT_COUNTER_CAPTURES_CHANNEL(node_id, counter_capture_pha) \
	DT_COUNTER_CAPTURES_CHANNEL_BY_IDX(node_id, counter_capture_pha, 0)

/**
 * @brief Get a counter capture specifier's flags cell value at an index
 *
 * This macro expects counter capture specifiers with cells named "flags".
 * If there is no "flags" cell in the specifier, zero is returned.
 * Refer to the node's binding to check specifier cell names if necessary.
 *
 * This is equivalent to:
 *
 *     DT_COUNTER_CAPTURES_CELL_BY_IDX(node_id, counter_capture_pha, idx, flags)
 *
 * @param node_id node identifier
 * @param counter_capture_pha lowercase-and-underscores counter capture
 *        property with type "phandle-array"
 * @param idx logical index into @p counter_capture_pha
 * @return the flags cell value at index @p idx, or zero if there is none
 * @see DT_COUNTER_CAPTURES_CELL_BY_IDX()
 */
#define DT_COUNTER_CAPTURES_FLAGS_BY_IDX(node_id, counter_capture_pha, idx) \
	DT_PHA_BY_IDX_OR(node_id, counter_capture_pha, idx, flags, 0)

/**
 * @brief Get a counter capture specifier's flags cell value by name
 *
 * This macro expects counter capture specifiers with cells named "flags".
 * If there is no "flags" cell in the specifier, zero is returned.
 * Refer to the node's binding to check specifier cell names if necessary.
 *
 * This is equivalent to:
 *
 *     DT_COUNTER_CAPTURES_CELL_BY_NAME(node_id, counter_capture_pha, name, flags)
 *
 * if there is a flags cell, but expands to zero if there is none.
 *
 * @param node_id node identifier
 * @param counter_capture_pha lowercase-and-underscores counter capture
 *        property with type "phandle-array"
 * @param name lowercase-and-underscores name of a <tt>counter-captures</tt> element
 *             as defined by the node's <tt>counter-capture-names</tt> property
 * @return the flags cell value in the specifier at the named element,
 *         or zero if there is none
 * @see DT_COUNTER_CAPTURES_CELL_BY_NAME()
 */
#define DT_COUNTER_CAPTURES_FLAGS_BY_NAME(node_id, counter_capture_pha, name) \
	DT_PHA_BY_NAME_OR(node_id, counter_capture_pha, name, flags, 0)

/**
 * @brief Equivalent to
 * DT_COUNTER_CAPTURES_FLAGS_BY_IDX(node_id, counter_capture_pha, 0)
 * @param node_id node identifier
 * @param counter_capture_pha lowercase-and-underscores counter capture
 *        property with type "phandle-array"
 * @return the flags cell value at index 0, or zero if there is none
 * @see DT_COUNTER_CAPTURES_FLAGS_BY_IDX()
 */
#define DT_COUNTER_CAPTURES_FLAGS(node_id, counter_capture_pha) \
	DT_COUNTER_CAPTURES_FLAGS_BY_IDX(node_id, counter_capture_pha, 0)

/**
 * @brief Get the node identifier for the counter controller from a
 *        DT_DRV_COMPAT instance's counter-captures property at an index
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into <tt>counter-captures</tt> property
 * @return the node identifier for the counter controller referenced at
 *         index @p idx
 * @see DT_COUNTER_CAPTURES_CTLR_BY_IDX()
 */
#define DT_INST_COUNTER_CAPTURES_CTLR_BY_IDX(inst, counter_capture_pha, idx) \
	DT_COUNTER_CAPTURES_CTLR_BY_IDX(DT_DRV_INST(inst), counter_capture_pha, idx)

/**
 * @brief Get the node identifier for the counter controller from a
 *        DT_DRV_COMPAT instance's counter-captures property by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a <tt>counter-captures</tt> element
 *             as defined by the node's <tt>counter-capture-names</tt> property
 * @return the node identifier for the counter controller in the named element
 * @see DT_COUNTER_CAPTURES_CTLR_BY_NAME()
 */
#define DT_INST_COUNTER_CAPTURES_CTLR_BY_NAME(inst, counter_capture_pha, name) \
	DT_COUNTER_CAPTURES_CTLR_BY_NAME(DT_DRV_INST(inst), counter_capture_pha, name)

/**
 * @brief Equivalent to DT_INST_COUNTER_CAPTURES_CTLR_BY_IDX(inst, counter_capture_pha, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the node identifier for the counter controller at index 0
 *         in the instance's <tt>counter-captures</tt> property
 * @see DT_COUNTER_CAPTURES_CTLR_BY_IDX()
 */
#define DT_INST_COUNTER_CAPTURES_CTLR(inst, counter_capture_pha) \
	DT_INST_COUNTER_CAPTURES_CTLR_BY_IDX(inst, counter_capture_pha, 0)

/**
 * @brief Get a DT_DRV_COMPAT instance's counter capture specifier's cell value
 *        at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into <tt>counter-captures</tt> property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index @p idx
 */
#define DT_INST_COUNTER_CAPTURES_CELL_BY_IDX(inst, counter_capture_pha, idx, cell) \
	DT_COUNTER_CAPTURES_CELL_BY_IDX(DT_DRV_INST(inst), counter_capture_pha, idx, cell)

/**
 * @brief Get a DT_DRV_COMPAT instance's counter capture specifier's cell value
 *        by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a <tt>counter-captures</tt> element
 *             as defined by the node's <tt>counter-capture-names</tt> property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_COUNTER_CAPTURES_CELL_BY_NAME()
 */
#define DT_INST_COUNTER_CAPTURES_CELL_BY_NAME(inst, counter_capture_pha, name, cell) \
	DT_COUNTER_CAPTURES_CELL_BY_NAME(DT_DRV_INST(inst), counter_capture_pha, name, cell)

/**
 * @brief Equivalent to DT_INST_COUNTER_CAPTURES_CELL_BY_IDX(inst, counter_capture_pha, 0, cell)
 * @param inst DT_DRV_COMPAT instance number
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index 0
 */
#define DT_INST_COUNTER_CAPTURES_CELL(inst, counter_capture_pha, cell) \
	DT_INST_COUNTER_CAPTURES_CELL_BY_IDX(inst, counter_capture_pha, 0, cell)

/**
 * @brief Equivalent to
 * DT_INST_COUNTER_CAPTURES_CELL_BY_IDX(inst, counter_capture_pha, idx, channel)
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into <tt>counter-captures</tt> property
 * @return the channel cell value at index @p idx
 * @see DT_INST_COUNTER_CAPTURES_CELL_BY_IDX()
 */
#define DT_INST_COUNTER_CAPTURES_CHANNEL_BY_IDX(inst, counter_capture_pha, idx) \
	DT_INST_COUNTER_CAPTURES_CELL_BY_IDX(inst, counter_capture_pha, idx, channel)

/**
 * @brief Equivalent to
 * DT_INST_COUNTER_CAPTURES_CELL_BY_NAME(inst, counter_capture_pha, name, channel)
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a <tt>counter-captures</tt> element
 *             as defined by the node's <tt>counter-capture-names</tt> property
 * @return the channel cell value in the specifier at the named element
 * @see DT_INST_COUNTER_CAPTURES_CELL_BY_NAME()
 */
#define DT_INST_COUNTER_CAPTURES_CHANNEL_BY_NAME(inst, counter_capture_pha, name) \
	DT_INST_COUNTER_CAPTURES_CELL_BY_NAME(inst, counter_capture_pha, name, channel)

/**
 * @brief Equivalent to DT_INST_COUNTER_CAPTURES_CHANNEL_BY_IDX(inst, counter_capture_pha, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the channel cell value at index 0
 * @see DT_INST_COUNTER_CAPTURES_CHANNEL_BY_IDX()
 */
#define DT_INST_COUNTER_CAPTURES_CHANNEL(inst, counter_capture_pha) \
	DT_INST_COUNTER_CAPTURES_CHANNEL_BY_IDX(inst, counter_capture_pha, 0)

/**
 * @brief Equivalent to DT_INST_COUNTER_CAPTURES_CELL_BY_IDX(inst, counter_capture_pha, idx, flags)
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into <tt>counter-captures</tt> property
 * @return the flags cell value at index @p idx, or zero if there is none
 * @see DT_INST_COUNTER_CAPTURES_CELL_BY_IDX()
 */
#define DT_INST_COUNTER_CAPTURES_FLAGS_BY_IDX(inst, counter_capture_pha, idx) \
	DT_INST_COUNTER_CAPTURES_CELL_BY_IDX(inst, counter_capture_pha, idx, flags)

/**
 * @brief Equivalent to
 * DT_INST_COUNTER_CAPTURES_CELL_BY_NAME(inst, counter_capture_pha, name, flags)
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a <tt>counter-captures</tt> element
 *             as defined by the node's <tt>counter-capture-names</tt> property
 * @return the flags cell value in the specifier at the named element,
 *         or zero if there is none
 * @see DT_INST_COUNTER_CAPTURES_CELL_BY_NAME()
 */
#define DT_INST_COUNTER_CAPTURES_FLAGS_BY_NAME(inst, counter_capture_pha, name) \
	DT_INST_COUNTER_CAPTURES_CELL_BY_NAME(inst, counter_capture_pha, name, flags)

/**
 * @brief Equivalent to DT_INST_COUNTER_CAPTURES_FLAGS_BY_IDX(inst, counter_capture_pha, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the flags cell value at index 0, or zero if there is none
 * @see DT_INST_COUNTER_CAPTURES_FLAGS_BY_IDX()
 */
#define DT_INST_COUNTER_CAPTURES_FLAGS(inst, counter_capture_pha) \
	DT_INST_COUNTER_CAPTURES_FLAGS_BY_IDX(inst, counter_capture_pha, 0)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_COUNTER_CAPTURE_H_ */
