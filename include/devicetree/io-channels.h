/**
 * @file
 * @brief IO channels devicetree macro public API header file.
 */

/*
 * Copyright (c) 2020, Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_IO_CHANNELS_H_
#define ZEPHYR_INCLUDE_DEVICETREE_IO_CHANNELS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-io-channels Devicetree IO Channels API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get a label property from the node referenced by an
 *        io-channels property at an index
 *
 * It's an error if the node referenced by the phandle in node_id's
 * io-channels property at index "idx" has no label property.
 *
 * Example devicetree fragment:
 *
 *     adc1: adc@... {
 *             label = "ADC_1";
 *     };
 *
 *     adc2: adc@... {
 *             label = "ADC_2";
 *     };
 *
 *     n: node {
 *             io-channels = <&adc1 10>, <&adc2 20>;
 *     };
 *
 * Example usage:
 *
 *     DT_IO_CHANNELS_LABEL_BY_IDX(DT_NODELABEL(n), 1) // "ADC_2"
 *
 * @param node_id node identifier for a node with an io-channels property
 * @param idx logical index into io-channels property
 * @return the label property of the node referenced at index "idx"
 * @see DT_PROP_BY_PHANDLE_IDX()
 */
#define DT_IO_CHANNELS_LABEL_BY_IDX(node_id, idx) \
	DT_PROP_BY_PHANDLE_IDX(node_id, io_channels, idx, label)

/**
 * @brief Get a label property from an io-channels property by name
 *
 * It's an error if the node referenced by the phandle in node_id's
 * io-channels property at the element named "name" has no label
 * property.
 *
 * Example devicetree fragment:
 *
 *     adc1: adc@... {
 *             label = "ADC_1";
 *     };
 *
 *     adc2: adc@... {
 *             label = "ADC_2";
 *     };
 *
 *     n: node {
 *             io-channels = <&adc1 10>, <&adc2 20>;
 *             io-channel-names = "SENSOR", "BANDGAP";
 *     };
 *
 * Example usage:
 *
 *     DT_IO_CHANNELS_LABEL_BY_NAME(DT_NODELABEL(n), bandgap) // "ADC_2"
 *
 * @param node_id node identifier for a node with an io-channels property
 * @param name lowercase-and-underscores name of an io-channels element
 *             as defined by the node's io-channel-names property
 * @return the label property of the node referenced at the named element
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_IO_CHANNELS_LABEL_BY_NAME(node_id, name) \
	DT_PROP(DT_PHANDLE_BY_NAME(node_id, io_channels, name), label)

/**
 * @brief Equivalent to DT_IO_CHANNELS_LABEL_BY_IDX(node_id, 0)
 * @param node_id node identifier for a node with an io-channels property
 * @return the label property of the node referenced at index 0
 * @see DT_IO_CHANNELS_LABEL_BY_IDX()
 */
#define DT_IO_CHANNELS_LABEL(node_id) DT_IO_CHANNELS_LABEL_BY_IDX(node_id, 0)

/**
 * @brief Get a label property from a DT_DRV_COMPAT instance's io-channels
 *        property at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into io-channels property
 * @return the label property of the node referenced at index "idx"
 * @see DT_IO_CHANNELS_LABEL_BY_IDX()
 */
#define DT_INST_IO_CHANNELS_LABEL_BY_IDX(inst, idx) \
	DT_IO_CHANNELS_LABEL_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get a label property from a DT_DRV_COMPAT instance's io-channels
 *        property by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of an io-channels element
 *             as defined by the instance's io-channel-names property
 * @return the label property of the node referenced at the named element
 * @see DT_IO_CHANNELS_LABEL_BY_NAME()
 */
#define DT_INST_IO_CHANNELS_LABEL_BY_NAME(inst, name) \
	DT_IO_CHANNELS_LABEL_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Equivalent to DT_INST_IO_CHANNELS_LABEL_BY_IDX(inst, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the label property of the node referenced at index 0
 */
#define DT_INST_IO_CHANNELS_LABEL(inst) DT_INST_IO_CHANNELS_LABEL_BY_IDX(inst, 0)

/**
 * @brief Get an io-channels specifier input cell at an index
 *
 * This macro only works for io-channels specifiers with cells named
 * "input". Refer to the node's binding to check if necessary.
 *
 * Example devicetree fragment:
 *
 *     adc1: adc@... {
 *             compatible = "vnd,adc";
 *             #io-channel-cells = <1>;
 *     };
 *
 *     adc2: adc@... {
 *             compatible = "vnd,adc";
 *             #io-channel-cells = <1>;
 *     };
 *
 *     n: node {
 *             io-channels = <&adc1 10>, <&adc2 20>;
 *     };
 *
 * Bindings fragment for the vnd,adc compatible:
 *
 *    io-channel-cells:
 *      - input
 *
 * Example usage:
 *
 *     DT_IO_CHANNELS_INPUT_BY_IDX(DT_NODELABEL(n), 0) // 10
 *     DT_IO_CHANNELS_INPUT_BY_IDX(DT_NODELABEL(n), 1) // 20
 *
 * @param node_id node identifier for a node with an io-channels property
 * @param idx logical index into io-channels property
 * @return the input cell in the specifier at index "idx"
 * @see DT_PHA_BY_IDX()
 */
#define DT_IO_CHANNELS_INPUT_BY_IDX(node_id, idx) \
	DT_PHA_BY_IDX(node_id, io_channels, idx, input)

/**
 * @brief Get an io-channels specifier input cell by name
 *
 * This macro only works for io-channels specifiers with cells named
 * "input". Refer to the node's binding to check if necessary.
 *
 * Example devicetree fragment:
 *
 *     adc1: adc@... {
 *             compatible = "vnd,adc";
 *             #io-channel-cells = <1>;
 *     };
 *
 *     adc2: adc@... {
 *             compatible = "vnd,adc";
 *             #io-channel-cells = <1>;
 *     };
 *
 *     n: node {
 *             io-channels = <&adc1 10>, <&adc2 20>;
 *             io-channel-names = "SENSOR", "BANDGAP";
 *     };
 *
 * Bindings fragment for the vnd,adc compatible:
 *
 *    io-channel-cells:
 *      - input
 *
 * Example usage:
 *
 *     DT_IO_CHANNELS_INPUT_BY_NAME(DT_NODELABEL(n), sensor) // 10
 *     DT_IO_CHANNELS_INPUT_BY_NAME(DT_NODELABEL(n), bandgap) // 20
 *
 * @param node_id node identifier for a node with an io-channels property
 * @param name lowercase-and-underscores name of an io-channels element
 *             as defined by the node's io-channel-names property
 * @return the input cell in the specifier at the named element
 * @see DT_PHA_BY_NAME()
 */
#define DT_IO_CHANNELS_INPUT_BY_NAME(node_id, name) \
	DT_PHA_BY_NAME(node_id, io_channels, name, input)
/**
 * @brief Equivalent to DT_IO_CHANNELS_INPUT_BY_IDX(node_id, 0)
 * @param node_id node identifier for a node with an io-channels property
 * @return the input cell in the specifier at index 0
 * @see DT_IO_CHANNELS_INPUT_BY_IDX()
 */
#define DT_IO_CHANNELS_INPUT(node_id) DT_IO_CHANNELS_INPUT_BY_IDX(node_id, 0)

/**
 * @brief Get an input cell from the "DT_DRV_INST(inst)" io-channels
 *        property at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into io-channels property
 * @return the input cell in the specifier at index "idx"
 * @see DT_IO_CHANNELS_INPUT_BY_IDX()
 */
#define DT_INST_IO_CHANNELS_INPUT_BY_IDX(inst, idx) \
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get an input cell from the "DT_DRV_INST(inst)" io-channels
 *        property by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of an io-channels element
 *             as defined by the instance's io-channel-names property
 * @return the input cell in the specifier at the named element
 * @see DT_IO_CHANNELS_INPUT_BY_NAME()
 */
#define DT_INST_IO_CHANNELS_INPUT_BY_NAME(inst, name) \
	DT_IO_CHANNELS_INPUT_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Equivalent to DT_INST_IO_CHANNELS_INPUT_BY_IDX(inst, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the input cell in the specifier at index 0
 */
#define DT_INST_IO_CHANNELS_INPUT(inst) DT_INST_IO_CHANNELS_INPUT_BY_IDX(inst, 0)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_IO_CHANNELS_H_ */
