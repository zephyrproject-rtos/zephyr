/**
 * @file
 * @brief ADC Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2020, Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_ADC_H_
#define ZEPHYR_INCLUDE_DEVICETREE_ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-adc Devicetree ADC API
 * @{
 */

/**
 * @brief Get IO channels controller "name" (label property) at an index
 *
 * It's an error if the IO channels controller referenced by the phandle
 * in property "io_channels" at index "idx" has no label property.
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
 * @param node_id node identifier
 * @param idx logical index into the property
 * @return the label property for the referenced node at index idx
 * @see DT_PHANDLE_BY_IDX()
 */
#define DT_IO_CHANNELS_LABEL_BY_IDX(node_id, idx) \
	DT_PROP_BY_PHANDLE_IDX(node_id, io_channels, idx, label)

/**
 * @brief Get IO channels controller "name" (label property) by name
 *
 * It's an error if the IO channels controller referenced by the phandle
 * in property "io_channels" at index "idx" has no label property.
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
 * @param node_id node identifier
 * @param name lowercase-and-underscores "io_channel" name
 * @return the label property for the referenced node by name
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_IO_CHANNELS_LABEL_BY_NAME(node_id, name) \
	DT_PROP(DT_PHANDLE_BY_NAME(node_id, io_channels, name), label)

/**
 * @brief Equivalent to DT_IO_CHANNELS_LABEL_BY_IDX(node_id, 0)
 * @param node_id node identifier
 * @return the label property for the named specifier at index 0
 * @see DT_IO_CHANNELS_LABEL_BY_IDX()
 */
#define DT_IO_CHANNELS_LABEL(node_id) DT_IO_CHANNELS_LABEL_BY_IDX(node_id, 0)

/**
 * @brief Get IO channel's controller "name" at an index
 * (see @ref DT_IO_CHANNELS_LABEL_BY_IDX)
 * @param inst instance number
 * @param idx logical index into the property
 * @return the label property for the named specifier at index idx
 */
#define DT_INST_IO_CHANNELS_LABEL_BY_IDX(inst, idx) \
	DT_IO_CHANNELS_LABEL_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get IO channel's controller "name" by name
 * (see @ref DT_IO_CHANNELS_LABEL_BY_NAME)
 * @param inst instance number
 * @param name lowercase-and-underscores "io_channel" name
 * @return the label property for the named specifier by name
 */
#define DT_INST_IO_CHANNELS_LABEL_BY_NAME(inst, name) \
	DT_IO_CHANNELS_LABEL_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Equivalent to DT_INST_IO_CHANNELS_LABEL_BY_IDX(inst, 0)
 * @param inst instance number
 * @return the label property for the named specifier at index 0
 */
#define DT_INST_IO_CHANNELS_LABEL(inst) DT_INST_IO_CHANNELS_LABEL_BY_IDX(inst, 0)

/**
 * @brief Get IO channels controller 'input' at an index
 *
 * This macro only works for IO channels controllers that specify a 'input'
 * field in the phandle-array specifier.  Refer to the specific IO channels
 * controller binding if needed.
 *
 * @param node_id node identifier
 * @param idx logical index into the property
 * @return the input value for the named specifier at index idx
 * @see DT_PHA_BY_IDX()
 * @see DT_PHA()
 */
#define DT_IO_CHANNELS_INPUT_BY_IDX(node_id, idx) \
	DT_PHA_BY_IDX(node_id, io_channels, idx, input)

/**
 * @brief Get IO channels controller 'input' by name
 *
 * This macro only works for IO channels controllers that specify a 'input'
 * field in the phandle-array specifier.  Refer to the specific IO channels
 * controller binding if needed.
 *
 * @param node_id node identifier
 * @param name lowercase-and-underscores "io_channel" name
 * @return the input value for the named specifier by name
 * @see DT_PHA_BY_NAME()
 */
#define DT_IO_CHANNELS_INPUT_BY_NAME(node_id, name) \
	DT_PHA_BY_NAME(node_id, io_channels, name, input)
/**
 * @brief Equivalent to DT_IO_CHANNELS_INPUT_BY_IDX(node_id, 0)
 * @param node_id node identifier
 * @return the label property for the named specifier at index 0
 * @see DT_IO_CHANNELS_INPUT_BY_IDX()
 */
#define DT_IO_CHANNELS_INPUT(node_id) DT_IO_CHANNELS_INPUT_BY_IDX(node_id, 0)

/**
 * @brief Get IO channel's controller "input" at an index
 * (see @ref DT_IO_CHANNELS_INPUT_BY_IDX)
 * @param inst instance number
 * @param idx logical index into the property
 * @return the input value for the named specifier at index idx
 */
#define DT_INST_IO_CHANNELS_INPUT_BY_IDX(inst, idx) \
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get IO channel's controller "input" by name
 * (see @ref DT_IO_CHANNELS_INPUT_BY_NAME)
 * @param inst instance number
 * @param name lowercase-and-underscores "io_channel" name
 * @return the input value for the named specifier at index idx
 */
#define DT_INST_IO_CHANNELS_INPUT_BY_NAME(inst, name) \
	DT_IO_CHANNELS_INPUT_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Equivalent to DT_INST_IO_CHANNELS_INPUT(inst, 0)
 * @param inst instance number
 * @return the input property for the named specifier at index 0
 */
#define DT_INST_IO_CHANNELS_INPUT(inst) DT_INST_IO_CHANNELS_INPUT_BY_IDX(inst, 0)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif  /* ZEPHYR_INCLUDE_DEVICETREE_ADC_H_ */
