/**
 * @file
 * @brief Reset Controller Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2022, Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_RESET_H_
#define ZEPHYR_INCLUDE_DEVICETREE_RESET_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-reset-controller Devicetree Reset Controller API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get the node identifier for the controller phandle from a
 *        "resets" phandle-array property at an index
 *
 * Example devicetree fragment:
 *
 *     reset1: reset-controller@... { ... };
 *
 *     reset2: reset-controller@... { ... };
 *
 *     n: node {
 *             resets = <&reset1 10>, <&reset2 20>;
 *     };
 *
 * Example usage:
 *
 *     DT_RESET_CTLR_BY_IDX(DT_NODELABEL(n), 0)) // DT_NODELABEL(reset1)
 *     DT_RESET_CTLR_BY_IDX(DT_NODELABEL(n), 1)) // DT_NODELABEL(reset2)
 *
 * @param node_id node identifier
 * @param idx logical index into "resets"
 * @return the node identifier for the reset controller referenced at
 *         index "idx"
 * @see DT_PHANDLE_BY_IDX()
 */
#define DT_RESET_CTLR_BY_IDX(node_id, idx) \
	DT_PHANDLE_BY_IDX(node_id, resets, idx)

/**
 * @brief Equivalent to DT_RESET_CTLR_BY_IDX(node_id, 0)
 * @param node_id node identifier
 * @return a node identifier for the reset controller at index 0
 *         in "resets"
 * @see DT_RESET_CTLR_BY_IDX()
 */
#define DT_RESET_CTLR(node_id) \
	DT_RESET_CTLR_BY_IDX(node_id, 0)

/**
 * @brief Get the node identifier for the controller phandle from a
 *        resets phandle-array property by name
 *
 * Example devicetree fragment:
 *
 *     reset1: reset-controller@... { ... };
 *
 *     reset2: reset-controller@... { ... };
 *
 *     n: node {
 *             resets = <&reset1 10>, <&reset2 20>;
 *             reset-names = "alpha", "beta";
 *     };
 *
 * Example usage:
 *
 *     DT_RESET_CTLR_BY_NAME(DT_NODELABEL(n), alpha) // DT_NODELABEL(reset1)
 *     DT_RESET_CTLR_BY_NAME(DT_NODELABEL(n), beta) // DT_NODELABEL(reset2)
 *
 * @param node_id node identifier
 * @param name lowercase-and-underscores name of a resets element
 *             as defined by the node's reset-names property
 * @return the node identifier for the reset controller referenced by name
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_RESET_CTLR_BY_NAME(node_id, name) \
	DT_PHANDLE_BY_NAME(node_id, resets, name)

/**
 * @brief Get a reset specifier's cell value at an index
 *
 * Example devicetree fragment:
 *
 *     reset: reset-controller@... {
 *             compatible = "vnd,reset";
 *             #reset-cells = <1>;
 *     };
 *
 *     n: node {
 *             resets = <&reset 10>;
 *     };
 *
 * Bindings fragment for the vnd,reset compatible:
 *
 *     reset-cells:
 *       - id
 *
 * Example usage:
 *
 *     DT_RESET_CELL_BY_IDX(DT_NODELABEL(n), 0, id) // 10
 *
 * @param node_id node identifier for a node with a resets property
 * @param idx logical index into resets property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
 * @see DT_PHA_BY_IDX()
 */
#define DT_RESET_CELL_BY_IDX(node_id, idx, cell) \
	DT_PHA_BY_IDX(node_id, resets, idx, cell)

/**
 * @brief Get a reset specifier's cell value by name
 *
 * Example devicetree fragment:
 *
 *     reset: reset-controller@... {
 *             compatible = "vnd,reset";
 *             #reset-cells = <1>;
 *     };
 *
 *     n: node {
 *             resets = <&reset 10>;
 *             reset-names = "alpha";
 *     };
 *
 * Bindings fragment for the vnd,reset compatible:
 *
 *     reset-cells:
 *       - id
 *
 * Example usage:
 *
 *     DT_RESET_CELL_BY_NAME(DT_NODELABEL(n), alpha, id) // 10
 *
 * @param node_id node identifier for a node with a resets property
 * @param name lowercase-and-underscores name of a resets element
 *             as defined by the node's reset-names property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_PHA_BY_NAME()
 */
#define DT_RESET_CELL_BY_NAME(node_id, name, cell) \
	DT_PHA_BY_NAME(node_id, resets, name, cell)

/**
 * @brief Equivalent to DT_RESET_CELL_BY_IDX(node_id, 0, cell)
 * @param node_id node identifier for a node with a resets property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index 0
 * @see DT_RESET_CELL_BY_IDX()
 */
#define DT_RESET_CELL(node_id, cell) \
	DT_RESET_CELL_BY_IDX(node_id, 0, cell)

/**
 * @brief Get the node identifier for the controller phandle from a
 *        "resets" phandle-array property at an index
 *
 * @param inst instance number
 * @param idx logical index into "resets"
 * @return the node identifier for the reset controller referenced at
 *         index "idx"
 * @see DT_RESET_CTLR_BY_IDX()
 */
#define DT_INST_RESET_CTLR_BY_IDX(inst, idx) \
	DT_RESET_CTLR_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Equivalent to DT_INST_RESET_CTLR_BY_IDX(inst, 0)
 * @param inst instance number
 * @return a node identifier for the reset controller at index 0
 *         in "resets"
 * @see DT_RESET_CTLR()
 */
#define DT_INST_RESET_CTLR(inst) \
	DT_INST_RESET_CTLR_BY_IDX(inst, 0)

/**
 * @brief Get the node identifier for the controller phandle from a
 *        resets phandle-array property by name
 *
 * @param inst instance number
 * @param name lowercase-and-underscores name of a resets element
 *             as defined by the node's reset-names property
 * @return the node identifier for the reset controller referenced by
 *         the named element
 * @see DT_RESET_CTLR_BY_NAME()
 */
#define DT_INST_RESET_CTLR_BY_NAME(inst, name) \
	DT_RESET_CTLR_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get a DT_DRV_COMPAT instance's reset specifier's cell value
 *        at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into resets property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
 * @see DT_RESET_CELL_BY_IDX()
 */
#define DT_INST_RESET_CELL_BY_IDX(inst, idx, cell) \
	DT_RESET_CELL_BY_IDX(DT_DRV_INST(inst), idx, cell)

/**
 * @brief Get a DT_DRV_COMPAT instance's reset specifier's cell value by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a resets element
 *             as defined by the node's reset-names property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_RESET_CELL_BY_NAME()
 */
#define DT_INST_RESET_CELL_BY_NAME(inst, name, cell) \
	DT_RESET_CELL_BY_NAME(DT_DRV_INST(inst), name, cell)

/**
 * @brief Equivalent to DT_INST_RESET_CELL_BY_IDX(inst, 0, cell)
 * @param inst DT_DRV_COMPAT instance number
 * @param cell lowercase-and-underscores cell name
 * @return the value of the cell inside the specifier at index 0
 */
#define DT_INST_RESET_CELL(inst, cell) \
	DT_INST_RESET_CELL_BY_IDX(inst, 0, cell)

/**
 * @brief Get a Reset Controller specifier's id cell at an index
 *
 * This macro only works for Reset Controller specifiers with cells named "id".
 * Refer to the node's binding to check if necessary.
 *
 * Example devicetree fragment:
 *
 *     reset: reset-controller@... {
 *             compatible = "vnd,reset";
 *             #reset-cells = <1>;
 *     };
 *
 *     n: node {
 *             resets = <&reset 10>;
 *     };
 *
 * Bindings fragment for the vnd,reset compatible:
 *
 *     reset-cells:
 *       - id
 *
 * Example usage:
 *
 *     DT_RESET_ID_BY_IDX(DT_NODELABEL(n), 0) // 10
 *
 * @param node_id node identifier
 * @param idx logical index into "resets"
 * @return the id cell value at index "idx"
 * @see DT_PHA_BY_IDX()
 */
#define DT_RESET_ID_BY_IDX(node_id, idx) \
	DT_PHA_BY_IDX(node_id, resets, idx, id)

/**
 * @brief Equivalent to DT_RESET_ID_BY_IDX(node_id, 0)
 * @param node_id node identifier
 * @return the id cell value at index 0
 * @see DT_RESET_ID_BY_IDX()
 */
#define DT_RESET_ID(node_id) \
	DT_RESET_ID_BY_IDX(node_id, 0)

/**
 * @brief Get a DT_DRV_COMPAT instance's Reset Controller specifier's id cell value
 *        at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into "resets"
 * @return the id cell value at index "idx"
 * @see DT_RESET_ID_BY_IDX()
 */
#define DT_INST_RESET_ID_BY_IDX(inst, idx) \
	DT_RESET_ID_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Equivalent to DT_INST_RESET_ID_BY_IDX(inst, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the id cell value at index 0
 * @see DT_INST_RESET_ID_BY_IDX()
 */
#define DT_INST_RESET_ID(inst) \
	DT_INST_RESET_ID_BY_IDX(inst, 0)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_RESET_H_ */
