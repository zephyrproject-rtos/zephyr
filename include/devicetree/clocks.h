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
 * @brief Get clock controller "name" (label property) at an index
 *
 * It's an error if the clock controller referenced by the phandle
 * in property "clocks" at index "idx" has no label property.
 *
 * Example devicetree fragment:
 *
 *     clk1: clkctrl@... {
 *             label = "CLK_1";
 *     };
 *
 *     clk2: clkctrl@... {
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
 * @param node_id node identifier
 * @param idx logical index into the property
 * @return the label property for the referenced node at index idx
 * @see DT_PROP_BY_PHANDLE_IDX()
 */
#define DT_CLOCKS_LABEL_BY_IDX(node_id, idx) \
	DT_PROP_BY_PHANDLE_IDX(node_id, clocks, idx, label)

/**
 * @brief Equivalent to DT_CLOCKS_LABEL_BY_IDX(node_id, 0)
 * @param node_id node identifier
 * @return the label property for the named specifier at index 0
 * @see DT_CLOCKS_LABEL_BY_IDX()
 */
#define DT_CLOCKS_LABEL(node_id) DT_CLOCKS_LABEL_BY_IDX(node_id, 0)

/**
 * @brief Get Clock controller "cell" value at an index
 *
 * Example devicetree fragment:
 *
 *     clk1: clkctrl@... {
 *             label = "CLK_1";
 *             #clock-cells = < 2 >;
 *     };
 *
 *     n: node {
 *             clocks = < &clk1 10 20 >, < &clk1 30 40 >;
 *     };
 *
 * Bindings fragment for the gpio0 node:
 *
 *     clock-cells:
 *       - bus
 *       - bits
 *
 * Example usage:
 *
 *     DT_CLOCKS_CELL_BY_IDX(DT_NODELABEL(n), bus, 0) // 10
 *     DT_CLOCKS_CELL_BY_IDX(DT_NODELABEL(n), bits, 1) // 40
 *
 * @param node_id node identifier
 * @param cell binding's cell name within the specifier at index "idx"
 * @param idx logical index into the property
 * @return the value of the cell inside the specifier at index "idx"
 * @see DT_PHA_PHANDLE_IDX()
 */
#define DT_CLOCKS_CELL_BY_IDX(node_id, cell, idx) \
	DT_PHA_BY_IDX(node_id, clocks, idx, cell)

/**
 * @brief Equivalent to DT_CLOCKS_CELL_BY_IDX(node_id, cell, 0)
 * @param node_id node identifier
 * @param cell binding's cell name within the specifier at index 0
 * @return the value of the cell inside the specifier at index 0
 * @see DT_CLOCKS_CELL_BY_IDX()
 */
#define DT_CLOCKS_CELL(node_id, cell) DT_CLOCKS_CELL_BY_IDX(node_id, cell, 0)

/**
 * @brief Get a DT_DRV_COMPAT clock controller "name" at an index
 * (see @ref DT_CLOCKS_LABEL_BY_IDX)
 * @param inst instance number
 * @param idx logical index into the property
 * @return the label property for the named specifier at index idx
 */
#define DT_INST_CLOCKS_LABEL_BY_IDX(inst, idx) \
	DT_CLOCKS_LABEL_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get a DT_DRV_COMPAT clock controller "name"
 * (see @ref DT_CLOCKS_LABEL_BY_IDX)
 * @param inst instance number
 * @return the label property for the named specifier at index 0
 */
#define DT_INST_CLOCKS_LABEL(inst) DT_INST_CLOCKS_LABEL_BY_IDX(inst, 0)

/**
 * @brief Get a DT_DRV_COMPAT clock controller "cell" value at an index
 * @param inst instance number
 * @param cell binding's cell name within the specifier at index "idx"
 * @param idx logical index into the property
 * @return the value of the cell inside the specifier at index "idx"
 */
#define DT_INST_CLOCKS_CELL_BY_IDX(inst, cell, idx) \
	DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(inst), cell, idx)

/**
 * @brief Get a DT_DRV_COMPAT clock controller "cell" value at an index 0
 * @param inst instance number
 * @param cell binding's cell name within the specifier at index 0
 * @return the value of the cell inside the specifier at index 0
 */
#define DT_INST_CLOCKS_CELL(inst, cell) \
	DT_INST_CLOCKS_CELL_BY_IDX(inst, cell, 0)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif  /* ZEPHYR_INCLUDE_DEVICETREE_CLOCKS_H_ */
