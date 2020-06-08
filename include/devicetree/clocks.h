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
 * @{
 */

/**
 * @brief Get a label property from the node referenced by a pwms
 *        property at an index
 *
 * It's an error if the clock controller node referenced by the
 * phandle in node_id's clocks property at index "idx" has no label
 * property.
 *
 * Example devicetree fragment:
 *
 *     clk1: clock-controller@... {
 *             label = "CLK_1";
 *     };
 *
 *     clk2: clock-controller@... {
 *             label = "CLK_2";
 *     };
 *
 *     n: node {
 *             clocks = <&clk1 10 20>, <&clk2 30 40>;
 *     };
 *
 * Example usage:
 *
 *     DT_CLOCKS_LABEL_BY_IDX(DT_NODELABEL(n), 1) // "CLK_2"
 *
 * @param node_id node identifier for a node with a clocks property
 * @param idx logical index into clocks property
 * @return the label property of the node referenced at index "idx"
 * @see DT_PROP_BY_PHANDLE_IDX()
 */
#define DT_CLOCKS_LABEL_BY_IDX(node_id, idx) \
	DT_PROP_BY_PHANDLE_IDX(node_id, clocks, idx, label)

/**
 * @brief Get a label property from a clocks property by name
 *
 * It's an error if the clock controller node referenced by the
 * phandle in node_id's clocks property at the element named "name"
 * has no label property.
 *
 * Example devicetree fragment:
 *
 *     clk1: clock-controller@... {
 *             label = "CLK_1";
 *     };
 *
 *     clk2: clock-controller@... {
 *             label = "CLK_2";
 *     };
 *
 *     n: node {
 *             clocks = <&clk1 10 20>, <&clk2 30 40>;
 *             clock-names = "alpha", "beta";
 *     };
 *
 * Example usage:
 *
 *     DT_CLOCKS_LABEL_BY_NAME(DT_NODELABEL(n), beta) // "CLK_2"
 *
 * @param node_id node identifier for a node with a clocks property
 * @param name lowercase-and-underscores name of a clocks element
 *             as defined by the node's clock-names property
 * @return the label property of the node referenced at the named element
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_CLOCKS_LABEL_BY_NAME(node_id, name) \
	DT_PROP(DT_PHANDLE_BY_NAME(node_id, clocks, name), label)

/**
 * @brief Equivalent to DT_CLOCKS_LABEL_BY_IDX(node_id, 0)
 * @param node_id node identifier for a node with a clocks property
 * @return the label property of the node referenced at index 0
 * @see DT_CLOCKS_LABEL_BY_IDX()
 */
#define DT_CLOCKS_LABEL(node_id) DT_CLOCKS_LABEL_BY_IDX(node_id, 0)

/**
 * @brief Get a clock specifier's cell value at an index
 *
 * Example devicetree fragment:
 *
 *     clk1: clock-controller@... {
 *             compatible = "vnd,clock";
 *             #clock-cells = < 2 >;
 *     };
 *
 *     n: node {
 *             clocks = < &clk1 10 20 >, < &clk1 30 40 >;
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
 * @param node_id node identifier for a node with a clocks property
 * @param idx logical index into clocks property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
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
 *             #clock-cells = < 2 >;
 *     };
 *
 *     n: node {
 *             clocks = < &clk1 10 20 >, < &clk1 30 40 >;
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
 * @param node_id node identifier for a node with a clocks property
 * @param name lowercase-and-underscores name of a clocks element
 *             as defined by the node's clock-names property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_PHA_BY_NAME()
 */
#define DT_CLOCKS_CELL_BY_NAME(node_id, name, cell) \
	DT_PHA_BY_NAME(node_id, clocks, name, cell)

/**
 * @brief Equivalent to DT_CLOCKS_CELL_BY_IDX(node_id, 0, cell)
 * @param node_id node identifier for a node with a clocks property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index 0
 * @see DT_CLOCKS_CELL_BY_IDX()
 */
#define DT_CLOCKS_CELL(node_id, cell) DT_CLOCKS_CELL_BY_IDX(node_id, 0, cell)

/**
 * @brief Get a label property from a DT_DRV_COMPAT instance's clocks
 *        property at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into clocks property
 * @return the label property of the node referenced at index "idx"
 * @see DT_CLOCKS_LABEL_BY_IDX()
 */
#define DT_INST_CLOCKS_LABEL_BY_IDX(inst, idx) \
	DT_CLOCKS_LABEL_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get a label property from a DT_DRV_COMPAT instance's clocks
 *        property by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a clocks element
 *             as defined by the node's clock-names property
 * @return the label property of the node referenced at the named element
 * @see DT_CLOCKS_LABEL_BY_NAME()
 */
#define DT_INST_CLOCKS_LABEL_BY_NAME(inst, name) \
	DT_CLOCKS_LABEL_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Equivalent to DT_INST_CLOCKS_LABEL_BY_IDX(inst, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the label property of the node referenced at index 0
 * @see DT_CLOCKS_LABEL_BY_IDX()
 */
#define DT_INST_CLOCKS_LABEL(inst) DT_INST_CLOCKS_LABEL_BY_IDX(inst, 0)

/**
 * @brief Get a DT_DRV_COMPAT instance's clock specifier's cell value
 *        at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into clocks property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
 * @see DT_CLOCKS_CELL_BY_IDX()
 */
#define DT_INST_CLOCKS_CELL_BY_IDX(inst, idx, cell) \
	DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(inst), idx, cell)

/**
 * @brief Get a DT_DRV_COMPAT instance's clock specifier's cell value by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a clocks element
 *             as defined by the node's clock-names property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_CLOCKS_CELL_BY_NAME()
 */
#define DT_INST_CLOCKS_CELL_BY_NAME(inst, name, cell) \
	DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(inst), name, cell)

/**
 * @brief Equivalent to DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, cell)
 * @param inst DT_DRV_COMPAT instance number
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
