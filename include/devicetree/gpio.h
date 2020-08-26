/**
 * @file
 * @brief GPIO Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2020, Linaro Ltd.
 * Copyright (c) 2020 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_GPIO_H_
#define ZEPHYR_INCLUDE_DEVICETREE_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-gpio Devicetree GPIO API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get a label property from a gpio phandle-array property
 *        at an index
 *
 * It's an error if the GPIO controller node referenced by the phandle
 * in node_id's "gpio_pha" property at index "idx" has no label
 * property.
 *
 * Example devicetree fragment:
 *
 *     gpio1: gpio@... {
 *             label = "GPIO_1";
 *     };
 *
 *     gpio2: gpio@... {
 *             label = "GPIO_2";
 *     };
 *
 *     n: node {
 *             gpios = <&gpio1 10 GPIO_ACTIVE_LOW>,
 *                     <&gpio2 30 GPIO_ACTIVE_HIGH>;
 *     };
 *
 * Example usage:
 *
 *     DT_GPIO_LABEL_BY_IDX(DT_NODELABEL(n), gpios, 1) // "GPIO_2"
 *
 * @param node_id node identifier
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @param idx logical index into "gpio_pha"
 * @return the label property of the node referenced at index "idx"
 * @see DT_PHANDLE_BY_IDX()
 */
#define DT_GPIO_LABEL_BY_IDX(node_id, gpio_pha, idx) \
	DT_PROP_BY_PHANDLE_IDX(node_id, gpio_pha, idx, label)

/**
 * @brief Equivalent to DT_GPIO_LABEL_BY_IDX(node_id, gpio_pha, 0)
 * @param node_id node identifier
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @return the label property of the node referenced at index 0
 * @see DT_GPIO_LABEL_BY_IDX()
 */
#define DT_GPIO_LABEL(node_id, gpio_pha) \
	DT_GPIO_LABEL_BY_IDX(node_id, gpio_pha, 0)

/**
 * @brief Get a GPIO specifier's pin cell at an index
 *
 * This macro only works for GPIO specifiers with cells named "pin".
 * Refer to the node's binding to check if necessary.
 *
 * Example devicetree fragment:
 *
 *     gpio1: gpio@... {
 *             compatible = "vnd,gpio";
 *             #gpio-cells = <2>;
 *     };
 *
 *     gpio2: gpio@... {
 *             compatible = "vnd,gpio";
 *             #gpio-cells = <2>;
 *     };
 *
 *     n: node {
 *             gpios = <&gpio1 10 GPIO_ACTIVE_LOW>,
 *                     <&gpio2 30 GPIO_ACTIVE_HIGH>;
 *     };
 *
 * Bindings fragment for the vnd,gpio compatible:
 *
 *     gpio-cells:
 *       - pin
 *       - flags
 *
 * Example usage:
 *
 *     DT_GPIO_PIN_BY_IDX(DT_NODELABEL(n), gpios, 0) // 10
 *     DT_GPIO_PIN_BY_IDX(DT_NODELABEL(n), gpios, 1) // 30
 *
 * @param node_id node identifier
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @param idx logical index into "gpio_pha"
 * @return the pin cell value at index "idx"
 * @see DT_PHA_BY_IDX()
 */
#define DT_GPIO_PIN_BY_IDX(node_id, gpio_pha, idx) \
	DT_PHA_BY_IDX(node_id, gpio_pha, idx, pin)

/**
 * @brief Equivalent to DT_GPIO_PIN_BY_IDX(node_id, gpio_pha, 0)
 * @param node_id node identifier
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @return the pin cell value at index 0
 * @see DT_GPIO_PIN_BY_IDX()
 */
#define DT_GPIO_PIN(node_id, gpio_pha) \
	DT_GPIO_PIN_BY_IDX(node_id, gpio_pha, 0)

/**
 * @brief Get a GPIO specifier's flags cell at an index
 *
 * This macro expects GPIO specifiers with cells named "flags".
 * If there is no "flags" cell in the GPIO specifier, zero is returned.
 * Refer to the node's binding to check specifier cell names if necessary.
 *
 * Example devicetree fragment:
 *
 *     gpio1: gpio@... {
 *             compatible = "vnd,gpio";
 *             #gpio-cells = <2>;
 *     };
 *
 *     gpio2: gpio@... {
 *             compatible = "vnd,gpio";
 *             #gpio-cells = <2>;
 *     };
 *
 *     n: node {
 *             gpios = <&gpio1 10 GPIO_ACTIVE_LOW>,
 *                     <&gpio2 30 GPIO_ACTIVE_HIGH>;
 *     };
 *
 * Bindings fragment for the vnd,gpio compatible:
 *
 *     gpio-cells:
 *       - pin
 *       - flags
 *
 * Example usage:
 *
 *     DT_GPIO_FLAGS_BY_IDX(DT_NODELABEL(n), gpios, 0) // GPIO_ACTIVE_LOW
 *     DT_GPIO_FLAGS_BY_IDX(DT_NODELABEL(n), gpios, 1) // GPIO_ACTIVE_HIGH
 *
 * @param node_id node identifier
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @param idx logical index into "gpio_pha"
 * @return the flags cell value at index "idx", or zero if there is none
 * @see DT_PHA_BY_IDX()
 */
#define DT_GPIO_FLAGS_BY_IDX(node_id, gpio_pha, idx) \
	DT_PHA_BY_IDX_OR(node_id, gpio_pha, idx, flags, 0)

/**
 * @brief Equivalent to DT_GPIO_FLAGS_BY_IDX(node_id, gpio_pha, 0)
 * @param node_id node identifier
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @return the flags cell value at index 0, or zero if there is none
 * @see DT_GPIO_FLAGS_BY_IDX()
 */
#define DT_GPIO_FLAGS(node_id, gpio_pha) \
	DT_GPIO_FLAGS_BY_IDX(node_id, gpio_pha, 0)

/**
 * @brief Get a label property from a DT_DRV_COMPAT instance's GPIO
 *        property at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @param idx logical index into "gpio_pha"
 * @return the label property of the node referenced at index "idx"
 */
#define DT_INST_GPIO_LABEL_BY_IDX(inst, gpio_pha, idx) \
	DT_GPIO_LABEL_BY_IDX(DT_DRV_INST(inst), gpio_pha, idx)

/**
 * @brief Equivalent to DT_INST_GPIO_LABEL_BY_IDX(inst, gpio_pha, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @return the label property of the node referenced at index 0
 */
#define DT_INST_GPIO_LABEL(inst, gpio_pha) \
	DT_INST_GPIO_LABEL_BY_IDX(inst, gpio_pha, 0)

/**
 * @brief Get a DT_DRV_COMPAT instance's GPIO specifier's pin cell value
 *        at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @param idx logical index into "gpio_pha"
 * @return the pin cell value at index "idx"
 * @see DT_GPIO_PIN_BY_IDX()
 */
#define DT_INST_GPIO_PIN_BY_IDX(inst, gpio_pha, idx) \
	DT_GPIO_PIN_BY_IDX(DT_DRV_INST(inst), gpio_pha, idx)

/**
 * @brief Equivalent to DT_INST_GPIO_PIN_BY_IDX(inst, gpio_pha, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @return the pin cell value at index 0
 * @see DT_INST_GPIO_PIN_BY_IDX()
 */
#define DT_INST_GPIO_PIN(inst, gpio_pha) \
	DT_INST_GPIO_PIN_BY_IDX(inst, gpio_pha, 0)

/**
 * @brief Get a DT_DRV_COMPAT instance's GPIO specifier's flags cell
 *        at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @param idx logical index into "gpio_pha"
 * @return the flags cell value at index "idx", or zero if there is none
 * @see DT_GPIO_FLAGS_BY_IDX()
 */
#define DT_INST_GPIO_FLAGS_BY_IDX(inst, gpio_pha, idx) \
	DT_GPIO_FLAGS_BY_IDX(DT_DRV_INST(inst), gpio_pha, idx)

/**
 * @brief Equivalent to DT_INST_GPIO_FLAGS_BY_IDX(inst, gpio_pha, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @return the flags cell value at index 0, or zero if there is none
 * @see DT_INST_GPIO_FLAGS_BY_IDX()
 */
#define DT_INST_GPIO_FLAGS(inst, gpio_pha) \
	DT_INST_GPIO_FLAGS_BY_IDX(inst, gpio_pha, 0)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_GPIO_H_ */
