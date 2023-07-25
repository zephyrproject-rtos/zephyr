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
 * @brief Get the node identifier for the controller phandle from a
 *        gpio phandle-array property at an index
 *
 * Example devicetree fragment:
 *
 *     gpio1: gpio@... { };
 *
 *     gpio2: gpio@... { };
 *
 *     n: node {
 *             gpios = <&gpio1 10 GPIO_ACTIVE_LOW>,
 *                     <&gpio2 30 GPIO_ACTIVE_HIGH>;
 *     };
 *
 * Example usage:
 *
 *     DT_GPIO_CTLR_BY_IDX(DT_NODELABEL(n), gpios, 1) // DT_NODELABEL(gpio2)
 *
 * @param node_id node identifier
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @param idx logical index into "gpio_pha"
 * @return the node identifier for the gpio controller referenced at
 *         index "idx"
 * @see DT_PHANDLE_BY_IDX()
 */
#define DT_GPIO_CTLR_BY_IDX(node_id, gpio_pha, idx) \
	DT_PHANDLE_BY_IDX(node_id, gpio_pha, idx)

/**
 * @brief Equivalent to DT_GPIO_CTLR_BY_IDX(node_id, gpio_pha, 0)
 * @param node_id node identifier
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @return a node identifier for the gpio controller at index 0
 *         in "gpio_pha"
 * @see DT_GPIO_CTLR_BY_IDX()
 */
#define DT_GPIO_CTLR(node_id, gpio_pha) \
	DT_GPIO_CTLR_BY_IDX(node_id, gpio_pha, 0)

/**
 * @deprecated If used to obtain a device instance with device_get_binding,
 * consider using @c DEVICE_DT_GET(DT_GPIO_CTLR_BY_IDX(node, gpio_pha, idx)).
 *
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
	DT_PROP(DT_GPIO_CTLR_BY_IDX(node_id, gpio_pha, idx), label) __DEPRECATED_MACRO

/**
 * @deprecated If used to obtain a device instance with device_get_binding,
 * consider using @c DEVICE_DT_GET(DT_GPIO_CTLR(node, gpio_pha)).
 *
 * @brief Equivalent to DT_GPIO_LABEL_BY_IDX(node_id, gpio_pha, 0)
 * @param node_id node identifier
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @return the label property of the node referenced at index 0
 * @see DT_GPIO_LABEL_BY_IDX()
 */
#define DT_GPIO_LABEL(node_id, gpio_pha) \
	DT_GPIO_LABEL_BY_IDX(node_id, gpio_pha, 0) __DEPRECATED_MACRO

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
 * @brief Get the number of GPIO hogs in a node
 *
 * This expands to the number of hogged GPIOs, or zero if there are none.
 *
 * Example devicetree fragment:
 *
 *     gpio1: gpio@... {
 *       compatible = "vnd,gpio";
 *       #gpio-cells = <2>;
 *
 *       n1: node-1 {
 *               gpio-hog;
 *               gpios = <0 GPIO_ACTIVE_HIGH>, <1 GPIO_ACTIVE_LOW>;
 *               output-high;
 *       };
 *
 *       n2: node-2 {
 *               gpio-hog;
 *               gpios = <3 GPIO_ACTIVE_HIGH>;
 *               output-low;
 *       };
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
 *     DT_NUM_GPIO_HOGS(DT_NODELABEL(n1)) // 2
 *     DT_NUM_GPIO_HOGS(DT_NODELABEL(n2)) // 1
 *
 * @param node_id node identifier; may or may not be a GPIO hog node.
 * @return number of hogged GPIOs in the node
 */
#define DT_NUM_GPIO_HOGS(node_id) \
	COND_CODE_1(IS_ENABLED(DT_CAT(node_id, _GPIO_HOGS_EXISTS)), \
		    (DT_CAT(node_id, _GPIO_HOGS_NUM)), (0))

/**
 * @brief Get a GPIO hog specifier's pin cell at an index
 *
 * This macro only works for GPIO specifiers with cells named "pin".
 * Refer to the node's binding to check if necessary.
 *
 * Example devicetree fragment:
 *
 *     gpio1: gpio@... {
 *       compatible = "vnd,gpio";
 *       #gpio-cells = <2>;
 *
 *       n1: node-1 {
 *               gpio-hog;
 *               gpios = <0 GPIO_ACTIVE_HIGH>, <1 GPIO_ACTIVE_LOW>;
 *               output-high;
 *       };
 *
 *       n2: node-2 {
 *               gpio-hog;
 *               gpios = <3 GPIO_ACTIVE_HIGH>;
 *               output-low;
 *       };
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
 *     DT_GPIO_HOG_PIN_BY_IDX(DT_NODELABEL(n1), 0) // 0
 *     DT_GPIO_HOG_PIN_BY_IDX(DT_NODELABEL(n1), 1) // 1
 *     DT_GPIO_HOG_PIN_BY_IDX(DT_NODELABEL(n2), 0) // 3
 *
 * @param node_id node identifier
 * @param idx logical index into "gpios"
 * @return the pin cell value at index "idx"
 */
#define DT_GPIO_HOG_PIN_BY_IDX(node_id, idx) \
	DT_CAT4(node_id, _GPIO_HOGS_IDX_, idx, _VAL_pin)

/**
 * @brief Get a GPIO hog specifier's flags cell at an index
 *
 * This macro expects GPIO specifiers with cells named "flags".
 * If there is no "flags" cell in the GPIO specifier, zero is returned.
 * Refer to the node's binding to check specifier cell names if necessary.
 *
 * Example devicetree fragment:
 *
 *     gpio1: gpio@... {
 *       compatible = "vnd,gpio";
 *       #gpio-cells = <2>;
 *
 *       n1: node-1 {
 *               gpio-hog;
 *               gpios = <0 GPIO_ACTIVE_HIGH>, <1 GPIO_ACTIVE_LOW>;
 *               output-high;
 *       };
 *
 *       n2: node-2 {
 *               gpio-hog;
 *               gpios = <3 GPIO_ACTIVE_HIGH>;
 *               output-low;
 *       };
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
 *     DT_GPIO_HOG_FLAGS_BY_IDX(DT_NODELABEL(n1), 0) // GPIO_ACTIVE_HIGH
 *     DT_GPIO_HOG_FLAGS_BY_IDX(DT_NODELABEL(n1), 1) // GPIO_ACTIVE_LOW
 *     DT_GPIO_HOG_FLAGS_BY_IDX(DT_NODELABEL(n2), 0) // GPIO_ACTIVE_HIGH
 *
 * @param node_id node identifier
 * @param idx logical index into "gpios"
 * @return the flags cell value at index "idx", or zero if there is none
 */
#define DT_GPIO_HOG_FLAGS_BY_IDX(node_id, idx) \
	COND_CODE_1(IS_ENABLED(DT_CAT4(node_id, _GPIO_HOGS_IDX_, idx, _VAL_flags_EXISTS)), \
		    (DT_CAT4(node_id, _GPIO_HOGS_IDX_, idx, _VAL_flags)), (0))

/**
 * @deprecated If used to obtain a device instance with device_get_binding,
 * consider using @c DEVICE_DT_GET(DT_INST_GPIO_CTLR_BY_IDX(node, gpio_pha, idx)).
 *
 * @brief Get a label property from a DT_DRV_COMPAT instance's GPIO
 *        property at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @param idx logical index into "gpio_pha"
 * @return the label property of the node referenced at index "idx"
 */
#define DT_INST_GPIO_LABEL_BY_IDX(inst, gpio_pha, idx) \
	DT_GPIO_LABEL_BY_IDX(DT_DRV_INST(inst), gpio_pha, idx) __DEPRECATED_MACRO

/**
 * @deprecated If used to obtain a device instance with device_get_binding,
 * consider using @c DEVICE_DT_GET(DT_INST_GPIO_CTLR(node, gpio_pha)).
 *
 * @brief Equivalent to DT_INST_GPIO_LABEL_BY_IDX(inst, gpio_pha, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @param gpio_pha lowercase-and-underscores GPIO property with
 *        type "phandle-array"
 * @return the label property of the node referenced at index 0
 */
#define DT_INST_GPIO_LABEL(inst, gpio_pha) \
	DT_INST_GPIO_LABEL_BY_IDX(inst, gpio_pha, 0) __DEPRECATED_MACRO

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
