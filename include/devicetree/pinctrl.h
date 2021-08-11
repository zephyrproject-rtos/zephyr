/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_PINCTRL_H_
#define ZEPHYR_INCLUDE_DEVICETREE_PINCTRL_H_

/**
 * @file
 * @brief Devicetree pin control helpers
 */

/**
 * @defgroup devicetree-pinctrl Pin control
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get a node identifier for a phandle in a pinctrl property by index
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             pinctrl-0 = <&foo &bar>;
 *             pinctrl-1 = <&baz &blub>;
 *     }
 *
 * Example usage:
 *
 *     DT_PINCTRL_BY_IDX(DT_NODELABEL(n), 0, 1) // DT_NODELABEL(bar)
 *     DT_PINCTRL_BY_IDX(DT_NODELABEL(n), 1, 0) // DT_NODELABEL(baz)
 *
 * @param node_id node with a pinctrl-'pc_idx' property
 * @param pc_idx index of the pinctrl property itself
 * @param idx index into the value of the pinctrl property
 * @return node identifier for the phandle at index 'idx' in 'pinctrl-'pc_idx''
 */
#define DT_PINCTRL_BY_IDX(node_id, pc_idx, idx) \
	DT_CAT6(node_id, _P_pinctrl_, pc_idx, _IDX_, idx, _PH)

/**
 * @brief Get a node identifier from a pinctrl-0 property
 *
 * This is equivalent to:
 *
 *     DT_PINCTRL_BY_IDX(node_id, 0, idx)
 *
 * It is provided for convenience since pinctrl-0 is commonly used.
 *
 * @param node_id node with a pinctrl-0 property
 * @param idx index into the pinctrl-0 property
 * @return node identifier for the phandle at index idx in the pinctrl-0
 *         property of that node
 */
#define DT_PINCTRL_0(node_id, idx) DT_PINCTRL_BY_IDX(node_id, 0, idx)

/**
 * @brief Get a node identifier for a phandle inside a pinctrl node by name
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             pinctrl-0 = <&foo &bar>;
 *             pinctrl-1 = <&baz &blub>;
 *             pinctrl-names = "default", "sleep";
 *     };
 *
 * Example usage:
 *
 *     DT_PINCTRL_BY_NAME(DT_NODELABEL(n), default, 1) // DT_NODELABEL(bar)
 *     DT_PINCTRL_BY_NAME(DT_NODELABEL(n), sleep, 0) // DT_NODELABEL(baz)
 *
 * @param node_id node with a named pinctrl property
 * @param name lowercase-and-underscores pinctrl property name
 * @param idx index into the value of the named pinctrl property
 * @return node identifier for the phandle at that index in the pinctrl
 *         property
 */
#define DT_PINCTRL_BY_NAME(node_id, name, idx) \
	DT_CAT6(node_id, _PINCTRL_NAME_, name, _IDX_, idx, _PH)

/**
 * @brief Convert a pinctrl name to its corresponding index
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             pinctrl-0 = <&foo &bar>;
 *             pinctrl-1 = <&baz &blub>;
 *             pinctrl-names = "default", "sleep";
 *     };
 *
 * Example usage:
 *
 *     DT_PINCTRL_NAME_TO_IDX(DT_NODELABEL(n), default) // 0
 *     DT_PINCTRL_NAME_TO_IDX(DT_NODELABEL(n), sleep)   // 1
 *
 * @param node_id node identifier with a named pinctrl property
 * @param name lowercase-and-underscores name name of the pinctrl whose index to get
 * @return integer literal for the index of the pinctrl property with that name
 */
#define DT_PINCTRL_NAME_TO_IDX(node_id, name) \
	DT_CAT4(node_id, _PINCTRL_NAME_, name, _IDX)

/**
 * @brief Convert a pinctrl property index to its name as a token
 *
 * This allows you to get a pinctrl property's name, and "remove the
 * quotes" from it.
 *
 * DT_PINCTRL_IDX_TO_NAME_TOKEN() can only be used if the node has a
 * pinctrl-'pc_idx' property and a pinctrl-names property element for
 * that index. It is an error to use it in other circumstances.
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             pinctrl-0 = <...>;
 *             pinctrl-1 = <...>;
 *             pinctrl-names = "default", "f.o.o2";
 *     };
 *
 * Example usage:
 *
 *     DT_PINCTRL_IDX_TO_NAME_TOKEN(DT_NODELABEL(n), 0) // default
 *     DT_PINCTRL_IDX_TO_NAME_TOKEN(DT_NODELABEL(n), 1) // f_o_o2
 *
 * The same caveats and restrictions that apply to DT_STRING_TOKEN()'s
 * return value also apply here.
 *
 * @param node_id node identifier
 * @param pc_idx index of a pinctrl property in that node
 * @return name of the pinctrl property, as a token, without any quotes
 *         and with non-alphanumeric characters converted to underscores
 */
#define DT_PINCTRL_IDX_TO_NAME_TOKEN(node_id, pc_idx) \
	DT_CAT4(node_id, _PINCTRL_IDX_, pc_idx, _TOKEN)

/**
 * @brief Like DT_PINCTRL_IDX_TO_NAME_TOKEN(), but with an uppercased result
 *
 * This does the a similar conversion as
 * DT_PINCTRL_IDX_TO_NAME_TOKEN(node_id, pc_idx). The only difference
 * is that alphabetical characters in the result are uppercased.
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             pinctrl-0 = <...>;
 *             pinctrl-1 = <...>;
 *             pinctrl-names = "default", "f.o.o2";
 *     };
 *
 * Example usage:
 *
 *     DT_PINCTRL_IDX_TO_NAME_TOKEN(DT_NODELABEL(n), 0) // DEFAULT
 *     DT_PINCTRL_IDX_TO_NAME_TOKEN(DT_NODELABEL(n), 1) // F_O_O2
 *
 * The same caveats and restrictions that apply to
 * DT_STRING_UPPER_TOKEN()'s return value also apply here.
 */
#define DT_PINCTRL_IDX_TO_NAME_UPPER_TOKEN(node_id, pc_idx) \
	DT_CAT4(node_id, _PINCTRL_IDX_, pc_idx, _UPPER_TOKEN)

/**
 * @brief Get the number of phandles in a pinctrl property
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             pinctrl-0 = <&foo &bar>;
 *     };
 *
 *     n2: node-2 {
 *             pinctrl-0 = <&baz>;
 *     };
 *
 * Example usage:
 *
 *     DT_NUM_PINCTRLS_BY_IDX(DT_NODELABEL(n1), 0) // 2
 *     DT_NUM_PINCTRLS_BY_IDX(DT_NODELABEL(n2), 0) // 1
 *
 * @param node_id node identifier with a pinctrl property
 * @param pc_idx index of the pinctrl property itself
 * @return number of phandles in the property with that index
 */
#define DT_NUM_PINCTRLS_BY_IDX(node_id, pc_idx) \
	DT_CAT4(node_id, _P_pinctrl_, pc_idx, _LEN)

/**
 * @brief Like DT_NUM_PINCTRLS_BY_IDX(), but by name instead
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             pinctrl-0 = <&foo &bar>;
 *             pinctrl-1 = <&baz>
 *             pinctrl-names = "default", "sleep";
 *     };
 *
 * Example usage:
 *
 *     DT_NUM_PINCTRLS_BY_NAME(DT_NODELABEL(n), default) // 2
 *     DT_NUM_PINCTRLS_BY_NAME(DT_NODELABEL(n), sleep)   // 1
 *
 * @param node_id node identifier with a pinctrl property
 * @param name lowercase-and-underscores name name of the pinctrl property
 * @return number of phandles in the property with that name
 */
#define DT_NUM_PINCTRLS_BY_NAME(node_id, name) \
	DT_NUM_PINCTRLS_BY_IDX(node_id, DT_PINCTRL_NAME_TO_IDX(node_id, name))

/**
 * @brief Get the number of pinctrl properties in a node
 *
 * This expands to 0 if there are no pinctrl-i properties.
 * Otherwise, it expands to the number of such properties.
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             pinctrl-0 = <...>;
 *             pinctrl-1 = <...>;
 *     };
 *
 *     n2: node-2 {
 *     };
 *
 * Example usage:
 *
 *     DT_NUM_PINCTRL_STATES(DT_NODELABEL(n1)) // 2
 *     DT_NUM_PINCTRL_STATES(DT_NODELABEL(n2)) // 0
 *
 * @param node_id node identifier; may or may not have any pinctrl properties
 * @return number of pinctrl properties in the node
 */
#define DT_NUM_PINCTRL_STATES(node_id) DT_CAT(node_id, _PINCTRL_NUM)

/**
 * @brief Test if a node has a pinctrl property with an index
 *
 * This expands to 1 if the pinctrl-'idx' property exists.
 * Otherwise, it expands to 0.
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             pinctrl-0 = <...>;
 *             pinctrl-1 = <...>;
 *     };
 *
 *     n2: node-2 {
 *     };
 *
 * Example usage:
 *
 *     DT_PINCTRL_HAS_IDX(DT_NODELABEL(n1), 0) // 1
 *     DT_PINCTRL_HAS_IDX(DT_NODELABEL(n1), 1) // 1
 *     DT_PINCTRL_HAS_IDX(DT_NODELABEL(n1), 2) // 0
 *     DT_PINCTRL_HAS_IDX(DT_NODELABEL(n2), 0) // 0
 *
 * @param node_id node identifier; may or may not have any pinctrl properties
 * @param pc_idx index of a pinctrl property whose existence to check
 * @return 1 if the property exists, 0 otherwise
 */
#define DT_PINCTRL_HAS_IDX(node_id, pc_idx) \
	IS_ENABLED(DT_CAT4(node_id, _PINCTRL_IDX_, pc_idx, _EXISTS))

/**
 * @brief Test if a node has a pinctrl property with a name
 *
 * This expands to 1 if the named pinctrl property exists.
 * Otherwise, it expands to 0.
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             pinctrl-0 = <...>;
 *             pinctrl-names = "default";
 *     };
 *
 *     n2: node-2 {
 *     };
 *
 * Example usage:
 *
 *     DT_PINCTRL_HAS_NAME(DT_NODELABEL(n1), default) // 1
 *     DT_PINCTRL_HAS_NAME(DT_NODELABEL(n1), sleep)   // 0
 *     DT_PINCTRL_HAS_NAME(DT_NODELABEL(n2), default) // 0
 *
 * @param node_id node identifier; may or may not have any pinctrl properties
 * @param name lowercase-and-underscores pinctrl property name to check
 * @return 1 if the property exists, 0 otherwise
 */
#define DT_PINCTRL_HAS_NAME(node_id, name) \
	IS_ENABLED(DT_CAT4(node_id, _PINCTRL_NAME_, name, _EXISTS))

/**
 * @brief Get a node identifier for a phandle in a pinctrl property by index
 *        for a DT_DRV_COMPAT instance
 *
 * This is equivalent to DT_PINCTRL_BY_IDX(DT_DRV_INST(inst), pc_idx, idx).
 *
 * @param inst instance number
 * @param pc_idx index of the pinctrl property itself
 * @param idx index into the value of the pinctrl property
 * @return node identifier for the phandle at index 'idx' in 'pinctrl-'pc_idx''
 */
#define DT_INST_PINCTRL_BY_IDX(inst, pc_idx, idx) \
	DT_PINCTRL_BY_IDX(DT_DRV_INST(inst), pc_idx, idx)

/**
 * @brief Get a node identifier from a pinctrl-0 property for a
 *        DT_DRV_COMPAT instance
 *
 * This is equivalent to:
 *
 *     DT_PINCTRL_BY_IDX(DT_DRV_INST(inst), 0, idx)
 *
 * It is provided for convenience since pinctrl-0 is commonly used.
 *
 * @param inst instance number
 * @param idx index into the pinctrl-0 property
 * @return node identifier for the phandle at index idx in the pinctrl-0
 *         property of that instance
 */
#define DT_INST_PINCTRL_0(inst, idx) \
	DT_PINCTRL_BY_IDX(DT_DRV_INST(inst), 0, idx)

/**
 * @brief Get a node identifier for a phandle inside a pinctrl node
 *        for a DT_DRV_COMPAT instance
 *
 * This is equivalent to DT_PINCTRL_BY_NAME(DT_DRV_INST(inst), name, idx).
 *
 * @param inst instance number
 * @param name lowercase-and-underscores pinctrl property name
 * @param idx index into the value of the named pinctrl property
 * @return node identifier for the phandle at that index in the pinctrl
 *         property
 */
#define DT_INST_PINCTRL_BY_NAME(inst, name, idx) \
	DT_PINCTRL_BY_NAME(DT_DRV_INST(inst), name, idx)

/**
 * @brief Convert a pinctrl name to its corresponding index
 *        for a DT_DRV_COMPAT instance
 *
 * This is equivalent to DT_PINCTRL_NAME_TO_IDX(DT_DRV_INST(inst),
 * name).
 *
 * @param inst instance number
 * @param name lowercase-and-underscores name of the pinctrl whose index to get
 * @return integer literal for the index of the pinctrl property with that name
 */
#define DT_INST_PINCTRL_NAME_TO_IDX(inst, name) \
	DT_PINCTRL_NAME_TO_IDX(DT_DRV_INST(inst), name)

/**
 * @brief Convert a pinctrl index to its name as an uppercased token
 *
 * This is equivalent to
 * DT_PINCTRL_IDX_TO_NAME_TOKEN(DT_DRV_INST(inst), pc_idx).
 *
 * @param inst instance number
 * @param pc_idx index of the pinctrl property itself
 * @return name of the pin control property as a token
 */
#define DT_INST_PINCTRL_IDX_TO_NAME_TOKEN(inst, pc_idx) \
	DT_PINCTRL_IDX_TO_NAME_TOKEN(DT_DRV_INST(inst), pc_idx)

/**
 * @brief Convert a pinctrl index to its name as an uppercased token
 *
 * This is equivalent to
 * DT_PINCTRL_IDX_TO_NAME_UPPER_TOKEN(DT_DRV_INST(inst), idx).
 *
 * @param inst instance number
 * @param pc_idx index of the pinctrl property itself
 * @return name of the pin control property as an uppercase token
 */
#define DT_INST_PINCTRL_IDX_TO_NAME_UPPER_TOKEN(inst, pc_idx) \
	DT_PINCTRL_IDX_TO_NAME_UPPER_TOKEN(DT_DRV_INST(inst), pc_idx)

/**
 * @brief Get the number of phandles in a pinctrl property
 *        for a DT_DRV_COMPAT instance
 *
 * This is equivalent to DT_NUM_PINCTRLS_BY_IDX(DT_DRV_INST(inst),
 * pc_idx).
 *
 * @param inst instance number
 * @param pc_idx index of the pinctrl property itself
 * @return number of phandles in the property with that index
 */
#define DT_INST_NUM_PINCTRLS_BY_IDX(inst, pc_idx) \
	DT_NUM_PINCTRLS_BY_IDX(DT_DRV_INST(inst), pc_idx)

/**
 * @brief Like DT_INST_NUM_PINCTRLS_BY_IDX(), but by name instead
 *
 * This is equivalent to DT_NUM_PINCTRLS_BY_NAME(DT_DRV_INST(inst),
 * name).
 *
 * @param inst instance number
 * @param name lowercase-and-underscores name of the pinctrl property
 * @return number of phandles in the property with that name
 */
#define DT_INST_NUM_PINCTRLS_BY_NAME(inst, name) \
	DT_NUM_PINCTRLS_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get the number of pinctrl properties in a DT_DRV_COMPAT instance
 *
 * This is equivalent to DT_NUM_PINCTRL_STATES(DT_DRV_INST(inst)).
 *
 * @param inst instance number
 * @return number of pinctrl properties in the instance
 */
#define DT_INST_NUM_PINCTRL_STATES(inst) \
	DT_NUM_PINCTRL_STATES(DT_DRV_INST(inst))

/**
 * @brief Test if a DT_DRV_COMPAT instance has a pinctrl property
 *        with an index
 *
 * This is equivalent to DT_PINCTRL_HAS_IDX(DT_DRV_INST(inst), pc_idx).
 *
 * @param inst instance number
 * @param pc_idx index of a pinctrl property whose existence to check
 * @return 1 if the property exists, 0 otherwise
 */
#define DT_INST_PINCTRL_HAS_IDX(inst, pc_idx) \
	DT_PINCTRL_HAS_IDX(DT_DRV_INST(inst), pc_idx)

/**
 * @brief Test if a DT_DRV_COMPAT instance has a pinctrl property with a name
 *
 * This is equivalent to DT_PINCTRL_HAS_NAME(DT_DRV_INST(inst), name).
 *
 * @param inst instance number
 * @param name lowercase-and-underscores pinctrl property name to check
 * @return 1 if the property exists, 0 otherwise
 */
#define DT_INST_PINCTRL_HAS_NAME(inst, name) \
	DT_PINCTRL_HAS_NAME(DT_DRV_INST(inst), name)


/**
 * @}
 */

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_PINCTRL_H_ */
