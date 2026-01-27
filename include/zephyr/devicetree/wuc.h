/**
 * @file
 * @brief Wakeup Controller Devicetree macro public API header file.
 */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_WUC_H_
#define ZEPHYR_INCLUDE_DEVICETREE_WUC_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-wakeup-controller Devicetree WUC Controller API
 * @ingroup devicetree
 * @ingroup wuc_interface
 * @{
 */

/**
 * @brief Get the node identifier for the controller phandle from a
 *        "wakeup-ctrls" phandle-array property at an index
 *
 * Example devicetree fragment:
 *
 *     wuc1: wakeup-controller@... { ... };
 *
 *     wuc2: wakeup-controller@... { ... };
 *
 *     n: node {
 *             wakeup-ctrls = <&wuc1 10>, <&wuc2 20>;
 *     };
 *
 * Example usage:
 *
 *     DT_WUC_BY_IDX(DT_NODELABEL(n), 0) // DT_NODELABEL(wuc1)
 *     DT_WUC_BY_IDX(DT_NODELABEL(n), 1) // DT_NODELABEL(wuc2)
 *
 * @param node_id node identifier
 * @param idx logical index into "wakeup-ctrls"
 * @return the node identifier for the wakeup controller referenced at
 *         index "idx"
 * @see DT_PHANDLE_BY_IDX()
 */
#define DT_WUC_BY_IDX(node_id, idx) DT_PHANDLE_BY_IDX(node_id, wakeup_ctrls, idx)

/**
 * @brief Equivalent to DT_WUC_BY_IDX(node_id, 0)
 * @param node_id node identifier
 * @return a node identifier for the wakeup controller at index 0
 *         in "wakeup-ctrls"
 * @see DT_WUC_BY_IDX()
 */
#define DT_WUC(node_id) DT_WUC_BY_IDX(node_id, 0)

/**
 * @brief Get a wakeup controller specifier's cell value at an index
 *
 * Example devicetree fragment:
 *
 *     wuc: wakeup-controller@... {
 *             compatible = "vnd,wuc";
 *             #wakeup-cells = <1>;
 *     };
 *
 *     n: node {
 *             wakeup-ctrls = <&wuc 10>;
 *     };
 *
 * Bindings fragment for the vnd,wuc compatible:
 *
 *     wakeup-cells:
 *       - id
 *
 * Example usage:
 *
 *     DT_WUC_CELL_BY_IDX(DT_NODELABEL(n), 0, id) // 10
 *
 * @param node_id node identifier for a node with a wakeup-ctrls property
 * @param idx logical index into wakeup-ctrls property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
 * @see DT_PHA_BY_IDX()
 */
#define DT_WUC_CELL_BY_IDX(node_id, idx, cell) DT_PHA_BY_IDX(node_id, wakeup_ctrls, idx, cell)

/**
 * @brief Equivalent to DT_WUC_CELL_BY_IDX(node_id, 0, cell)
 * @param node_id node identifier for a node with a wakeup-ctrls property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index 0
 * @see DT_WUC_CELL_BY_IDX()
 */
#define DT_WUC_CELL(node_id, cell) DT_WUC_CELL_BY_IDX(node_id, 0, cell)

/**
 * @brief Get the node identifier for the controller phandle from a
 *        "wakeup-ctrls" phandle-array property at an index
 *
 * @param inst instance number
 * @param idx logical index into "wakeup-ctrls"
 * @return the node identifier for the wakeup controller referenced at
 *         index "idx"
 * @see DT_WUC_BY_IDX()
 */
#define DT_INST_WUC_BY_IDX(inst, idx) DT_WUC_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Equivalent to DT_INST_WUC_BY_IDX(inst, 0)
 * @param inst instance number
 * @return a node identifier for the wakeup controller at index 0
 *         in "wakeup-ctrls"
 * @see DT_WUC_BY_IDX()
 */
#define DT_INST_WUC(inst) DT_INST_WUC_BY_IDX(inst, 0)

/**
 * @brief Get a DT_DRV_COMPAT instance's wakeup controller specifier's cell value
 *        at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into wakeup-ctrls property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
 * @see DT_WUC_CELL_BY_IDX()
 */
#define DT_INST_WUC_CELL_BY_IDX(inst, idx, cell) DT_WUC_CELL_BY_IDX(DT_DRV_INST(inst), idx, cell)

/**
 * @brief Equivalent to DT_INST_WUC_CELL_BY_IDX(inst, 0, cell)
 * @param inst DT_DRV_COMPAT instance number
 * @param cell lowercase-and-underscores cell name
 * @return the value of the cell inside the specifier at index 0
 */
#define DT_INST_WUC_CELL(inst, cell) DT_INST_WUC_CELL_BY_IDX(inst, 0, cell)

/**
 * @brief Get a Wakeup Controller specifier's id cell at an index
 *
 * This macro only works for Wakeup Controller specifiers with cells named "id".
 * Refer to the node's binding to check if necessary.
 *
 * Example devicetree fragment:
 *
 *     wuc: wakeup-controller@... {
 *             compatible = "vnd,wuc";
 *             #wakeup-cells = <1>;
 *     };
 *
 *     n: node {
 *             wakeup-ctrls = <&wuc 10>;
 *     };
 *
 * Bindings fragment for the vnd,wuc compatible:
 *
 *     wakeup-cells:
 *       - id
 *
 * Example usage:
 *
 *     DT_WUC_ID_BY_IDX(DT_NODELABEL(n), 0) // 10
 *
 * @param node_id node identifier
 * @param idx logical index into "wakeup-ctrls"
 * @return the id cell value at index "idx"
 * @see DT_PHA_BY_IDX()
 */
#define DT_WUC_ID_BY_IDX(node_id, idx) DT_PHA_BY_IDX(node_id, wakeup_ctrls, idx, id)

/**
 * @brief Equivalent to DT_WUC_ID_BY_IDX(node_id, 0)
 * @param node_id node identifier
 * @return the id cell value at index 0
 * @see DT_WUC_ID_BY_IDX()
 */
#define DT_WUC_ID(node_id) DT_WUC_ID_BY_IDX(node_id, 0)

/**
 * @brief Get a DT_DRV_COMPAT instance's Wakeup Controller specifier's id cell value
 *        at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into "wakeup-ctrls"
 * @return the id cell value at index "idx"
 * @see DT_WUC_ID_BY_IDX()
 */
#define DT_INST_WUC_ID_BY_IDX(inst, idx) DT_WUC_ID_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Equivalent to DT_INST_WUC_ID_BY_IDX(inst, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the id cell value at index 0
 * @see DT_INST_WUC_ID_BY_IDX()
 */
#define DT_INST_WUC_ID(inst) DT_INST_WUC_ID_BY_IDX(inst, 0)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_WUC_H_ */
