/**
 * @file
 * @brief Clocks Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2020, Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_CLOCKS_H_
#define ZEPHYR_INCLUDE_DEVICETREE_CLOCKS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-clocks Devicetree Clocks API
 * @ingroup devicetree
 * @ingroup clock_control_interface
 * @{
 */

/**
 * @brief Test if a node has a <tt>clocks</tt> phandle-array property at a given index
 *
 * This expands to 1 if the given index is valid <tt>clocks</tt> property phandle-array index.
 * Otherwise, it expands to 0.
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             clocks = <...>, <...>;
 *     };
 *
 *     n2: node-2 {
 *             clocks = <...>;
 *     };
 *
 * Example usage:
 *
 *     DT_CLOCKS_HAS_IDX(DT_NODELABEL(n1), 0) // 1
 *     DT_CLOCKS_HAS_IDX(DT_NODELABEL(n1), 1) // 1
 *     DT_CLOCKS_HAS_IDX(DT_NODELABEL(n1), 2) // 0
 *     DT_CLOCKS_HAS_IDX(DT_NODELABEL(n2), 1) // 0
 *
 * @param node_id node identifier; may or may not have any <tt>clocks</tt> property
 * @param idx index of a <tt>clocks</tt> property phandle-array whose existence to check
 * @return 1 if the index exists, 0 otherwise
 */
#define DT_CLOCKS_HAS_IDX(node_id, idx) \
	DT_PROP_HAS_IDX(node_id, clocks, idx)

/**
 * @brief Test if a node has a <tt>clock-names</tt> array property holds a given name
 *
 * This expands to 1 if the name is available as <tt>clock-names</tt> array property cell.
 * Otherwise, it expands to 0.
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             clocks = <...>, <...>;
 *             clock-names = "alpha", "beta";
 *     };
 *
 *     n2: node-2 {
 *             clocks = <...>;
 *             clock-names = "alpha";
 *     };
 *
 * Example usage:
 *
 *     DT_CLOCKS_HAS_NAME(DT_NODELABEL(n1), alpha) // 1
 *     DT_CLOCKS_HAS_NAME(DT_NODELABEL(n1), beta)  // 1
 *     DT_CLOCKS_HAS_NAME(DT_NODELABEL(n2), beta)  // 0
 *
 * @param node_id node identifier; may or may not have any <tt>clock-names</tt> property.
 * @param name lowercase-and-underscores <tt>clock-names</tt> cell value name to check
 * @return 1 if the clock name exists, 0 otherwise
 */
#define DT_CLOCKS_HAS_NAME(node_id, name) \
	DT_PROP_HAS_NAME(node_id, clocks, name)

/**
 * @brief Get the number of elements in a <tt>clocks</tt> property
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             clocks = <&foo>, <&bar>;
 *     };
 *
 *     n2: node-2 {
 *             clocks = <&foo>;
 *     };
 *
 * Example usage:
 *
 *     DT_NUM_CLOCKS(DT_NODELABEL(n1)) // 2
 *     DT_NUM_CLOCKS(DT_NODELABEL(n2)) // 1
 *
 * @param node_id node identifier with a <tt>clocks</tt> property
 * @return number of elements in the property
 */
#define DT_NUM_CLOCKS(node_id) \
	DT_PROP_LEN(node_id, clocks)


/**
 * @brief Get the node identifier for the controller phandle from a
 *        <tt>clocks</tt> phandle-array property at an index
 *
 * Example devicetree fragment:
 *
 *     clk1: clock-controller@... { ... };
 *
 *     clk2: clock-controller@... { ... };
 *
 *     n: node {
 *             clocks = <&clk1 10 20>, <&clk2 30 40>;
 *     };
 *
 * Example usage:
 *
 *     DT_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(n), 0)) // DT_NODELABEL(clk1)
 *     DT_CLOCKS_CTLR_BY_IDX(DT_NODELABEL(n), 1)) // DT_NODELABEL(clk2)
 *
 * @param node_id node identifier
 * @param idx logical index into <tt>clocks</tt> property
 * @return the node identifier for the clock controller referenced at index @p idx
 * @see DT_PHANDLE_BY_IDX()
 */
#define DT_CLOCKS_CTLR_BY_IDX(node_id, idx) \
	DT_PHANDLE_BY_IDX(node_id, clocks, idx)

/**
 * @brief Equivalent to DT_CLOCKS_CTLR_BY_IDX(node_id, 0)
 * @param node_id node identifier
 * @return a node identifier for the clocks controller at index 0 in <tt>clocks</tt>
 * @see DT_CLOCKS_CTLR_BY_IDX()
 */
#define DT_CLOCKS_CTLR(node_id) DT_CLOCKS_CTLR_BY_IDX(node_id, 0)

/**
 * @brief Get the node identifier for the controller phandle from a
 *        <tt>clocks</tt> phandle-array property by name
 *
 * Example devicetree fragment:
 *
 *     clk1: clock-controller@... { ... };
 *
 *     clk2: clock-controller@... { ... };
 *
 *     n: node {
 *             clocks = <&clk1 10 20>, <&clk2 30 40>;
 *             clock-names = "alpha", "beta";
 *     };
 *
 * Example usage:
 *
 *     DT_CLOCKS_CTLR_BY_NAME(DT_NODELABEL(n), beta) // DT_NODELABEL(clk2)
 *
 * @param node_id node identifier
 * @param name lowercase-and-underscores name of a <tt>clocks</tt> element
 *             as defined by the node's <tt>clock-names</tt> property
 * @return the node identifier for the clock controller referenced by @p name
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_CLOCKS_CTLR_BY_NAME(node_id, name) \
	DT_PHANDLE_BY_NAME(node_id, clocks, name)

/**
 * @brief Get a clock specifier's cell value at an index
 *
 * Example devicetree fragment:
 *
 *     clk1: clock-controller@... {
 *             compatible = "vnd,clock";
 *             #clock-cells = <2>;
 *     };
 *
 *     n: node {
 *             clocks = <&clk1 10 20>, <&clk1 30 40>;
 *     };
 *
 * Bindings fragment for the vnd,clock compatible:
 *
 *     clock-cells:
 *       - bus
 *       - bits
 *
 * Example usage:
 *
 *     DT_CLOCKS_CELL_BY_IDX(DT_NODELABEL(n), 0, bus) // 10
 *     DT_CLOCKS_CELL_BY_IDX(DT_NODELABEL(n), 1, bits) // 40
 *
 * @param node_id node identifier for a node with a <tt>clocks</tt> property
 * @param idx logical index into <tt>clocks</tt> property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index @p idx
 * @see DT_PHA_BY_IDX()
 */
#define DT_CLOCKS_CELL_BY_IDX(node_id, idx, cell) \
	DT_PHA_BY_IDX(node_id, clocks, idx, cell)

/**
 * @brief Get a clock specifier's cell value by name
 *
 * Example devicetree fragment:
 *
 *     clk1: clock-controller@... {
 *             compatible = "vnd,clock";
 *             #clock-cells = <2>;
 *     };
 *
 *     n: node {
 *             clocks = <&clk1 10 20>, <&clk1 30 40>;
 *             clock-names = "alpha", "beta";
 *     };
 *
 * Bindings fragment for the vnd,clock compatible:
 *
 *     clock-cells:
 *       - bus
 *       - bits
 *
 * Example usage:
 *
 *     DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(n), alpha, bus) // 10
 *     DT_CLOCKS_CELL_BY_NAME(DT_NODELABEL(n), beta, bits) // 40
 *
 * @param node_id node identifier for a node with a <tt>clocks</tt> property
 * @param name lowercase-and-underscores name of a <tt>clocks</tt> element
 *             as defined by the node's <tt>clock-names</tt> property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_PHA_BY_NAME()
 */
#define DT_CLOCKS_CELL_BY_NAME(node_id, name, cell) \
	DT_PHA_BY_NAME(node_id, clocks, name, cell)

/**
 * @brief Equivalent to DT_CLOCKS_CELL_BY_IDX(node_id, 0, cell)
 * @param node_id node identifier for a node with a <tt>clocks</tt> property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index 0
 * @see DT_CLOCKS_CELL_BY_IDX()
 */
#define DT_CLOCKS_CELL(node_id, cell) DT_CLOCKS_CELL_BY_IDX(node_id, 0, cell)

/**
 * @brief Equivalent to DT_CLOCKS_HAS_IDX(DT_DRV_INST(inst), idx)
 * @param inst @c DT_DRV_COMPAT instance number; may or may not have any <tt>clocks</tt> property
 * @param idx index of a <tt>clocks</tt> property phandle-array whose existence to check
 * @return 1 if the index exists, 0 otherwise
 */
#define DT_INST_CLOCKS_HAS_IDX(inst, idx) \
	DT_CLOCKS_HAS_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Equivalent to DT_CLOCK_HAS_NAME(DT_DRV_INST(inst), name)
 * @param inst @c DT_DRV_COMPAT instance number; may or may not have any <tt>clock-names</tt>
 *             property.
 * @param name lowercase-and-underscores <tt>clock-names</tt> cell value name to check
 * @return 1 if the clock name exists, 0 otherwise
 */
#define DT_INST_CLOCKS_HAS_NAME(inst, name) \
	DT_CLOCKS_HAS_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Equivalent to DT_NUM_CLOCKS(DT_DRV_INST(inst))
 * @param inst instance number
 * @return number of elements in the <tt>clocks</tt> property
 */
#define DT_INST_NUM_CLOCKS(inst) \
	DT_NUM_CLOCKS(DT_DRV_INST(inst))

/**
 * @brief Get the node identifier for the controller phandle from a
 *        <tt>clocks</tt> phandle-array property at an index
 *
 * @param inst instance number
 * @param idx logical index into <tt>clocks</tt> property
 * @return the node identifier for the clock controller referenced at index @p idx
 * @see DT_CLOCKS_CTLR_BY_IDX()
 */
#define DT_INST_CLOCKS_CTLR_BY_IDX(inst, idx) \
	DT_CLOCKS_CTLR_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Equivalent to DT_INST_CLOCKS_CTLR_BY_IDX(inst, 0)
 * @param inst instance number
 * @return a node identifier for the clocks controller at index 0 in <tt>clocks</tt> property
 * @see DT_CLOCKS_CTLR()
 */
#define DT_INST_CLOCKS_CTLR(inst) DT_INST_CLOCKS_CTLR_BY_IDX(inst, 0)

/**
 * @brief Get the node identifier for the controller phandle from a
 *        <tt>clocks</tt> phandle-array property by name
 *
 * @param inst instance number
 * @param name lowercase-and-underscores name of a <tt>clocks</tt> element
 *             as defined by the node's <tt>clock-names</tt> property
 * @return the node identifier for the clock controller referenced by
 *         the named element
 * @see DT_CLOCKS_CTLR_BY_NAME()
 */
#define DT_INST_CLOCKS_CTLR_BY_NAME(inst, name) \
	DT_CLOCKS_CTLR_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get a @c DT_DRV_COMPAT instance's clock specifier's cell value
 *        at an index
 * @param inst @c DT_DRV_COMPAT instance number
 * @param idx logical index into <tt>clocks</tt> property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index @p idx
 * @see DT_CLOCKS_CELL_BY_IDX()
 */
#define DT_INST_CLOCKS_CELL_BY_IDX(inst, idx, cell) \
	DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(inst), idx, cell)

/**
 * @brief Get a @c DT_DRV_COMPAT instance's clock specifier's cell value by name
 * @param inst @c DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a <tt>clocks</tt> element
 *             as defined by the node's <tt>clock-names</tt> property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_CLOCKS_CELL_BY_NAME()
 */
#define DT_INST_CLOCKS_CELL_BY_NAME(inst, name, cell) \
	DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(inst), name, cell)

/**
 * @brief Equivalent to DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, cell)
 * @param inst @c DT_DRV_COMPAT instance number
 * @param cell lowercase-and-underscores cell name
 * @return the value of the cell inside the specifier at index 0
 */
#define DT_INST_CLOCKS_CELL(inst, cell) \
	DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, cell)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_CLOCKS_H_ */
