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
 * @defgroup devicetree-dmas Devicetree dma API
 * @{
 */

/**
 * @brief Get dma controller "name" (label property) at an index
 *
 * It's an error if the dma controller referenced by the phandle
 * in property "dmas" at index "idx" has no label property.
 *
 * Example devicetree fragment:
 *
 *     dma1: dma@... {
 *             label = "DMA_1";
 *     };
 *
 *     dma2: dma@... {
 *             label = "DMA_2";
 *     };
 *
 *     n: node {
 *		dmas = <&dma1 1 2 0x400 0x3>,
 *			<&dma2 6 3 0x404 0x5>;
 *		dma-names = "tx", "rx";
 *     };
 *
 * Example usage:
 *
 *     DT_DMAS_LABEL_BY_IDX(DT_NODELABEL(n), 1) // "DMA_2"
 *
 * @param node_id node identifier
 * @param idx logical index into the "dmas" property
 * @return the label property for the dma-controller at index idx
 */
#define DT_DMAS_LABEL_BY_IDX(node_id, idx) \
	DT_PROP_BY_PHANDLE_IDX(node_id, dmas, idx, label)

/**
 * @brief Get dmas's controller "name" at an index
 * (see @ref DT_DMAS_LABEL_BY_IDX)
 * @param inst instance number
 * @param idx logical index into the "dmas" property
 * @return the label property for the targeted dma-controller
 */
#define DT_INST_DMAS_LABEL_BY_IDX(inst, idx) \
	DT_DMAS_LABEL_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get dma controller "name" (label property) by dma channel specifier
 * name
 *
 * It's an error if the dma controller referenced by the phandle
 * in property "dmas" referenced as "name" has no label property.
 *
 * Example devicetree fragment:
 *
 *     dma1: dma@... {
 *             label = "DMA_1";
 *     };
 *
 *     dma2: dma@... {
 *             label = "DMA_2";
 *     };
 *
 *     n: node {
 *		dmas = <&dma1 1 2 0x400 0x3>,
 *			<&dma2 6 3 0x404 0x5>;
 *		dma-names = "tx", "rx";
 *     };
 *
 * Example usage:
 *
 *     DT_DMAS_LABEL_BY_NAME(DT_NODELABEL(n), rx) // "DMA_2"
 *
 * @param node_id node identifier
 * @param name lowercase-and-underscores dma channel specifier name
 * @return the label property for the targeted dma-controller
 */
#define DT_DMAS_LABEL_BY_NAME(node_id, name) \
	DT_PROP(DT_PHANDLE_BY_NAME(node_id, dmas, name), label)

/**
 * @brief Get dmas's controller "name" by name
 * (see @ref DT_DMAS_LABEL_BY_NAME)
 * @param inst instance number
 * @param name lowercase-and-underscores "dmas" name
 * @return the label property for the targeted dma-controller
 */
#define DT_INST_DMAS_LABEL_BY_NAME(inst, name) \
	DT_DMAS_LABEL_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get dma channel specifier cell value at an index
 *
 * Example devicetree fragment:
 *
 *     dma1: dma@... {
 *             label = "DMA_1";
 *     };
 *
 *     dma2: dma@... {
 *             label = "DMA_2";
 *     };
 *
 *     n: node {
 *		dmas = <&dma1 1 0x400>,
 *		       <&dma2 6 0x404>;
 *		dma-names = "tx", "rx";
 *     };
 *
 * Bindings fragment for the dma nodes:
 *
 *     dma-cells:
 *       - channel
 *       - config
 *
 * Example usage:
 *
 *     DT_DMAS_CELLS_BY_IDX(DT_NODELABEL(n), channel, 0) // 1
 *     DT_DMAS_CELLS_BY_IDX(DT_NODELABEL(n), config, 1) // 0x404
 *
 * @param node_id node identifier
 * @param cell_name binding's cell name within the specifier at index idx
 * @param idx logical index into the "dmas" property
 * @return the value of "cell" inside the dma channel specifier
 */

#define DT_DMAS_CELLS_BY_IDX(node_id, cell_name, idx) \
	DT_PHA_BY_IDX(node_id, dmas, cell_name, idx)

/**
 * @brief Get dma channel specifier cell value at an index
 * (see @ref DT_DMAS_CELLS_BY_IDX)
 * @param inst instance number
 * @param cell_name binding's cell name within the specifier at index idx
 * @param idx logical index into the "dmas" property
 * @return the value of "cell" inside the dma channel specifier
 */
#define DT_INST_DMAS_CELLS_BY_IDX(inst, cell_name, idx) \
	DT_PHA_BY_IDX(DT_DRV_INST(inst), dmas, cell_name, idx)

/**
 * @brief Get dma channel specifier cell value by name
 *
 * Example devicetree fragment:
 *
 *     dma1: dma@... {
 *             label = "DMA_1";
 *     };
 *
 *     dma2: dma@... {
 *             label = "DMA_2";
 *     };
 *
 *     n: node {
 *		dmas = <&dma1 1 0x400>,
 *		       <&dma2 6 0x404>;
 *		dma-names = "tx", "rx";
 *     };
 *
 * Bindings fragment for the dma nodes:
 *
 *     dma-cells:
 *       - channel
 *       - config
 *
 * Example usage:
 *
 *     DT_DMAS_CELLS_BY_NAME(DT_NODELABEL(n), channel, tx) // 1
 *     DT_DMAS_CELLS_BY_NAME(DT_NODELABEL(n), config, rx) // 0x404
 *
 * @param node_id node identifier
 * @param cell_name binding's cell name within the specifier referenced
 * as "name"
 * @param name lowercase-and-underscores dma channel specifier name
 * @return the value of "cell" inside the dma channel specifier
 * @see DT_PHA_PHANDLE_IDX()
 */

#define DT_DMAS_CELLS_BY_NAME(node_id, cell_name, name) \
	DT_PHA_BY_NAME(node_id, dmas, cell_name, name)

/**
 * @brief Get dma channel specifier cell value by name
 * (see @ref DT_DMAS_CELLS_BY_NAME)
 * @param inst instance number
 * @param cell_name binding's cell name within the specifier referenced
 * as "name"
 * @param name lowercase-and-underscores dma channel specifier name
 * @return the value of "cell" inside the specifier
 */
#define DT_INST_DMAS_CELLS_BY_NAME(inst, cell_name, name) \
	DT_DMAS_CELLS_BY_NAME(DT_DRV_INST(inst), cell_name, name)

/**
 * @brief Does a DMA client have a channel specifier at an index?
 * @param node_id node identifier
 * @param idx logical index into the "dmas" property
 * @return 1 if the "dmas" property has a specifier at index "idx", 0 otherwise
 */

#define DT_DMAS_HAS_IDX(node_id, idx) \
	IS_ENABLED(DT_CAT(node_id, _P_dmas_IDX_##idx##_EXISTS))

/**
 * @brief Does a DMA client have a channel specifier at an index?
 * @param inst instance number
 * @param idx logical index into the "dmas" property
 * @return 1 if the "dmas" property has a specifier at index "idx", 0 otherwise
 */

#define DT_INST_DMAS_HAS_IDX(inst, idx) \
	DT_DMAS_HAS_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Does a DMA client have a DMA specifier with a given name?
 * @param node_id node identifier
 * @param name lowercase-and-underscores dma channel specifier name
 * @return 1 if it has a "name" dma channel specifier, 0 otherwise
 */
#define DT_DMAS_HAS_NAME(node_id, name) \
	IS_ENABLED(DT_CAT(node_id, _P_dmas_NAME_##name##_EXISTS))

/**
 * @brief Does a DMA client have a DMA specifier with a given name?
 * @param inst instance number
 * @param name lowercase-and-underscores dma channel specifier name
 * @return 1 if it has a "name" dma channel specifier, 0 otherwise
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
