/**
 * @file
 * @brief DMA Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2020, Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_DMAS_H_
#define ZEPHYR_INCLUDE_DEVICETREE_DMAS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-dmas Devicetree DMA API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get the node identifier for the DMA controller from a
 *        dmas property at an index
 *
 * Example devicetree fragment:
 *
 *     dma1: dma@... { ... };
 *
 *     dma2: dma@... { ... };
 *
 *     n: node {
 *		dmas = <&dma1 1 2 0x400 0x3>,
 *			<&dma2 6 3 0x404 0x5>;
 *     };
 *
 * Example usage:
 *
 *     DT_DMAS_CTLR_BY_IDX(DT_NODELABEL(n), 0) // DT_NODELABEL(dma1)
 *     DT_DMAS_CTLR_BY_IDX(DT_NODELABEL(n), 1) // DT_NODELABEL(dma2)
 *
 * @param node_id node identifier for a node with a dmas property
 * @param idx logical index into dmas property
 * @return the node identifier for the DMA controller referenced at
 *         index "idx"
 * @see DT_PROP_BY_PHANDLE_IDX()
 */
#define DT_DMAS_CTLR_BY_IDX(node_id, idx) DT_PHANDLE_BY_IDX(node_id, dmas, idx)

/**
 * @brief Get the node identifier for the DMA controller from a
 *        dmas property by name
 *
 * Example devicetree fragment:
 *
 *     dma1: dma@... { ... };
 *
 *     dma2: dma@... { ... };
 *
 *     n: node {
 *		dmas = <&dma1 1 2 0x400 0x3>,
 *			<&dma2 6 3 0x404 0x5>;
 *		dma-names = "tx", "rx";
 *     };
 *
 * Example usage:
 *
 *     DT_DMAS_CTLR_BY_NAME(DT_NODELABEL(n), tx) // DT_NODELABEL(dma1)
 *     DT_DMAS_CTLR_BY_NAME(DT_NODELABEL(n), rx) // DT_NODELABEL(dma2)
 *
 * @param node_id node identifier for a node with a dmas property
 * @param name lowercase-and-underscores name of a dmas element
 *             as defined by the node's dma-names property
 * @return the node identifier for the DMA controller in the named element
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_DMAS_CTLR_BY_NAME(node_id, name) \
	DT_PHANDLE_BY_NAME(node_id, dmas, name)

/**
 * @brief Equivalent to DT_DMAS_CTLR_BY_IDX(node_id, 0)
 * @param node_id node identifier for a node with a dmas property
 * @return the node identifier for the DMA controller at index 0
 *         in the node's "dmas" property
 * @see DT_DMAS_CTLR_BY_IDX()
 */
#define DT_DMAS_CTLR(node_id) DT_DMAS_CTLR_BY_IDX(node_id, 0)

/**
 * @brief Get the node identifier for the DMA controller from a
 *        DT_DRV_COMPAT instance's dmas property at an index
 *
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into dmas property
 * @return the node identifier for the DMA controller referenced at
 *         index "idx"
 * @see DT_DMAS_CTLR_BY_IDX()
 */
#define DT_INST_DMAS_CTLR_BY_IDX(inst, idx) \
	DT_DMAS_CTLR_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get the node identifier for the DMA controller from a
 *        DT_DRV_COMPAT instance's dmas property by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a dmas element
 *             as defined by the node's dma-names property
 * @return the node identifier for the DMA controller in the named element
 * @see DT_DMAS_CTLR_BY_NAME()
 */
#define DT_INST_DMAS_CTLR_BY_NAME(inst, name) \
	DT_DMAS_CTLR_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Equivalent to DT_INST_DMAS_CTLR_BY_IDX(inst, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the node identifier for the DMA controller at index 0
 *         in the instance's "dmas" property
 * @see DT_DMAS_CTLR_BY_IDX()
 */
#define DT_INST_DMAS_CTLR(inst) DT_INST_DMAS_CTLR_BY_IDX(inst, 0)

/**
 * @brief Get a DMA specifier's cell value at an index
 *
 * Example devicetree fragment:
 *
 *     dma1: dma@... {
 *             compatible = "vnd,dma";
 *             #dma-cells = <2>;
 *     };
 *
 *     dma2: dma@... {
 *             compatible = "vnd,dma";
 *             #dma-cells = <2>;
 *     };
 *
 *     n: node {
 *		dmas = <&dma1 1 0x400>,
 *		       <&dma2 6 0x404>;
 *     };
 *
 * Bindings fragment for the vnd,dma compatible:
 *
 *     dma-cells:
 *       - channel
 *       - config
 *
 * Example usage:
 *
 *     DT_DMAS_CELL_BY_IDX(DT_NODELABEL(n), 0, channel) // 1
 *     DT_DMAS_CELL_BY_IDX(DT_NODELABEL(n), 1, channel) // 6
 *     DT_DMAS_CELL_BY_IDX(DT_NODELABEL(n), 0, config) // 0x400
 *     DT_DMAS_CELL_BY_IDX(DT_NODELABEL(n), 1, config) // 0x404
 *
 * @param node_id node identifier for a node with a dmas property
 * @param idx logical index into dmas property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
 * @see DT_PHA_BY_IDX()
 */
#define DT_DMAS_CELL_BY_IDX(node_id, idx, cell) \
	DT_PHA_BY_IDX(node_id, dmas, idx, cell)

/**
 * @brief Get a DT_DRV_COMPAT instance's DMA specifier's cell value at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into dmas property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
 * @see DT_DMAS_CELL_BY_IDX()
 */
#define DT_INST_DMAS_CELL_BY_IDX(inst, idx, cell) \
	DT_PHA_BY_IDX(DT_DRV_INST(inst), dmas, idx, cell)

/**
 * @brief Get a DMA specifier's cell value by name
 *
 * Example devicetree fragment:
 *
 *     dma1: dma@... {
 *             compatible = "vnd,dma";
 *             #dma-cells = <2>;
 *     };
 *
 *     dma2: dma@... {
 *             compatible = "vnd,dma";
 *             #dma-cells = <2>;
 *     };
 *
 *     n: node {
 *		dmas = <&dma1 1 0x400>,
 *		       <&dma2 6 0x404>;
 *		dma-names = "tx", "rx";
 *     };
 *
 * Bindings fragment for the vnd,dma compatible:
 *
 *     dma-cells:
 *       - channel
 *       - config
 *
 * Example usage:
 *
 *     DT_DMAS_CELL_BY_NAME(DT_NODELABEL(n), tx, channel) // 1
 *     DT_DMAS_CELL_BY_NAME(DT_NODELABEL(n), rx, channel) // 6
 *     DT_DMAS_CELL_BY_NAME(DT_NODELABEL(n), tx, config) // 0x400
 *     DT_DMAS_CELL_BY_NAME(DT_NODELABEL(n), rx, config) // 0x404
 *
 * @param node_id node identifier for a node with a dmas property
 * @param name lowercase-and-underscores name of a dmas element
 *             as defined by the node's dma-names property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_PHA_BY_NAME()
 */
#define DT_DMAS_CELL_BY_NAME(node_id, name, cell) \
	DT_PHA_BY_NAME(node_id, dmas, name, cell)

/**
 * @brief Like DT_DMAS_CELL_BY_NAME(), but with a fallback to @p default_value
 *
 * If the value exists, this expands to DT_DMAS_CELL_BY_NAME(node_id,
 * name, cell). The @p default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to @p default_value.
 *
 * @param node_id node identifier for a node with a dmas property
 * @param name lowercase-and-underscores name of a dmas element
 *             as defined by the node's dma-names property
 * @param cell lowercase-and-underscores cell name
 * @param default_value a fallback value to expand to
 * @return the cell's value or @p default_value
 * @see DT_PHA_BY_NAME_OR()
 */
#define DT_DMAS_CELL_BY_NAME_OR(node_id, name, cell, default_value) \
	DT_PHA_BY_NAME_OR(node_id, dmas, name, cell, default_value)

/**
 * @brief Get a DT_DRV_COMPAT instance's DMA specifier's cell value by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a dmas element
 *             as defined by the node's dma-names property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_DMAS_CELL_BY_NAME()
 */
#define DT_INST_DMAS_CELL_BY_NAME(inst, name, cell) \
	DT_DMAS_CELL_BY_NAME(DT_DRV_INST(inst), name, cell)

/**
 * @brief Is index "idx" valid for a dmas property?
 * @param node_id node identifier for a node with a dmas property
 * @param idx logical index into dmas property
 * @return 1 if the "dmas" property has index "idx", 0 otherwise
 */
#define DT_DMAS_HAS_IDX(node_id, idx) \
	IS_ENABLED(DT_CAT4(node_id, _P_dmas_IDX_, idx, _EXISTS))

/**
 * @brief Is index "idx" valid for a DT_DRV_COMPAT instance's dmas property?
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into dmas property
 * @return 1 if the "dmas" property has a specifier at index "idx", 0 otherwise
 */
#define DT_INST_DMAS_HAS_IDX(inst, idx) \
	DT_DMAS_HAS_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Does a dmas property have a named element?
 * @param node_id node identifier for a node with a dmas property
 * @param name lowercase-and-underscores name of a dmas element
 *             as defined by the node's dma-names property
 * @return 1 if the dmas property has the named element, 0 otherwise
 */
#define DT_DMAS_HAS_NAME(node_id, name) \
	DT_PROP_HAS_NAME(node_id, dmas, name)

/**
 * @brief Does a DT_DRV_COMPAT instance's dmas property have a named element?
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a dmas element
 *             as defined by the node's dma-names property
 * @return 1 if the dmas property has the named element, 0 otherwise
 */
#define DT_INST_DMAS_HAS_NAME(inst, name) \
	DT_DMAS_HAS_NAME(DT_DRV_INST(inst), name)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_DMAS_H_ */
