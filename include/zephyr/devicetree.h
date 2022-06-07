/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2020 Nordic Semiconductor
 * Copyright (c) 2020, Linaro Ltd.
 *
 * Not a generated file. Feel free to modify.
 */

/**
 * @file
 * @brief Devicetree main header
 *
 * API for accessing the current application's devicetree macros.
 */

#ifndef DEVICETREE_H
#define DEVICETREE_H

#include <devicetree_unfixed.h>
#include <devicetree_fixups.h>

#include <zephyr/sys/util.h>

/**
 * @brief devicetree.h API
 * @defgroup devicetree Devicetree
 * @{
 * @}
 */

/*
 * Property suffixes
 * -----------------
 *
 * These are the optional parts that come after the _P_<property>
 * part in DT_N_<path-id>_P_<property-id> macros, or the "prop-suf"
 * nonterminal in the DT guide's macros.bnf file.
 *
 * Before adding new ones, check this list to avoid conflicts. If any
 * are missing from this list, please add them. It should be complete.
 *
 * _ENUM_IDX: property's value as an index into bindings enum
 * _ENUM_TOKEN: property's value as a token into bindings enum (string
 *              enum values are identifiers) [deprecated, use _STRING_TOKEN]
 * _ENUM_UPPER_TOKEN: like _ENUM_TOKEN, but uppercased [deprecated, use
 *		      _STRING_UPPER_TOKEN]
 * _EXISTS: property is defined
 * _FOREACH_PROP_ELEM: helper for "iterating" over values in the property
 * _FOREACH_PROP_ELEM_VARGS: foreach functions with variable number of arguments
 * _IDX_<i>: logical index into property
 * _IDX_<i>_EXISTS: logical index into property is defined
 * _IDX_<i>_PH: phandle array's phandle by index (or phandle, phandles)
 * _IDX_<i>_VAL_<val>: phandle array's specifier value by index
 * _IDX_<i>_VAL_<val>_EXISTS: cell value exists, by index
 * _LEN: property logical length
 * _NAME_<name>_PH: phandle array's phandle by name
 * _NAME_<name>_VAL_<val>: phandle array's property specifier by name
 * _NAME_<name>_VAL_<val>_EXISTS: cell value exists, by name
 * _STRING_TOKEN: string property's value as a token
 * _STRING_UPPER_TOKEN: like _STRING_TOKEN, but uppercased
 */

/**
 * @defgroup devicetree-generic-id Node identifiers and helpers
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Name for an invalid node identifier
 *
 * This supports cases where factored macros can be invoked from paths where
 * devicetree data may or may not be available.  It is a preprocessor identifier
 * that does not match any valid devicetree node identifier.
 */
#define DT_INVALID_NODE _

/**
 * @brief Node identifier for the root node in the devicetree
 */
#define DT_ROOT DT_N

/**
 * @brief Get a node identifier for a devicetree path
 *
 * (This macro returns a node identifier from path components. To get
 * a path string from a node identifier, use DT_NODE_PATH() instead.)
 *
 * The arguments to this macro are the names of non-root nodes in the
 * tree required to reach the desired node, starting from the root.
 * Non-alphanumeric characters in each name must be converted to
 * underscores to form valid C tokens, and letters must be lowercased.
 *
 * Example devicetree fragment:
 *
 *     / {
 *             soc {
 *                     serial1: serial@40001000 {
 *                             status = "okay";
 *                             current-speed = <115200>;
 *                             ...
 *                     };
 *             };
 *     };
 *
 * You can use DT_PATH(soc, serial_40001000) to get a node identifier
 * for the serial@40001000 node. Node labels like "serial1" cannot be
 * used as DT_PATH() arguments; use DT_NODELABEL() for those instead.
 *
 * Example usage with DT_PROP() to get the current-speed property:
 *
 *     DT_PROP(DT_PATH(soc, serial_40001000), current_speed) // 115200
 *
 * (The current-speed property is also in "lowercase-and-underscores"
 * form when used with this API.)
 *
 * When determining arguments to DT_PATH():
 *
 * - the first argument corresponds to a child node of the root ("soc" above)
 * - a second argument corresponds to a child of the first argument
 *   ("serial_40001000" above, from the node name "serial@40001000"
 *   after lowercasing and changing "@" to "_")
 * - and so on for deeper nodes in the desired node's path
 *
 * @param ... lowercase-and-underscores node names along the node's path,
 *            with each name given as a separate argument
 * @return node identifier for the node with that path
 */
#define DT_PATH(...) DT_PATH_INTERNAL(__VA_ARGS__)

/**
 * @brief Get a node identifier for a node label
 *
 * Convert non-alphanumeric characters in the node label to
 * underscores to form valid C tokens, and lowercase all letters. Note
 * that node labels are not the same thing as label properties.
 *
 * Example devicetree fragment:
 *
 *     serial1: serial@40001000 {
 *             label = "UART_0";
 *             status = "okay";
 *             current-speed = <115200>;
 *             ...
 *     };
 *
 * The only node label in this example is "serial1".
 *
 * The string "UART_0" is *not* a node label; it's the value of a
 * property named label.
 *
 * You can use DT_NODELABEL(serial1) to get a node identifier for the
 * serial@40001000 node. Example usage with DT_PROP() to get the
 * current-speed property:
 *
 *     DT_PROP(DT_NODELABEL(serial1), current_speed) // 115200
 *
 * Another example devicetree fragment:
 *
 *     cpu@0 {
 *            L2_0: l2-cache {
 *                    cache-level = <2>;
 *                    ...
 *            };
 *     };
 *
 * Example usage to get the cache-level property:
 *
 *     DT_PROP(DT_NODELABEL(l2_0), cache_level) // 2
 *
 * Notice how "L2_0" in the devicetree is lowercased to "l2_0" in the
 * DT_NODELABEL() argument.
 *
 * @param label lowercase-and-underscores node label name
 * @return node identifier for the node with that label
 */
#define DT_NODELABEL(label) DT_CAT(DT_N_NODELABEL_, label)

/**
 * @brief Get a node identifier from /aliases
 *
 * This macro's argument is a property of the /aliases node. It
 * returns a node identifier for the node which is aliased. Convert
 * non-alphanumeric characters in the alias property to underscores to
 * form valid C tokens, and lowercase all letters.
 *
 * Example devicetree fragment:
 *
 *     / {
 *             aliases {
 *                     my-serial = &serial1;
 *             };
 *
 *             soc {
 *                     serial1: serial@40001000 {
 *                             status = "okay";
 *                             current-speed = <115200>;
 *                             ...
 *                     };
 *             };
 *     };
 *
 * You can use DT_ALIAS(my_serial) to get a node identifier for the
 * serial@40001000 node. Notice how my-serial in the devicetree
 * becomes my_serial in the DT_ALIAS() argument. Example usage with
 * DT_PROP() to get the current-speed property:
 *
 *     DT_PROP(DT_ALIAS(my_serial), current_speed) // 115200
 *
 * @param alias lowercase-and-underscores alias name.
 * @return node identifier for the node with that alias
 */
#define DT_ALIAS(alias) DT_CAT(DT_N_ALIAS_, alias)

/**
 * @brief Get a node identifier for an instance of a compatible
 *
 * All nodes with a particular compatible property value are assigned
 * instance numbers, which are zero-based indexes specific to that
 * compatible. You can get a node identifier for these nodes by
 * passing DT_INST() an instance number, "inst", along with the
 * lowercase-and-underscores version of the compatible, "compat".
 *
 * Instance numbers have the following properties:
 *
 * - for each compatible, instance numbers start at 0 and are contiguous
 * - exactly one instance number is assigned for each node with a compatible,
 *   **including disabled nodes**
 * - enabled nodes (status property is "okay" or missing) are assigned the
 *   instance numbers starting from 0, and disabled nodes have instance
 *   numbers which are greater than those of any enabled node
 *
 * No other guarantees are made. In particular:
 *
 * - instance numbers **in no way reflect** any numbering scheme that
 *   might exist in SoC documentation, node labels or unit addresses,
 *   or properties of the /aliases node (use DT_NODELABEL() or DT_ALIAS()
 *   for those)
 * - there **is no general guarantee** that the same node will have
 *   the same instance number between builds, even if you are building
 *   the same application again in the same build directory
 *
 * Example devicetree fragment:
 *
 *     serial1: serial@40001000 {
 *             compatible = "vnd,soc-serial";
 *             status = "disabled";
 *             current-speed = <9600>;
 *             ...
 *     };
 *
 *     serial2: serial@40002000 {
 *             compatible = "vnd,soc-serial";
 *             status = "okay";
 *             current-speed = <57600>;
 *             ...
 *     };
 *
 *     serial3: serial@40003000 {
 *             compatible = "vnd,soc-serial";
 *             current-speed = <115200>;
 *             ...
 *     };
 *
 * Assuming no other nodes in the devicetree have compatible
 * "vnd,soc-serial", that compatible has nodes with instance numbers
 * 0, 1, and 2.
 *
 * The nodes serial@40002000 and serial@40003000 are both enabled, so
 * their instance numbers are 0 and 1, but no guarantees are made
 * regarding which node has which instance number.
 *
 * Since serial@40001000 is the only disabled node, it has instance
 * number 2, since disabled nodes are assigned the largest instance
 * numbers. Therefore:
 *
 *     // Could be 57600 or 115200. There is no way to be sure:
 *     // either serial@40002000 or serial@40003000 could
 *     // have instance number 0, so this could be the current-speed
 *     // property of either of those nodes.
 *     DT_PROP(DT_INST(0, vnd_soc_serial), current_speed)
 *
 *     // Could be 57600 or 115200, for the same reason.
 *     // If the above expression expands to 57600, then
 *     // this expands to 115200, and vice-versa.
 *     DT_PROP(DT_INST(1, vnd_soc_serial), current_speed)
 *
 *     // 9600, because there is only one disabled node, and
 *     // disabled nodes are "at the the end" of the instance
 *     // number "list".
 *     DT_PROP(DT_INST(2, vnd_soc_serial), current_speed)
 *
 * Notice how "vnd,soc-serial" in the devicetree becomes vnd_soc_serial
 * (without quotes) in the DT_INST() arguments. (As usual, current-speed
 * in the devicetree becomes current_speed as well.)
 *
 * Nodes whose "compatible" property has multiple values are assigned
 * independent instance numbers for each compatible.
 *
 * @param inst instance number for compatible "compat"
 * @param compat lowercase-and-underscores compatible, without quotes
 * @return node identifier for the node with that instance number and
 *         compatible
 */
#define DT_INST(inst, compat) UTIL_CAT(DT_N_INST, DT_DASH(inst, compat))

/**
 * @brief Get a node identifier for a parent node
 *
 * Example devicetree fragment:
 *
 *     parent: parent-node {
 *             child: child-node {
 *                     ...
 *             };
 *     };
 *
 * The following are equivalent ways to get the same node identifier:
 *
 *     DT_NODELABEL(parent)
 *     DT_PARENT(DT_NODELABEL(child))
 *
 * @param node_id node identifier
 * @return a node identifier for the node's parent
 */
#define DT_PARENT(node_id) UTIL_CAT(node_id, _PARENT)

/**
 * @brief Get a DT_DRV_COMPAT parent's node identifier
 * @param inst instance number
 * @return a node identifier for the instance's parent
 */
#define DT_INST_PARENT(inst) DT_PARENT(DT_DRV_INST(inst))

/**
 * @brief Get a node identifier for a grandparent node
 *
 * Example devicetree fragment:
 *
 *     gparent: grandparent-node {
 *             parent: parent-node {
 *                     child: child-node { ... }
 *             };
 *     };
 *
 * The following are equivalent ways to get the same node identifier:
 *
 *     DT_GPARENT(DT_NODELABEL(child))
 *     DT_PARENT(DT_PARENT(DT_NODELABEL(child))
 *
 * @param node_id node identifier
 * @return a node identifier for the node's parent's parent
 */
#define DT_GPARENT(node_id) DT_PARENT(DT_PARENT(node_id))

/**
 * @brief Get a node identifier for a child node
 *
 * Example devicetree fragment:
 *
 *     / {
 *             soc-label: soc {
 *                     serial1: serial@40001000 {
 *                             status = "okay";
 *                             current-speed = <115200>;
 *                             ...
 *                     };
 *             };
 *     };
 *
 * Example usage with @ref DT_PROP() to get the status of the
 * serial@40001000 node:
 *
 *     #define SOC_NODE DT_NODELABEL(soc_label)
 *     DT_PROP(DT_CHILD(SOC_NODE, serial_40001000), status) // "okay"
 *
 * Node labels like "serial1" cannot be used as the "child" argument
 * to this macro. Use DT_NODELABEL() for that instead.
 *
 * You can also use DT_FOREACH_CHILD() to iterate over node
 * identifiers for all of a node's children.
 *
 * @param node_id node identifier
 * @param child lowercase-and-underscores child node name
 * @return node identifier for the node with the name referred to by 'child'
 */
#define DT_CHILD(node_id, child) UTIL_CAT(node_id, DT_S_PREFIX(child))

/**
 * @brief Get a node identifier for a status "okay" node with a compatible
 *
 * Use this if you want to get an arbitrary enabled node with a given
 * compatible, and you do not care which one you get. If any enabled
 * nodes with the given compatible exist, a node identifier for one
 * of them is returned. Otherwise, @p DT_INVALID_NODE is returned.
 *
 * Example devicetree fragment:
 *
 *	node-a {
 *		compatible = "vnd,device";
 *		status = "okay";
 *	};
 *
 *	node-b {
 *		compatible = "vnd,device";
 *		status = "okay";
 *	};
 *
 *	node-c {
 *		compatible = "vnd,device";
 *		status = "disabled";
 *	};
 *
 * Example usage:
 *
 *     DT_COMPAT_GET_ANY_STATUS_OKAY(vnd_device)
 *
 * This expands to a node identifier for either @p node-a or @p
 * node-b. It will not expand to a node identifier for @p node-c,
 * because that node does not have status "okay".
 *
 * @param compat lowercase-and-underscores compatible, without quotes
 * @return node identifier for a node with that compatible, or DT_INVALID_NODE
 */
#define DT_COMPAT_GET_ANY_STATUS_OKAY(compat)			\
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(compat),	\
		    (DT_INST(0, compat)),		\
		    (DT_INVALID_NODE))

/**
 * @brief Get a devicetree node's full path as a string literal
 *
 * This returns the path to a node from a node identifier. To get a
 * node identifier from path components instead, use DT_PATH().
 *
 * Example devicetree fragment:
 *
 *     / {
 *             soc {
 *                     node: my-node@12345678 { ... };
 *             };
 *     };
 *
 * Example usage:
 *
 *    DT_NODE_PATH(DT_NODELABEL(node)) // "/soc/my-node@12345678"
 *    DT_NODE_PATH(DT_PATH(soc))       // "/soc"
 *    DT_NODE_PATH(DT_ROOT)            // "/"
 *
 * @param node_id node identifier
 * @return the node's full path in the devicetree
 */
#define DT_NODE_PATH(node_id) DT_CAT(node_id, _PATH)

/**
 * @brief Get a devicetree node's name with unit-address as a string literal
 *
 * This returns the node name and unit-address from a node identifier.
 *
 * Example devicetree fragment:
 *
 *     / {
 *             soc {
 *                     node: my-node@12345678 { ... };
 *             };
 *     };
 *
 * Example usage:
 *
 *    DT_NODE_FULL_NAME(DT_NODELABEL(node)) // "my-node@12345678"
 *
 * @param node_id node identifier
 * @return the node's name with unit-address as a string in the devicetree
 */
#define DT_NODE_FULL_NAME(node_id) DT_CAT(node_id, _FULL_NAME)

/**
 * @brief Get a devicetree node's index into its parent's list of children
 *
 * Indexes are zero-based.
 *
 * It is an error to use this macro with the root node.
 *
 * Example devicetree fragment:
 *
 *     parent {
 *             c1: child-1 {};
 *             c2: child-2 {};
 *     };
 *
 * Example usage:
 *
 *     DT_NODE_CHILD_IDX(DT_NODELABEL(c1)) // 0
 *     DT_NODE_CHILD_IDX(DT_NODELABEL(c2)) // 1
 *
 * @param node_id node identifier
 * @return the node's index in its parent node's list of children
 */
#define DT_NODE_CHILD_IDX(node_id) DT_CAT(node_id, _CHILD_IDX)

/**
 * @brief Do node_id1 and node_id2 refer to the same node?
 *
 * Both "node_id1" and "node_id2" must be node identifiers for nodes
 * that exist in the devicetree (if unsure, you can check with
 * DT_NODE_EXISTS()).
 *
 * The expansion evaluates to 0 or 1, but may not be a literal integer
 * 0 or 1.
 *
 * @param node_id1 first node identifier
 * @param node_id2 second node identifier
 * @return an expression that evaluates to 1 if the node identifiers
 *         refer to the same node, and evaluates to 0 otherwise
 */
#define DT_SAME_NODE(node_id1, node_id2) \
	(DT_DEP_ORD(node_id1) == (DT_DEP_ORD(node_id2)))

/* Implementation note: distinct nodes have distinct node identifiers.
 * See include/devicetree/ordinals.h. */

/**
 * @}
 */

/**
 * @defgroup devicetree-generic-prop Property accessors
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get a devicetree property value
 *
 * For properties whose bindings have the following types, this macro
 * expands to:
 *
 * - string: a string literal
 * - boolean: 0 if the property is false, or 1 if it is true
 * - int: the property's value as an integer literal
 * - array, uint8-array, string-array: an initializer expression in braces,
 *   whose elements are integer or string literals (like {0, 1, 2},
 *   {"hello", "world"}, etc.)
 * - phandle: a node identifier for the node with that phandle
 *
 * A property's type is usually defined by its binding. In some
 * special cases, it has an assumed type defined by the devicetree
 * specification even when no binding is available: "compatible" has
 * type string-array, "status" and "label" have type string, and
 * "interrupt-controller" has type boolean.
 *
 * For other properties or properties with unknown type due to a
 * missing binding, behavior is undefined.
 *
 * For usage examples, see @ref DT_PATH(), @ref DT_ALIAS(), @ref
 * DT_NODELABEL(), and @ref DT_INST() above.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @return a representation of the property's value
 */
#define DT_PROP(node_id, prop) DT_CAT(node_id, _P_##prop)

/**
 * @brief Get a property's logical length
 *
 * Here, "length" is a number of elements, which may differ from the
 * property's size in bytes.
 *
 * The return value depends on the property's type:
 *
 * - for types array, string-array, and uint8-array, this expands
 *   to the number of elements in the array
 * - for type phandles, this expands to the number of phandles
 * - for type phandle-array, this expands to the number of
 *   phandle and specifier blocks in the property
 *
 * These properties are handled as special cases:
 *
 * - reg property: use DT_NUM_REGS(node_id) instead
 * - interrupts property: use DT_NUM_IRQS(node_id) instead
 *
 * It is an error to use this macro with the ranges, dma-ranges, reg
 * or interrupts properties.
 *
 * For other properties, behavior is undefined.
 *
 * @param node_id node identifier
 * @param prop a lowercase-and-underscores property with a logical length
 * @return the property's length
 */
#define DT_PROP_LEN(node_id, prop) DT_PROP(node_id, prop##_LEN)

/**
 * @brief Like DT_PROP_LEN(), but with a fallback to default_value
 *
 * If the property is defined (as determined by DT_NODE_HAS_PROP()),
 * this expands to DT_PROP_LEN(node_id, prop). The default_value
 * parameter is not expanded in this case.
 *
 * Otherwise, this expands to default_value.
 *
 * @param node_id node identifier
 * @param prop a lowercase-and-underscores property with a logical length
 * @param default_value a fallback value to expand to
 * @return the property's length or the given default value
 */
#define DT_PROP_LEN_OR(node_id, prop, default_value) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop), \
		    (DT_PROP_LEN(node_id, prop)), (default_value))

/**
 * @brief Is index "idx" valid for an array type property?
 *
 * If this returns 1, then DT_PROP_BY_IDX(node_id, prop, idx) or
 * DT_PHA_BY_IDX(node_id, prop, idx, ...) are valid at index "idx".
 * If it returns 0, it is an error to use those macros with that index.
 *
 * These properties are handled as special cases:
 *
 * - reg property: use DT_REG_HAS_IDX(node_id, idx) instead
 * - interrupts property: use DT_IRQ_HAS_IDX(node_id, idx) instead
 *
 * It is an error to use this macro with the reg or interrupts properties.
 *
 * @param node_id node identifier
 * @param prop a lowercase-and-underscores property with a logical length
 * @param idx index to check
 * @return An expression which evaluates to 1 if "idx" is a valid index
 *         into the given property, and 0 otherwise.
 */
#define DT_PROP_HAS_IDX(node_id, prop, idx) \
	IS_ENABLED(DT_CAT6(node_id, _P_, prop, _IDX_, idx, _EXISTS))

/**
 * @brief Is name "name" available in a foo-names property?
 *
 * This property is handled as special case:
 *
 * - interrupts property: use DT_IRQ_HAS_NAME(node_id, idx) instead
 *
 * It is an error to use this macro with the interrupts property.
 *
 * Example devicetree fragment:
 *
 *	nx: node-x {
 *		foos = <&bar xx yy>, <&baz xx zz>;
 *		foo-names = "event", "error";
 *		status = "okay";
 *	};
 *
 * Example usage:
 *
 *     DT_PROP_HAS_NAME(nx, foos, event)    // 1
 *     DT_PROP_HAS_NAME(nx, foos, failure)  // 0
 *
 * @param node_id node identifier
 * @param prop a lowercase-and-underscores "prop-names" type property
 * @param name a lowercase-and-underscores name to check
 * @return An expression which evaluates to 1 if "name" is an available
 *         name into the given property, and 0 otherwise.
 */
#define DT_PROP_HAS_NAME(node_id, prop, name) \
	IS_ENABLED(DT_CAT6(node_id, _P_, prop, _NAME_, name, _EXISTS))

/**
 * @brief Get the value at index "idx" in an array type property
 *
 * It might help to read the argument order as being similar to
 * "node->property[index]".
 *
 * When the property's binding has type array, string-array,
 * uint8-array, or phandles, this expands to the idx-th array element
 * as an integer, string literal, or node identifier respectively.
 *
 * These properties are handled as special cases:
 *
 * - reg property: use DT_REG_ADDR_BY_IDX() or DT_REG_SIZE_BY_IDX() instead
 * - interrupts property: use DT_IRQ_BY_IDX() instead
 *
 * For non-array properties, behavior is undefined.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return a representation of the idx-th element of the property
 */
#define DT_PROP_BY_IDX(node_id, prop, idx) DT_PROP(node_id, prop##_IDX_##idx)

/**
 * @brief Like DT_PROP(), but with a fallback to default_value
 *
 * If the value exists, this expands to DT_PROP(node_id, prop).
 * The default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to default_value.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return the property's value or default_value
 */
#define DT_PROP_OR(node_id, prop, default_value) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop), \
		    (DT_PROP(node_id, prop)), (default_value))

/**
 * @brief Equivalent to DT_PROP(node_id, label)
 *
 * This is a convenience for the Zephyr device API, which uses label
 * properties as device_get_binding() arguments.
 * @param node_id node identifier
 * @return node's label property value
 */
#define DT_LABEL(node_id) DT_PROP(node_id, label)

/**
 * @brief Get a property value's index into its enumeration values
 *
 * The return values start at zero.
 *
 * Example devicetree fragment:
 *
 *     usb1: usb@12340000 {
 *             maximum-speed = "full-speed";
 *     };
 *     usb2: usb@12341000 {
 *             maximum-speed = "super-speed";
 *     };
 *
 * Example bindings fragment:
 *
 *     properties:
 *       maximum-speed:
 *         type: string
 *         enum:
 *            - "low-speed"
 *            - "full-speed"
 *            - "high-speed"
 *            - "super-speed"
 *
 * Example usage:
 *
 *     DT_ENUM_IDX(DT_NODELABEL(usb1), maximum_speed) // 1
 *     DT_ENUM_IDX(DT_NODELABEL(usb2), maximum_speed) // 3
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @return zero-based index of the property's value in its enum: list
 */
#define DT_ENUM_IDX(node_id, prop) DT_PROP(node_id, prop##_ENUM_IDX)

/**
 * @brief Like DT_ENUM_IDX(), but with a fallback to a default enum index
 *
 * If the value exists, this expands to its zero based index value thanks to
 * DT_ENUM_IDX(node_id, prop).
 *
 * Otherwise, this expands to provided default index enum value.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param default_idx_value a fallback index value to expand to
 * @return zero-based index of the property's value in its enum if present,
 *         default_idx_value otherwise
 */
#define DT_ENUM_IDX_OR(node_id, prop, default_idx_value) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop), \
		    (DT_ENUM_IDX(node_id, prop)), (default_idx_value))

/**
 * @brief Get a string property's value as a token.
 *
 * This removes "the quotes" from a string property's value,
 * converting any non-alphanumeric characters to underscores. This can
 * be useful, for example, when programmatically using the value to
 * form a C variable or code.
 *
 * DT_STRING_TOKEN() can only be used for properties with string type.
 *
 * It is an error to use DT_STRING_TOKEN() in other circumstances.
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             prop = "foo";
 *     };
 *     n2: node-2 {
 *             prop = "FOO";
 *     }
 *     n3: node-3 {
 *             prop = "123 foo";
 *     };
 *
 * Example bindings fragment:
 *
 *     properties:
 *       prop:
 *         type: string
 *
 * Example usage:
 *
 *     DT_STRING_TOKEN(DT_NODELABEL(n1), prop) // foo
 *     DT_STRING_TOKEN(DT_NODELABEL(n2), prop) // FOO
 *     DT_STRING_TOKEN(DT_NODELABEL(n3), prop) // 123_foo
 *
 * Notice how:
 *
 * - Unlike C identifiers, the property values may begin with a
 *   number. It's the user's responsibility not to use such values as
 *   the name of a C identifier.
 *
 * - The uppercased "FOO" in the DTS remains @p FOO as a token. It is
 *   *not* converted to @p foo.
 *
 * - The whitespace in the DTS "123 foo" string is converted to @p
 *   123_foo as a token.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @return the value of @p prop as a token, i.e. without any quotes
 *         and with special characters converted to underscores
 */
#define DT_STRING_TOKEN(node_id, prop) \
	DT_CAT4(node_id, _P_, prop, _STRING_TOKEN)

/**
 * @brief Like DT_STRING_TOKEN(), but with a fallback to default_value
 *
 * If the value exists, this expands to DT_STRING_TOKEN(node_id, prop).
 * The default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to default_value.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return the property's value as a token, or @p default_value
 */
#define DT_STRING_TOKEN_OR(node_id, prop, default_value) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop), \
		(DT_STRING_TOKEN(node_id, prop)), (default_value))

/**
 * @brief Like DT_STRING_TOKEN(), but uppercased.
 *
 * This removes "the quotes" from a string property's value,
 * converting any non-alphanumeric characters to underscores, and
 * capitalizing the result. This can be useful, for example, when
 * programmatically using the value to form a C variable or code.
 *
 * DT_STRING_UPPER_TOKEN() can only be used for properties with string type.
 *
 * It is an error to use DT_STRING_UPPER_TOKEN() in other circumstances.
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             prop = "foo";
 *     };
 *     n2: node-2 {
 *             prop = "123 foo";
 *     };
 *
 * Example bindings fragment:
 *
 *     properties:
 *       prop:
 *         type: string
 *
 * Example usage:
 *
 *     DT_STRING_UPPER_TOKEN(DT_NODELABEL(n1), prop) // FOO
 *     DT_STRING_UPPER_TOKEN(DT_NODELABEL(n2), prop) // 123_FOO
 *
 * Notice how:
 *
 * - Unlike C identifiers, the property values may begin with a
 *   number. It's the user's responsibility not to use such values as
 *   the name of a C identifier.
 *
 * - The lowercased "foo" in the DTS becomes @p FOO as a token, i.e.
 *   it is uppercased.
 *
 * - The whitespace in the DTS "123 foo" string is converted to @p
 *   123_FOO as a token, i.e. it is uppercased and whitespace becomes
 *   an underscore.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @return the value of @p prop as an uppercased token, i.e. without
 *         any quotes and with special characters converted to underscores
 */
#define DT_STRING_UPPER_TOKEN(node_id, prop) \
	DT_CAT4(node_id, _P_, prop, _STRING_UPPER_TOKEN)

/**
 * @brief Like DT_STRING_UPPER_TOKEN(), but with a fallback to default_value
 *
 * If the value exists, this expands to DT_STRING_UPPER_TOKEN(node_id, prop).
 * The default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to default_value.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return the property's value as an uppercased token,
 *         or @p default_value
 */
#define DT_STRING_UPPER_TOKEN_OR(node_id, prop, default_value) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop), \
		(DT_STRING_UPPER_TOKEN(node_id, prop)), (default_value))

/*
 * phandle properties
 *
 * These are special-cased to manage the impedance mismatch between
 * phandles, which are just uint32_t node properties that only make sense
 * within the tree itself, and C values.
 */

/**
 * @brief Get a property value from a phandle in a property.
 *
 * This is a shorthand for:
 *
 *     DT_PROP(DT_PHANDLE_BY_IDX(node_id, phs, idx), prop)
 *
 * That is, "prop" is a property of the phandle's node, not a
 * property of "node_id".
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             foo = <&n2 &n3>;
 *     };
 *
 *     n2: node-2 {
 *             bar = <42>;
 *     };
 *
 *     n3: node-3 {
 *             baz = <43>;
 *     };
 *
 * Example usage:
 *
 *     #define N1 DT_NODELABEL(n1)
 *
 *     DT_PROP_BY_PHANDLE_IDX(N1, foo, 0, bar) // 42
 *     DT_PROP_BY_PHANDLE_IDX(N1, foo, 1, baz) // 43
 *
 * @param node_id node identifier
 * @param phs lowercase-and-underscores property with type "phandle",
 *            "phandles", or "phandle-array"
 * @param idx logical index into "phs", which must be zero if "phs"
 *            has type "phandle"
 * @param prop lowercase-and-underscores property of the phandle's node
 * @return the property's value
 */
#define DT_PROP_BY_PHANDLE_IDX(node_id, phs, idx, prop) \
	DT_PROP(DT_PHANDLE_BY_IDX(node_id, phs, idx), prop)

/**
 * @brief Like DT_PROP_BY_PHANDLE_IDX(), but with a fallback to
 * default_value.
 *
 * If the value exists, this expands to DT_PROP_BY_PHANDLE_IDX(node_id, phs,
 * idx, prop). The default_value parameter is not expanded in this
 * case.
 *
 * Otherwise, this expands to default_value.
 *
 * @param node_id node identifier
 * @param phs lowercase-and-underscores property with type "phandle",
 *            "phandles", or "phandle-array"
 * @param idx logical index into "phs", which must be zero if "phs"
 *            has type "phandle"
 * @param prop lowercase-and-underscores property of the phandle's node
 * @param default_value a fallback value to expand to
 * @return the property's value
 */
#define DT_PROP_BY_PHANDLE_IDX_OR(node_id, phs, idx, prop, default_value) \
	DT_PROP_OR(DT_PHANDLE_BY_IDX(node_id, phs, idx), prop, default_value)

/**
 * @brief Get a property value from a phandle's node
 *
 * This is equivalent to DT_PROP_BY_PHANDLE_IDX(node_id, ph, 0, prop).
 *
 * @param node_id node identifier
 * @param ph lowercase-and-underscores property of "node_id"
 *           with type "phandle"
 * @param prop lowercase-and-underscores property of the phandle's node
 * @return the property's value
 */
#define DT_PROP_BY_PHANDLE(node_id, ph, prop) \
	DT_PROP_BY_PHANDLE_IDX(node_id, ph, 0, prop)

/**
 * @brief Get a phandle-array specifier cell value at an index
 *
 * It might help to read the argument order as being similar to
 * "node->phandle_array[index].cell". That is, the cell value is in
 * the "pha" property of "node_id", inside the specifier at index
 * "idx".
 *
 * Example devicetree fragment:
 *
 *     gpio0: gpio@... {
 *             #gpio-cells = <2>;
 *     };
 *
 *     gpio1: gpio@... {
 *             #gpio-cells = <2>;
 *     };
 *
 *     led: led_0 {
 *             gpios = <&gpio0 17 0x1>, <&gpio1 5 0x3>;
 *     };
 *
 * Bindings fragment for the gpio0 and gpio1 nodes:
 *
 *     gpio-cells:
 *       - pin
 *       - flags
 *
 * Above, "gpios" has two elements:
 *
 * - index 0 has specifier <17 0x1>, so its "pin" cell is 17, and its
 *   "flags" cell is 0x1
 * - index 1 has specifier <5 0x3>, so "pin" is 5 and "flags" is 0x3
 *
 * Example usage:
 *
 *     #define LED DT_NODELABEL(led)
 *
 *     DT_PHA_BY_IDX(LED, gpios, 0, pin)   // 17
 *     DT_PHA_BY_IDX(LED, gpios, 1, flags) // 0x3
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param idx logical index into "pha"
 * @param cell lowercase-and-underscores cell name within the specifier
 *             at "pha" index "idx"
 * @return the cell's value
 */
#define DT_PHA_BY_IDX(node_id, pha, idx, cell) \
	DT_PROP(node_id, pha##_IDX_##idx##_VAL_##cell)

/**
 * @brief Like DT_PHA_BY_IDX(), but with a fallback to default_value.
 *
 * If the value exists, this expands to DT_PHA_BY_IDX(node_id, pha,
 * idx, cell). The default_value parameter is not expanded in this
 * case.
 *
 * Otherwise, this expands to default_value.
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param idx logical index into "pha"
 * @param cell lowercase-and-underscores cell name within the specifier
 *             at "pha" index "idx"
 * @param default_value a fallback value to expand to
 * @return the cell's value or "default_value"
 */
#define DT_PHA_BY_IDX_OR(node_id, pha, idx, cell, default_value) \
	DT_PROP_OR(node_id, pha##_IDX_##idx##_VAL_##cell, default_value)
/* Implementation note: the _IDX_##idx##_VAL_##cell##_EXISTS
 * macros are defined, so it's safe to use DT_PROP_OR() here, because
 * that uses an IS_ENABLED() on the _EXISTS macro.
 */

/**
 * @brief Equivalent to DT_PHA_BY_IDX(node_id, pha, 0, cell)
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param cell lowercase-and-underscores cell name
 * @return the cell's value
 */
#define DT_PHA(node_id, pha, cell) DT_PHA_BY_IDX(node_id, pha, 0, cell)

/**
 * @brief Like DT_PHA(), but with a fallback to default_value
 *
 * If the value exists, this expands to DT_PHA(node_id, pha, cell).
 * The default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to default_value.
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param cell lowercase-and-underscores cell name
 * @param default_value a fallback value to expand to
 * @return the cell's value or default_value
 */
#define DT_PHA_OR(node_id, pha, cell, default_value) \
	DT_PHA_BY_IDX_OR(node_id, pha, 0, cell, default_value)

/**
 * @brief Get a value within a phandle-array specifier by name
 *
 * This is like DT_PHA_BY_IDX(), except it treats "pha" as a structure
 * where each array element has a name.
 *
 * It might help to read the argument order as being similar to
 * "node->phandle_struct.name.cell". That is, the cell value is in the
 * "pha" property of "node_id", treated as a data structure where
 * each array element has a name.
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             io-channels = <&adc1 10>, <&adc2 20>;
 *             io-channel-names = "SENSOR", "BANDGAP";
 *     };
 *
 * Bindings fragment for the "adc1" and "adc2" nodes:
 *
 *     io-channel-cells:
 *       - input
 *
 * Example usage:
 *
 *     DT_PHA_BY_NAME(DT_NODELABEL(n), io_channels, sensor, input)  // 10
 *     DT_PHA_BY_NAME(DT_NODELABEL(n), io_channels, bandgap, input) // 20
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param name lowercase-and-underscores name of a specifier in "pha"
 * @param cell lowercase-and-underscores cell name in the named specifier
 * @return the cell's value
 */
#define DT_PHA_BY_NAME(node_id, pha, name, cell) \
	DT_PROP(node_id, pha##_NAME_##name##_VAL_##cell)

/**
 * @brief Like DT_PHA_BY_NAME(), but with a fallback to default_value
 *
 * If the value exists, this expands to DT_PHA_BY_NAME(node_id, pha,
 * name, cell). The default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to default_value.
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param name lowercase-and-underscores name of a specifier in "pha"
 * @param cell lowercase-and-underscores cell name in the named specifier
 * @param default_value a fallback value to expand to
 * @return the cell's value or default_value
 */
#define DT_PHA_BY_NAME_OR(node_id, pha, name, cell, default_value) \
	DT_PROP_OR(node_id, pha##_NAME_##name##_VAL_##cell, default_value)
/* Implementation note: the _NAME_##name##_VAL_##cell##_EXISTS
 * macros are defined, so it's safe to use DT_PROP_OR() here, because
 * that uses an IS_ENABLED() on the _EXISTS macro.
 */

/**
 * @brief Get a phandle's node identifier from a phandle array by name
 *
 * It might help to read the argument order as being similar to
 * "node->phandle_struct.name.phandle". That is, the phandle array is
 * treated as a structure with named elements. The return value is
 * the node identifier for a phandle inside the structure.
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
 * Above, "io-channels" has two elements:
 *
 * - the element named "SENSOR" has phandle &adc1
 * - the element named "BANDGAP" has phandle &adc2
 *
 * Example usage:
 *
 *     #define NODE DT_NODELABEL(n)
 *
 *     DT_LABEL(DT_PHANDLE_BY_NAME(NODE, io_channels, sensor))  // "ADC_1"
 *     DT_LABEL(DT_PHANDLE_BY_NAME(NODE, io_channels, bandgap)) // "ADC_2"
 *
 * Notice how devicetree properties and names are lowercased, and
 * non-alphanumeric characters are converted to underscores.
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param name lowercase-and-underscores name of an element in "pha"
 * @return a node identifier for the node with that phandle
 */
#define DT_PHANDLE_BY_NAME(node_id, pha, name) \
	DT_PROP(node_id, pha##_NAME_##name##_PH)

/**
 * @brief Get a node identifier for a phandle in a property.
 *
 * When a node's value at a logical index contains a phandle, this
 * macro returns a node identifier for the node with that phandle.
 *
 * Therefore, if "prop" has type "phandle", "idx" must be zero. (A
 * "phandle" type is treated as a "phandles" with a fixed length of
 * 1).
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             foo = <&n2 &n3>;
 *     };
 *
 *     n2: node-2 { ... };
 *     n3: node-3 { ... };
 *
 * Above, "foo" has type phandles and has two elements:
 *
 * - index 0 has phandle &n2, which is node-2's phandle
 * - index 1 has phandle &n3, which is node-3's phandle
 *
 * Example usage:
 *
 *     #define N1 DT_NODELABEL(n1)
 *
 *     DT_PHANDLE_BY_IDX(N1, foo, 0) // node identifier for node-2
 *     DT_PHANDLE_BY_IDX(N1, foo, 1) // node identifier for node-3
 *
 * Behavior is analogous for phandle-arrays.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name in "node_id"
 *             with type "phandle", "phandles" or "phandle-array"
 * @param idx index into "prop"
 * @return node identifier for the node with the phandle at that index
 */
#define DT_PHANDLE_BY_IDX(node_id, prop, idx) \
	DT_CAT6(node_id, _P_, prop, _IDX_, idx, _PH)
/*
 * Implementation note: using DT_CAT6 above defers concatenation until
 * after expansion of each parameter. This is important when 'idx' is
 * expandable to a number, but it isn't one "yet".
 */

/**
 * @brief Get a node identifier for a phandle property's value
 *
 * This is equivalent to DT_PHANDLE_BY_IDX(node_id, prop, 0). Its primary
 * benefit is readability when "prop" has type "phandle".
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property of "node_id"
 *             with type "phandle"
 * @return a node identifier for the node pointed to by "ph"
 */
#define DT_PHANDLE(node_id, prop) DT_PHANDLE_BY_IDX(node_id, prop, 0)

/**
 * @}
 */

/**
 * @defgroup devicetree-ranges-prop ranges property
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get the number of range blocks in the ranges property
 *
 * Use this instead of DT_PROP_LEN(node_id, ranges).
 *
 * Example devicetree fragment:
 *
 *     pcie0: pcie@0 {
 *             compatible = "intel,pcie";
 *             reg = <0 1>;
 *             #address-cells = <3>;
 *             #size-cells = <2>;
 *
 *             ranges = <0x1000000 0 0 0 0x3eff0000 0 0x10000>,
 *                      <0x2000000 0 0x10000000 0 0x10000000 0 0x2eff0000>,
 *                      <0x3000000 0x80 0 0x80 0 0x80 0>;
 *     };
 *
 *     other: other@1 {
 *             reg = <1 1>;
 *
 *             ranges = <0x0 0x0 0x0 0x3eff0000 0x10000>,
 *                      <0x0 0x10000000 0x0 0x10000000 0x2eff0000>;
 *     };
 *
 * Example usage:
 *
 *     DT_NUM_RANGES(DT_NODELABEL(pcie0)) // 3
 *     DT_NUM_RANGES(DT_NODELABEL(other)) // 2
 *
 * @param node_id node identifier
 */
#define DT_NUM_RANGES(node_id) DT_CAT(node_id, _RANGES_NUM)

/**
 * @brief Is "idx" a valid range block index?
 *
 * If this returns 1, then DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(node_id, idx),
 * DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(node_id, idx) or
 * DT_RANGES_LENGTH_BY_IDX(node_id, idx) are valid.
 * For DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(node_id, idx) the return value
 * of DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(node_id, idx) will indicate
 * validity.
 * If it returns 0, it is an error to use those macros with index "idx",
 * including DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(node_id, idx).
 *
 * Example devicetree fragment:
 *
 *
 *     pcie0: pcie@0 {
 *             compatible = "intel,pcie";
 *             reg = <0 1>;
 *             #address-cells = <3>;
 *             #size-cells = <2>;
 *
 *             ranges = <0x1000000 0 0 0 0x3eff0000 0 0x10000>,
 *                      <0x2000000 0 0x10000000 0 0x10000000 0 0x2eff0000>,
 *                      <0x3000000 0x80 0 0x80 0 0x80 0>;
 *     };
 *
 *     other: other@1 {
 *             reg = <1 1>;
 *
 *             ranges = <0x0 0x0 0x0 0x3eff0000 0x10000>,
 *                      <0x0 0x10000000 0x0 0x10000000 0x2eff0000>;
 *     };
 *
 * Example usage:
 *
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(pcie0), 0) // 1
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(pcie0), 1) // 1
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(pcie0), 2) // 1
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(pcie0), 3) // 0
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(other), 0) // 1
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(other), 1) // 1
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(other), 2) // 0
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(other), 3) // 0
 *
 * @param node_id node identifier
 * @param idx index to check
 * @return 1 if "idx" is a valid register block index,
 *         0 otherwise.
 */
#define DT_RANGES_HAS_IDX(node_id, idx) \
	IS_ENABLED(DT_CAT4(node_id, _RANGES_IDX_, idx, _EXISTS))

/**
 * @brief Does a ranges property have child bus flags at index?
 *
 * If this returns 1, then DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(node_id, idx) is valid.
 * If it returns 0, it is an error to use this macro with index "idx".
 * This macro only returns 1 for PCIe buses (i.e. nodes whose bindings specify they
 * are "pcie" bus nodes.)
 *
 * Example devicetree fragment:
 *
 *     parent {
 *             #address-cells = <2>;
 *
 *             pcie0: pcie@0 {
 *                     compatible = "intel,pcie";
 *                     reg = <0 0 1>;
 *                     #address-cells = <3>;
 *                     #size-cells = <2>;
 *
 *                     ranges = <0x1000000 0 0 0 0x3eff0000 0 0x10000>,
 *                              <0x2000000 0 0x10000000 0 0x10000000 0 0x2eff0000>,
 *                              <0x3000000 0x80 0 0x80 0 0x80 0>;
 *             };
 *
 *             other: other@1 {
 *                     reg = <0 1 1>;
 *
 *                     ranges = <0x0 0x0 0x0 0x3eff0000 0x10000>,
 *                              <0x0 0x10000000 0x0 0x10000000 0x2eff0000>;
 *             };
 *     };
 *
 * Example usage:
 *
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(pcie0), 0) // 1
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(pcie0), 1) // 1
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(pcie0), 2) // 1
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(pcie0), 3) // 0
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(other), 0) // 0
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(other), 1) // 0
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(other), 2) // 0
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(other), 3) // 0
 *
 * @param node_id node identifier
 * @param idx logical index into the ranges array
 * @return 1 if "idx" is a valid child bus flags index,
 *         0 otherwise.
 */
#define DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(node_id, idx) \
	IS_ENABLED(DT_CAT4(node_id, _RANGES_IDX_, idx, _VAL_CHILD_BUS_FLAGS_EXISTS))

/**
 * @brief Get the ranges property child bus flags at index
 *
 * When the node is a PCIe bus, the Child Bus Address has an extra cell used to store some
 * flags, thus this cell is extracted from the Child Bus Address as Child Bus Flags field.
 *
 * Example devicetree fragments:
 *
 *     parent {
 *             #address-cells = <2>;
 *
 *             pcie0: pcie@0 {
 *                     compatible = "intel,pcie";
 *                     reg = <0 0 1>;
 *                     #address-cells = <3>;
 *                     #size-cells = <2>;
 *
 *                     ranges = <0x1000000 0 0 0 0x3eff0000 0 0x10000>,
 *                              <0x2000000 0 0x10000000 0 0x10000000 0 0x2eff0000>,
 *                              <0x3000000 0x80 0 0x80 0 0x80 0>;
 *             };
 *     };
 *
 * Example usage:
 *
 *     DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(DT_NODELABEL(pcie0), 0) // 0x1000000
 *     DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(DT_NODELABEL(pcie0), 1) // 0x2000000
 *     DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(DT_NODELABEL(pcie0), 2) // 0x3000000
 *
 * @param node_id node identifier
 * @param idx logical index into the ranges array
 * @returns range child bus flags field at idx
 */
#define DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(node_id, idx) \
	DT_CAT4(node_id, _RANGES_IDX_, idx, _VAL_CHILD_BUS_FLAGS)

/**
 * @brief Get the ranges property child bus address at index
 *
 * When the node is a PCIe bus, the Child Bus Address has an extra cell used to store some
 * flags, thus this cell is removed from the Child Bus Address.
 *
 * Example devicetree fragments:
 *
 *     parent {
 *             #address-cells = <2>;
 *
 *             pcie0: pcie@0 {
 *                     compatible = "intel,pcie";
 *                     reg = <0 0 1>;
 *                     #address-cells = <3>;
 *                     #size-cells = <2>;
 *
 *                     ranges = <0x1000000 0 0 0 0x3eff0000 0 0x10000>,
 *                              <0x2000000 0 0x10000000 0 0x10000000 0 0x2eff0000>,
 *                              <0x3000000 0x80 0 0x80 0 0x80 0>;
 *             };
 *
 *             other: other@1 {
 *                     reg = <0 1 1>;
 *
 *                     ranges = <0x0 0x0 0x0 0x3eff0000 0x10000>,
 *                              <0x0 0x10000000 0x0 0x10000000 0x2eff0000>;
 *             };
 *     };
 *
 * Example usage:
 *
 *     DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_NODELABEL(pcie0), 0) // 0
 *     DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_NODELABEL(pcie0), 1) // 0x10000000
 *     DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_NODELABEL(pcie0), 2) // 0x8000000000
 *     DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_NODELABEL(other), 0) // 0
 *     DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_NODELABEL(other), 1) // 0x10000000
 *
 * @param node_id node identifier
 * @param idx logical index into the ranges array
 * @returns range child bus address field at idx
 */
#define DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(node_id, idx) \
	DT_CAT4(node_id, _RANGES_IDX_, idx, _VAL_CHILD_BUS_ADDRESS)

/**
 * @brief Get the ranges property parent bus address at index
 *
 * Similarly to DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(), this properly accounts
 * for child bus flags cells when the node is a PCIe bus.
 *
 * Example devicetree fragment:
 *
 *     parent {
 *             #address-cells = <2>;
 *
 *             pcie0: pcie@0 {
 *                     compatible = "intel,pcie";
 *                     reg = <0 0 1>;
 *                     #address-cells = <3>;
 *                     #size-cells = <2>;
 *
 *                     ranges = <0x1000000 0 0 0 0x3eff0000 0 0x10000>,
 *                              <0x2000000 0 0x10000000 0 0x10000000 0 0x2eff0000>,
 *                              <0x3000000 0x80 0 0x80 0 0x80 0>;
 *             };
 *
 *             other: other@1 {
 *                     reg = <0 1 1>;
 *
 *                     ranges = <0x0 0x0 0x0 0x3eff0000 0x10000>,
 *                              <0x0 0x10000000 0x0 0x10000000 0x2eff0000>;
 *             };
 *     };
 *
 * Example usage:
 *
 *     DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(DT_NODELABEL(pcie0), 0) // 0x3eff0000
 *     DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(DT_NODELABEL(pcie0), 1) // 0x10000000
 *     DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(DT_NODELABEL(pcie0), 2) // 0x8000000000
 *     DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(DT_NODELABEL(other), 0) // 0x3eff0000
 *     DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(DT_NODELABEL(other), 1) // 0x10000000
 *
 * @param node_id node identifier
 * @param idx logical index into the ranges array
 * @returns range parent bus address field at idx
 */
#define DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(node_id, idx) \
	DT_CAT4(node_id, _RANGES_IDX_, idx, _VAL_PARENT_BUS_ADDRESS)

/**
 * @brief Get the ranges property length at index
 *
 * Similarly to DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(), this properly accounts
 * for child bus flags cells when the node is a PCIe bus.
 *
 * Example devicetree fragment:
 *
 *     parent {
 *             #address-cells = <2>;
 *
 *             pcie0: pcie@0 {
 *                     compatible = "intel,pcie";
 *                     reg = <0 0 1>;
 *                     #address-cells = <3>;
 *                     #size-cells = <2>;
 *
 *                     ranges = <0x1000000 0 0 0 0x3eff0000 0 0x10000>,
 *                              <0x2000000 0 0x10000000 0 0x10000000 0 0x2eff0000>,
 *                              <0x3000000 0x80 0 0x80 0 0x80 0>;
 *             };
 *
 *             other: other@1 {
 *                     reg = <0 1 1>;
 *
 *                     ranges = <0x0 0x0 0x0 0x3eff0000 0x10000>,
 *                              <0x0 0x10000000 0x0 0x10000000 0x2eff0000>;
 *             };
 *     };
 *
 * Example usage:
 *
 *     DT_RANGES_LENGTH_BY_IDX(DT_NODELABEL(pcie0), 0) // 0x10000
 *     DT_RANGES_LENGTH_BY_IDX(DT_NODELABEL(pcie0), 1) // 0x2eff0000
 *     DT_RANGES_LENGTH_BY_IDX(DT_NODELABEL(pcie0), 2) // 0x8000000000
 *     DT_RANGES_LENGTH_BY_IDX(DT_NODELABEL(other), 0) // 0x10000
 *     DT_RANGES_LENGTH_BY_IDX(DT_NODELABEL(other), 1) // 0x2eff0000
 *
 * @param node_id node identifier
 * @param idx logical index into the ranges array
 * @returns range length field at idx
 */
#define DT_RANGES_LENGTH_BY_IDX(node_id, idx) \
	DT_CAT4(node_id, _RANGES_IDX_, idx, _VAL_LENGTH)

/**
 * @brief Invokes "fn" for each entry of "node_id" ranges property
 *
 * The macro "fn" must take two parameters, "node_id" which will be the node
 * identifier of the node with the ranges property and "idx" the index of
 * the ranges block.
 *
 * Example devicetree fragment:
 *
 *     n: node@0 {
 *             reg = <0 0 1>;
 *
 *             ranges = <0x0 0x0 0x0 0x3eff0000 0x10000>,
 *                      <0x0 0x10000000 0x0 0x10000000 0x2eff0000>;
 *     };
 *
 * Example usage:
 *
 *     #define RANGE_LENGTH(node_id, idx) DT_RANGES_LENGTH_BY_IDX(node_id, idx),
 *
 *     const uint64_t *ranges_length[] = {
 *             DT_FOREACH_RANGE(DT_NODELABEL(n), RANGE_LENGTH)
 *     };
 *
 * This expands to:
 *
 *     const char *ranges_length[] = {
 *         0x10000, 0x2eff0000,
 *     };
 *
 * @param node_id node identifier
 * @param fn macro to invoke
 */
#define DT_FOREACH_RANGE(node_id, fn) \
	DT_CAT(node_id, _FOREACH_RANGE)(fn)

/**
 * @}
 */

/**
 * @defgroup devicetree-reg-prop reg property
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get the number of register blocks in the reg property
 *
 * Use this instead of DT_PROP_LEN(node_id, reg).
 * @param node_id node identifier
 * @return Number of register blocks in the node's "reg" property.
 */
#define DT_NUM_REGS(node_id) DT_CAT(node_id, _REG_NUM)

/**
 * @brief Is "idx" a valid register block index?
 *
 * If this returns 1, then DT_REG_ADDR_BY_IDX(node_id, idx) or
 * DT_REG_SIZE_BY_IDX(node_id, idx) are valid.
 * If it returns 0, it is an error to use those macros with index "idx".
 * @param node_id node identifier
 * @param idx index to check
 * @return 1 if "idx" is a valid register block index,
 *         0 otherwise.
 */
#define DT_REG_HAS_IDX(node_id, idx) \
	IS_ENABLED(DT_CAT(node_id, _REG_IDX_##idx##_EXISTS))

/**
 * @brief Get the base address of the register block at index "idx"
 * @param node_id node identifier
 * @param idx index of the register whose address to return
 * @return address of the idx-th register block
 */
#define DT_REG_ADDR_BY_IDX(node_id, idx) \
	DT_CAT(node_id, _REG_IDX_##idx##_VAL_ADDRESS)

/**
 * @brief Get the size of the register block at index "idx"
 *
 * This is the size of an individual register block, not the total
 * number of register blocks in the property; use DT_NUM_REGS() for
 * that.
 *
 * @param node_id node identifier
 * @param idx index of the register whose size to return
 * @return size of the idx-th register block
 */
#define DT_REG_SIZE_BY_IDX(node_id, idx) \
	DT_CAT(node_id, _REG_IDX_##idx##_VAL_SIZE)

/**
 * @brief Get a node's (only) register block address
 *
 * Equivalent to DT_REG_ADDR_BY_IDX(node_id, 0).
 * @param node_id node identifier
 * @return node's register block address
 */
#define DT_REG_ADDR(node_id) DT_REG_ADDR_BY_IDX(node_id, 0)

/**
 * @brief Get a node's (only) register block size
 *
 * Equivalent to DT_REG_SIZE_BY_IDX(node_id, 0).
 * @param node_id node identifier
 * @return node's only register block's size
 */
#define DT_REG_SIZE(node_id) DT_REG_SIZE_BY_IDX(node_id, 0)

/**
 * @brief Get a register block's base address by name
 * @param node_id node identifier
 * @param name lowercase-and-underscores register specifier name
 * @return address of the register block specified by name
 */
#define DT_REG_ADDR_BY_NAME(node_id, name) \
	DT_CAT(node_id, _REG_NAME_##name##_VAL_ADDRESS)

/**
 * @brief Get a register block's size by name
 * @param node_id node identifier
 * @param name lowercase-and-underscores register specifier name
 * @return size of the register block specified by name
 */
#define DT_REG_SIZE_BY_NAME(node_id, name) \
	DT_CAT(node_id, _REG_NAME_##name##_VAL_SIZE)

/**
 * @}
 */

/**
 * @defgroup devicetree-interrupts-prop interrupts property
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get the number of interrupt sources for the node
 *
 * Use this instead of DT_PROP_LEN(node_id, interrupts).
 *
 * @param node_id node identifier
 * @return Number of interrupt specifiers in the node's "interrupts" property.
 */
#define DT_NUM_IRQS(node_id) DT_CAT(node_id, _IRQ_NUM)

/**
 * @brief Is "idx" a valid interrupt index?
 *
 * If this returns 1, then DT_IRQ_BY_IDX(node_id, idx) is valid.
 * If it returns 0, it is an error to use that macro with this index.
 * @param node_id node identifier
 * @param idx index to check
 * @return 1 if the idx is valid for the interrupt property
 *         0 otherwise.
 */
#define DT_IRQ_HAS_IDX(node_id, idx) \
	IS_ENABLED(DT_CAT(node_id, _IRQ_IDX_##idx##_EXISTS))

/**
 * @brief Does an interrupts property have a named cell specifier at an index?
 * If this returns 1, then DT_IRQ_BY_IDX(node_id, idx, cell) is valid.
 * If it returns 0, it is an error to use that macro.
 * @param node_id node identifier
 * @param idx index to check
 * @param cell named cell value whose existence to check
 * @return 1 if the named cell exists in the interrupt specifier at index idx
 *         0 otherwise.
 */
#define DT_IRQ_HAS_CELL_AT_IDX(node_id, idx, cell) \
	IS_ENABLED(DT_CAT(node_id, _IRQ_IDX_##idx##_VAL_##cell##_EXISTS))

/**
 * @brief Equivalent to DT_IRQ_HAS_CELL_AT_IDX(node_id, 0, cell)
 * @param node_id node identifier
 * @param cell named cell value whose existence to check
 * @return 1 if the named cell exists in the interrupt specifier at index 0
 *         0 otherwise.
 */
#define DT_IRQ_HAS_CELL(node_id, cell) DT_IRQ_HAS_CELL_AT_IDX(node_id, 0, cell)

/**
 * @brief Does an interrupts property have a named specifier value at an index?
 * If this returns 1, then DT_IRQ_BY_NAME(node_id, name, cell) is valid.
 * If it returns 0, it is an error to use that macro.
 * @param node_id node identifier
 * @param name lowercase-and-underscores interrupt specifier name
 * @return 1 if "name" is a valid named specifier
 *         0 otherwise.
 */
#define DT_IRQ_HAS_NAME(node_id, name) \
	IS_ENABLED(DT_CAT(node_id, _IRQ_NAME_##name##_VAL_irq_EXISTS))

/**
 * @brief Get a value within an interrupt specifier at an index
 *
 * It might help to read the argument order as being similar to
 * "node->interrupts[index].cell".
 *
 * This can be used to get information about an individual interrupt
 * when a device generates more than one.
 *
 * Example devicetree fragment:
 *
 *     my-serial: serial@... {
 *             interrupts = < 33 0 >, < 34 1 >;
 *     };
 *
 * Assuming the node's interrupt domain has "#interrupt-cells = <2>;" and
 * the individual cells in each interrupt specifier are named "irq" and
 * "priority" by the node's binding, here are some examples:
 *
 *     #define SERIAL DT_NODELABEL(my_serial)
 *
 *     Example usage                       Value
 *     -------------                       -----
 *     DT_IRQ_BY_IDX(SERIAL, 0, irq)          33
 *     DT_IRQ_BY_IDX(SERIAL, 0, priority)      0
 *     DT_IRQ_BY_IDX(SERIAL, 1, irq,          34
 *     DT_IRQ_BY_IDX(SERIAL, 1, priority)      1
 *
 * @param node_id node identifier
 * @param idx logical index into the interrupt specifier array
 * @param cell cell name specifier
 * @return the named value at the specifier given by the index
 */
#define DT_IRQ_BY_IDX(node_id, idx, cell)   \
	DT_CAT(node_id, _IRQ_IDX_##idx##_VAL_##cell)

/**
 * @brief Get a value within an interrupt specifier by name
 *
 * It might help to read the argument order as being similar to
 * "node->interrupts.name.cell".
 *
 * This can be used to get information about an individual interrupt
 * when a device generates more than one, if the bindings give each
 * interrupt specifier a name.
 *
 * @param node_id node identifier
 * @param name lowercase-and-underscores interrupt specifier name
 * @param cell cell name specifier
 * @return the named value at the specifier given by the index
 */
#define DT_IRQ_BY_NAME(node_id, name, cell) \
	DT_CAT(node_id, _IRQ_NAME_##name##_VAL_##cell)

/**
 * @brief Get an interrupt specifier's value
 * Equivalent to DT_IRQ_BY_IDX(node_id, 0, cell).
 * @param node_id node identifier
 * @param cell cell name specifier
 * @return the named value at that index
 */
#define DT_IRQ(node_id, cell) DT_IRQ_BY_IDX(node_id, 0, cell)

/**
 * @brief Get a node's (only) irq number
 *
 * Equivalent to DT_IRQ(node_id, irq). This is provided as a convenience
 * for the common case where a node generates exactly one interrupt,
 * and the IRQ number is in a cell named "irq".
 *
 * @param node_id node identifier
 * @return the interrupt number for the node's only interrupt
 */
#define DT_IRQN(node_id) DT_IRQ(node_id, irq)

/**
 * @}
 */

/**
 * @defgroup devicetree-generic-chosen Chosen nodes
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get a node identifier for a /chosen node property
 *
 * This is only valid to call if DT_HAS_CHOSEN(prop) is 1.
 * @param prop lowercase-and-underscores property name for
 *             the /chosen node
 * @return a node identifier for the chosen node property
 */
#define DT_CHOSEN(prop) DT_CAT(DT_CHOSEN_, prop)

/**
 * @brief Test if the devicetree has a /chosen node
 * @param prop lowercase-and-underscores devicetree property
 * @return 1 if the chosen property exists and refers to a node,
 *         0 otherwise
 */
#define DT_HAS_CHOSEN(prop) IS_ENABLED(DT_CHOSEN_##prop##_EXISTS)

/**
 * @}
 */

/**
 * @defgroup devicetree-generic-foreach "For-each" macros
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Invokes "fn" for each child of "node_id"
 *
 * The macro "fn" must take one parameter, which will be the node
 * identifier of a child node of "node_id".
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             child-1 {
 *                     label = "foo";
 *             };
 *             child-2 {
 *                     label = "bar";
 *             };
 *     };
 *
 * Example usage:
 *
 *     #define LABEL_AND_COMMA(node_id) DT_LABEL(node_id),
 *
 *     const char *child_labels[] = {
 *         DT_FOREACH_CHILD(DT_NODELABEL(n), LABEL_AND_COMMA)
 *     };
 *
 * This expands to:
 *
 *     const char *child_labels[] = {
 *         "foo", "bar",
 *     };
 *
 * @param node_id node identifier
 * @param fn macro to invoke
 */
#define DT_FOREACH_CHILD(node_id, fn) \
	DT_CAT(node_id, _FOREACH_CHILD)(fn)

/**
 * @brief Invokes "fn" for each child of "node_id" with multiple arguments
 *
 * The macro "fn" takes multiple arguments. The first should be the node
 * identifier for the child node. The remaining are passed-in by the caller.
 *
 * @param node_id node identifier
 * @param fn macro to invoke
 * @param ... variable number of arguments to pass to fn
 *
 * @see DT_FOREACH_CHILD
 */
#define DT_FOREACH_CHILD_VARGS(node_id, fn, ...) \
	DT_CAT(node_id, _FOREACH_CHILD_VARGS)(fn, __VA_ARGS__)

/**
 * @brief Call "fn" on the child nodes with status "okay"
 *
 * The macro "fn" should take one argument, which is the node
 * identifier for the child node.
 *
 * As usual, both a missing status and an "ok" status are
 * treated as "okay".
 *
 * @param node_id node identifier
 * @param fn macro to invoke
 */
#define DT_FOREACH_CHILD_STATUS_OKAY(node_id, fn) \
	DT_CAT(node_id, _FOREACH_CHILD_STATUS_OKAY)(fn)

/**
 * @brief Call "fn" on the child nodes with status "okay" with multiple
 * arguments
 *
 * The macro "fn" takes multiple arguments. The first should be the node
 * identifier for the child node. The remaining are passed-in by the caller.
 *
 * As usual, both a missing status and an "ok" status are
 * treated as "okay".
 *
 * @param node_id node identifier
 * @param fn macro to invoke
 * @param ... variable number of arguments to pass to fn
 *
 * @see DT_FOREACH_CHILD_STATUS_OKAY
 */
#define DT_FOREACH_CHILD_STATUS_OKAY_VARGS(node_id, fn, ...) \
	DT_CAT(node_id, _FOREACH_CHILD_STATUS_OKAY_VARGS)(fn, __VA_ARGS__)

/**
 * @brief Invokes "fn" for each element in the value of property "prop".
 *
 * The macro "fn" must take three parameters: fn(node_id, prop, idx).
 * "node_id" and "prop" are the same as what is passed to
 * DT_FOREACH_PROP_ELEM, and "idx" is the current index into the array.
 * The "idx" values are integer literals starting from 0.
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             my-ints = <1 2 3>;
 *     };
 *
 * Example usage:
 *
 *     #define TIMES_TWO(node_id, prop, idx) \
 *	       (2 * DT_PROP_BY_IDX(node_id, prop, idx)),
 *
 *     int array[] = {
 *             DT_FOREACH_PROP_ELEM(DT_NODELABEL(n), my_ints, TIMES_TWO)
 *     };
 *
 * This expands to:
 *
 *     int array[] = {
 *             (2 * 1), (2 * 2), (2 * 3),
 *     };
 *
 * In general, this macro expands to:
 *
 *     fn(node_id, prop, 0) fn(node_id, prop, 1) [...] fn(node_id, prop, n-1)
 *
 * where "n" is the number of elements in "prop", as it would be
 * returned by <tt>DT_PROP_LEN(node_id, prop)</tt>.
 *
 * The "prop" argument must refer to a property with type string,
 * array, uint8-array, string-array, phandles, or phandle-array. It is
 * an error to use this macro with properties of other types.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 */
#define DT_FOREACH_PROP_ELEM(node_id, prop, fn)		\
	DT_CAT4(node_id, _P_, prop, _FOREACH_PROP_ELEM)(fn)

/**
 * @brief Invokes "fn" for each element in the value of property "prop" with
 * multiple arguments.
 *
 * The macro "fn" must take multiple parameters: fn(node_id, prop, idx, ...).
 * "node_id" and "prop" are the same as what is passed to
 * DT_FOREACH_PROP_ELEM, and "idx" is the current index into the array.
 * The "idx" values are integer literals starting from 0. The remaining
 * arguments are passed-in by the caller.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @param ... variable number of arguments to pass to fn
 *
 * @see DT_FOREACH_PROP_ELEM
 */
#define DT_FOREACH_PROP_ELEM_VARGS(node_id, prop, fn, ...)		\
	DT_CAT4(node_id, _P_, prop, _FOREACH_PROP_ELEM_VARGS)(fn, __VA_ARGS__)

/**
 * @brief Invokes "fn" for each status "okay" node of a compatible.
 *
 * This macro expands to:
 *
 *     fn(node_id_1) fn(node_id_2) ... fn(node_id_n)
 *
 * where each "node_id_<i>" is a node identifier for some node with
 * compatible "compat" and status "okay". Whitespace is added between
 * expansions as shown above.
 *
 * Example devicetree fragment:
 *
 *     / {
 *             a {
 *                     compatible = "foo";
 *                     status = "okay";
 *             };
 *             b {
 *                     compatible = "foo";
 *                     status = "disabled";
 *             };
 *             c {
 *                     compatible = "foo";
 *             };
 *     };
 *
 * Example usage:
 *
 *     DT_FOREACH_STATUS_OKAY(foo, DT_NODE_PATH)
 *
 * This expands to one of the following:
 *
 *     "/a" "/c"
 *     "/c" "/a"
 *
 * "One of the following" is because no guarantees are made about the
 * order that node identifiers are passed to "fn" in the expansion.
 *
 * (The "/c" string literal is present because a missing status
 * property is always treated as if the status were set to "okay".)
 *
 * Note also that "fn" is responsible for adding commas, semicolons,
 * or other terminators as needed.
 *
 * @param compat lowercase-and-underscores devicetree compatible
 * @param fn Macro to call for each enabled node. Must accept a
 *           node_id as its only parameter.
 */
#define DT_FOREACH_STATUS_OKAY(compat, fn)				\
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(compat),			\
		    (UTIL_CAT(DT_FOREACH_OKAY_, compat)(fn)),	\
		    ())

/**
 * @brief Invokes "fn" for each status "okay" node of a compatible
 *        with multiple arguments.
 *
 * This is like DT_FOREACH_STATUS_OKAY() except you can also pass
 * additional arguments to "fn".
 *
 * Example devicetree fragment:
 *
 *     / {
 *             a {
 *                     compatible = "foo";
 *                     val = <3>;
 *             };
 *             b {
 *                     compatible = "foo";
 *                     val = <4>;
 *             };
 *     };
 *
 * Example usage:
 *
 *     #define MY_FN(node_id, operator) DT_PROP(node_id, val) operator
 *     x = DT_FOREACH_STATUS_OKAY_VARGS(foo, MY_FN, +) 0;
 *
 * This expands to one of the following:
 *
 *     x = 3 + 4 + 0;
 *     x = 4 + 3 + 0;
 *
 * i.e. it sets x to 7. As with DT_FOREACH_STATUS_OKAY(), there are no
 * guarantees about the order nodes appear in the expansion.
 *
 * @param compat lowercase-and-underscores devicetree compatible
 * @param fn Macro to call for each enabled node. Must accept a
 *           node_id as its only parameter.
 * @param ... Additional arguments to pass to "fn"
 */
#define DT_FOREACH_STATUS_OKAY_VARGS(compat, fn, ...)			\
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(compat),			\
		    (UTIL_CAT(DT_FOREACH_OKAY_VARGS_,			\
			      compat)(fn, __VA_ARGS__)),		\
		    ())

/**
 * @}
 */

/**
 * @defgroup devicetree-generic-exist Existence checks
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Does a node identifier refer to a node?
 *
 * Tests whether a node identifier refers to a node which exists, i.e.
 * is defined in the devicetree.
 *
 * It doesn't matter whether or not the node has a matching binding,
 * or what the node's status value is. This is purely a check of
 * whether the node exists at all.
 *
 * @param node_id a node identifier
 * @return 1 if the node identifier refers to a node,
 *         0 otherwise.
 */
#define DT_NODE_EXISTS(node_id) IS_ENABLED(DT_CAT(node_id, _EXISTS))

/**
 * @brief Does a node identifier refer to a node with a status?
 *
 * Example uses:
 *
 *     DT_NODE_HAS_STATUS(DT_PATH(soc, i2c_12340000), okay)
 *     DT_NODE_HAS_STATUS(DT_PATH(soc, i2c_12340000), disabled)
 *
 * Tests whether a node identifier refers to a node which:
 *
 * - exists in the devicetree, and
 * - has a status property matching the second argument
 *   (except that either a missing status or an "ok" status
 *   in the devicetree is treated as if it were "okay" instead)
 *
 * @param node_id a node identifier
 * @param status a status as one of the tokens okay or disabled, not a string
 * @return 1 if the node has the given status, 0 otherwise.
 */
#define DT_NODE_HAS_STATUS(node_id, status) \
	DT_NODE_HAS_STATUS_INTERNAL(node_id, status)

/**
 * @brief Does the devicetree have a status "okay" node with a compatible?
 *
 * Test for whether the devicetree has any nodes with status "okay"
 * and the given compatible. That is, this returns 1 if and only if
 * there is at least one "node_id" for which both of these
 * expressions return 1:
 *
 *     DT_NODE_HAS_STATUS(node_id, okay)
 *     DT_NODE_HAS_COMPAT(node_id, compat)
 *
 * As usual, both a missing status and an "ok" status are treated as
 * "okay".
 *
 * @param compat lowercase-and-underscores compatible, without quotes
 * @return 1 if both of the above conditions are met, 0 otherwise
 */
#define DT_HAS_COMPAT_STATUS_OKAY(compat) \
	IS_ENABLED(DT_CAT(DT_COMPAT_HAS_OKAY_, compat))

/**
 * @brief Get the number of instances of a given compatible with
 *        status "okay"
 * @param compat lowercase-and-underscores compatible, without quotes
 * @return Number of instances with status "okay"
 */
#define DT_NUM_INST_STATUS_OKAY(compat)			\
	UTIL_AND(DT_HAS_COMPAT_STATUS_OKAY(compat),		\
		 UTIL_CAT(DT_N_INST, DT_DASH(compat, NUM_OKAY)))

/**
 * @brief Does a devicetree node match a compatible?
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             compatible = "vnd,specific-device", "generic-device";
 *     }
 *
 * Example usages which evaluate to 1:
 *
 *     DT_NODE_HAS_COMPAT(DT_NODELABEL(n), vnd_specific_device)
 *     DT_NODE_HAS_COMPAT(DT_NODELABEL(n), generic_device)
 *
 * This macro only uses the value of the compatible property. Whether
 * or not a particular compatible has a matching binding has no effect
 * on its value, nor does the node's status.
 *
 * @param node_id node identifier
 * @param compat lowercase-and-underscores compatible, without quotes
 * @return 1 if the node's compatible property contains compat,
 *         0 otherwise.
 */
#define DT_NODE_HAS_COMPAT(node_id, compat) \
	IS_ENABLED(DT_CAT(node_id, _COMPAT_MATCHES_##compat))

/**
 * @brief Does a devicetree node have a compatible and status?
 *
 * This is equivalent to:
 *
 *     (DT_NODE_HAS_COMPAT(node_id, compat) &&
 *      DT_NODE_HAS_STATUS(node_id, status))
 *
 * @param node_id node identifier
 * @param compat lowercase-and-underscores compatible, without quotes
 * @param status okay or disabled as a token, not a string
 */
#define DT_NODE_HAS_COMPAT_STATUS(node_id, compat, status) \
	DT_NODE_HAS_COMPAT(node_id, compat) && DT_NODE_HAS_STATUS(node_id, status)

/**
 * @brief Does a devicetree node have a property?
 *
 * Tests whether a devicetree node has a property defined.
 *
 * This tests whether the property is defined at all, not whether a
 * boolean property is true or false. To get a boolean property's
 * truth value, use DT_PROP(node_id, prop) instead.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @return 1 if the node has the property, 0 otherwise.
 */
#define DT_NODE_HAS_PROP(node_id, prop) \
	IS_ENABLED(DT_CAT(node_id, _P_##prop##_EXISTS))


/**
 * @brief Does a phandle array have a named cell specifier at an index?
 *
 * If this returns 1, then the phandle-array property "pha" has a cell
 * named "cell" at index "idx", and therefore DT_PHA_BY_IDX(node_id,
 * pha, idx, cell) is valid. If it returns 0, it's an error to use
 * DT_PHA_BY_IDX() with the same arguments.
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param idx index to check within "pha"
 * @param cell lowercase-and-underscores cell name whose existence to check
 *             at index "idx"
 * @return 1 if the named cell exists in the specifier at index idx,
 *         0 otherwise.
 */
#define DT_PHA_HAS_CELL_AT_IDX(node_id, pha, idx, cell)             \
	IS_ENABLED(DT_PROP(node_id,                                 \
			   pha##_IDX_##idx##_VAL_##cell##_EXISTS))

/**
 * @brief Equivalent to DT_PHA_HAS_CELL_AT_IDX(node_id, pha, 0, cell)
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param cell lowercase-and-underscores cell name whose existence to check
 *             at index "idx"
 * @return 1 if the named cell exists in the specifier at index 0,
 *         0 otherwise.
 */
#define DT_PHA_HAS_CELL(node_id, pha, cell) \
	DT_PHA_HAS_CELL_AT_IDX(node_id, pha, 0, cell)

/**
 * @}
 */

/**
 * @defgroup devicetree-generic-bus Bus helpers
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Node's bus controller
 *
 * Get the node identifier of the node's bus controller. This can be
 * used with @ref DT_PROP() to get properties of the bus controller.
 *
 * It is an error to use this with nodes which do not have bus
 * controllers.
 *
 * Example devicetree fragment:
 *
 *     i2c@deadbeef {
 *             label = "I2C_CTLR";
 *             status = "okay";
 *             clock-frequency = < 100000 >;
 *
 *             i2c_device: accelerometer@12 {
 *                     ...
 *             };
 *     };
 *
 * Example usage:
 *
 *     DT_PROP(DT_BUS(DT_NODELABEL(i2c_device)), clock_frequency) // 100000
 *
 * @param node_id node identifier
 * @return a node identifier for the node's bus controller
 */
#define DT_BUS(node_id) DT_CAT(node_id, _BUS)

/**
 * @brief Node's bus controller's label property
 * @param node_id node identifier
 * @return the label property of the node's bus controller DT_BUS(node)
 */
#define DT_BUS_LABEL(node_id) DT_PROP(DT_BUS(node_id), label)

/**
 * @brief Is a node on a bus of a given type?
 *
 * Example devicetree overlay:
 *
 *     &i2c0 {
 *            temp: temperature-sensor@76 {
 *                     compatible = "vnd,some-sensor";
 *                     reg = <0x76>;
 *            };
 *     };
 *
 * Example usage, assuming "i2c0" is an I2C bus controller node, and
 * therefore "temp" is on an I2C bus:
 *
 *     DT_ON_BUS(DT_NODELABEL(temp), i2c) // 1
 *     DT_ON_BUS(DT_NODELABEL(temp), spi) // 0
 *
 * @param node_id node identifier
 * @param bus lowercase-and-underscores bus type as a C token (i.e.
 *            without quotes)
 * @return 1 if the node is on a bus of the given type,
 *         0 otherwise
 */
#define DT_ON_BUS(node_id, bus) IS_ENABLED(DT_CAT(node_id, _BUS_##bus))

/**
 * @}
 */

/**
 * @defgroup devicetree-inst Instance-based devicetree APIs
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Node identifier for an instance of a DT_DRV_COMPAT compatible
 * @param inst instance number
 * @return a node identifier for the node with DT_DRV_COMPAT compatible and
 *         instance number "inst"
 */
#define DT_DRV_INST(inst) DT_INST(inst, DT_DRV_COMPAT)

/**
 * @brief Call "fn" on all child nodes of DT_DRV_INST(inst).
 *
 * The macro "fn" should take one argument, which is the node
 * identifier for the child node.
 *
 * @param inst instance number
 * @param fn macro to invoke on each child node identifier
 *
 * @see DT_FOREACH_CHILD
 */
#define DT_INST_FOREACH_CHILD(inst, fn) \
	DT_FOREACH_CHILD(DT_DRV_INST(inst), fn)

/**
 * @brief Call "fn" on all child nodes of DT_DRV_INST(inst).
 *
 * The macro "fn" takes multiple arguments. The first should be the node
 * identifier for the child node. The remaining are passed-in by the caller.
 *
 * @param inst instance number
 * @param fn macro to invoke on each child node identifier
 * @param ... variable number of arguments to pass to fn
 *
 * @see DT_FOREACH_CHILD
 */
#define DT_INST_FOREACH_CHILD_VARGS(inst, fn, ...) \
	DT_FOREACH_CHILD_VARGS(DT_DRV_INST(inst), fn, __VA_ARGS__)

/**
 * @brief Get a DT_DRV_COMPAT value's index into its enumeration values
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return zero-based index of the property's value in its enum: list
 */
#define DT_INST_ENUM_IDX(inst, prop) \
	DT_ENUM_IDX(DT_DRV_INST(inst), prop)

/**
 * @brief Like DT_INST_ENUM_IDX(), but with a fallback to a default enum index
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param default_idx_value a fallback index value to expand to
 * @return zero-based index of the property's value in its enum if present,
 *         default_idx_value otherwise
 */
#define DT_INST_ENUM_IDX_OR(inst, prop, default_idx_value) \
	DT_ENUM_IDX_OR(DT_DRV_INST(inst), prop, default_idx_value)

/**
 * @brief Get a DT_DRV_COMPAT instance property
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return a representation of the property's value
 */
#define DT_INST_PROP(inst, prop) DT_PROP(DT_DRV_INST(inst), prop)

/**
 * @brief Get a DT_DRV_COMPAT property length
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return logical length of the property
 */
#define DT_INST_PROP_LEN(inst, prop) DT_PROP_LEN(DT_DRV_INST(inst), prop)

/**
 * @brief Is index "idx" valid for an array type property
 *        on a DT_DRV_COMPAT instance?
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param idx index to check
 * @return 1 if "idx" is a valid index into the given property,
 *         0 otherwise.
 */
#define DT_INST_PROP_HAS_IDX(inst, prop, idx) \
	DT_PROP_HAS_IDX(DT_DRV_INST(inst), prop, idx)

/**
 * @brief Is name "name" available in a foo-names property?
 * @param inst instance number
 * @param prop a lowercase-and-underscores "prop-names" type property
 * @param name a lowercase-and-underscores name to check
 * @return An expression which evaluates to 1 if "name" is an available
 *         name into the given property, and 0 otherwise.
 */
#define DT_INST_PROP_HAS_NAME(inst, prop, name) \
	DT_PROP_HAS_NAME(DT_DRV_INST(inst), prop, name)

/**
 * @brief Get a DT_DRV_COMPAT element value in an array property
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return a representation of the idx-th element of the property
 */
#define DT_INST_PROP_BY_IDX(inst, prop, idx) \
	DT_PROP_BY_IDX(DT_DRV_INST(inst), prop, idx)

/**
 * @brief Like DT_INST_PROP(), but with a fallback to default_value
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return DT_INST_PROP(inst, prop) or default_value
 */
#define DT_INST_PROP_OR(inst, prop, default_value) \
	DT_PROP_OR(DT_DRV_INST(inst), prop, default_value)

/**
 * @brief Get a DT_DRV_COMPAT instance's "label" property
 * @param inst instance number
 * @return instance's label property value
 */
#define DT_INST_LABEL(inst) DT_INST_PROP(inst, label)

/**
 * @brief Get a DT_DRV_COMPAT instance's string property's value as a
 *        token.
 *
 * @param inst instance number
 * @param prop lowercase-and-underscores property string name
 * @return the value of @p prop as a token, i.e. without any quotes
 *         and with special characters converted to underscores
 */
#define DT_INST_STRING_TOKEN(inst, prop) \
	DT_STRING_TOKEN(DT_DRV_INST(inst), prop)

/**
 * @brief Like DT_INST_STRING_TOKEN(), but uppercased.
 * @param inst instance number
 * @param prop lowercase-and-underscores property string name
 * @return the value of @p prop as an uppercased token, i.e. without
 *         any quotes and with special characters converted to underscores
 */
#define DT_INST_STRING_UPPER_TOKEN(inst, prop) \
	DT_STRING_UPPER_TOKEN(DT_DRV_INST(inst), prop)

/**
 * @brief Get a DT_DRV_COMPAT instance's property value from a phandle's node
 * @param inst instance number
 * @param ph lowercase-and-underscores property of "inst"
 *           with type "phandle"
 * @param prop lowercase-and-underscores property of the phandle's node
 * @return the value of "prop" as described in the DT_PROP() documentation
 */
#define DT_INST_PROP_BY_PHANDLE(inst, ph, prop) \
	DT_INST_PROP_BY_PHANDLE_IDX(inst, ph, 0, prop)

/**
 * @brief Get a DT_DRV_COMPAT instance's property value from a phandle in a
 * property.
 * @param inst instance number
 * @param phs lowercase-and-underscores property with type "phandle",
 *            "phandles", or "phandle-array"
 * @param idx logical index into "phs", which must be zero if "phs"
 *            has type "phandle"
 * @param prop lowercase-and-underscores property of the phandle's node
 * @return the value of "prop" as described in the DT_PROP() documentation
 */
#define DT_INST_PROP_BY_PHANDLE_IDX(inst, phs, idx, prop) \
	DT_PROP_BY_PHANDLE_IDX(DT_DRV_INST(inst), phs, idx, prop)

/**
 * @brief Get a DT_DRV_COMPAT instance's phandle-array specifier value at an index
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param idx logical index into the property "pha"
 * @param cell binding's cell name within the specifier at index "idx"
 * @return the value of the cell inside the specifier at index "idx"
 */
#define DT_INST_PHA_BY_IDX(inst, pha, idx, cell) \
	DT_PHA_BY_IDX(DT_DRV_INST(inst), pha, idx, cell)

/**
 * @brief Like DT_INST_PHA_BY_IDX(), but with a fallback to default_value
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param idx logical index into the property "pha"
 * @param cell binding's cell name within the specifier at index "idx"
 * @param default_value a fallback value to expand to
 * @return DT_INST_PHA_BY_IDX(inst, pha, idx, cell) or default_value
 */
#define DT_INST_PHA_BY_IDX_OR(inst, pha, idx, cell, default_value) \
	DT_PHA_BY_IDX_OR(DT_DRV_INST(inst), pha, idx, cell, default_value)

/**
 * @brief Get a DT_DRV_COMPAT instance's phandle-array specifier value
 * Equivalent to DT_INST_PHA_BY_IDX(inst, pha, 0, cell)
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param cell binding's cell name for the specifier at "pha" index 0
 * @return the cell value
 */
#define DT_INST_PHA(inst, pha, cell) DT_INST_PHA_BY_IDX(inst, pha, 0, cell)

/**
 * @brief Like DT_INST_PHA(), but with a fallback to default_value
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param cell binding's cell name for the specifier at "pha" index 0
 * @param default_value a fallback value to expand to
 * @return DT_INST_PHA(inst, pha, cell) or default_value
 */
#define DT_INST_PHA_OR(inst, pha, cell, default_value) \
	DT_INST_PHA_BY_IDX_OR(inst, pha, 0, cell, default_value)

/**
 * @brief Get a DT_DRV_COMPAT instance's value within a phandle-array
 * specifier by name
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param name lowercase-and-underscores name of a specifier in "pha"
 * @param cell binding's cell name for the named specifier
 * @return the cell value
 */
#define DT_INST_PHA_BY_NAME(inst, pha, name, cell) \
	DT_PHA_BY_NAME(DT_DRV_INST(inst), pha, name, cell)

/**
 * @brief Like DT_INST_PHA_BY_NAME(), but with a fallback to default_value
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param name lowercase-and-underscores name of a specifier in "pha"
 * @param cell binding's cell name for the named specifier
 * @param default_value a fallback value to expand to
 * @return DT_INST_PHA_BY_NAME(inst, pha, name, cell) or default_value
 */
#define DT_INST_PHA_BY_NAME_OR(inst, pha, name, cell, default_value) \
	DT_PHA_BY_NAME_OR(DT_DRV_INST(inst), pha, name, cell, default_value)

/**
 * @brief Get a DT_DRV_COMPAT instance's phandle node identifier from a
 * phandle array by name
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param name lowercase-and-underscores name of an element in "pha"
 * @return node identifier for the phandle at the element named "name"
 */
#define DT_INST_PHANDLE_BY_NAME(inst, pha, name) \
	DT_PHANDLE_BY_NAME(DT_DRV_INST(inst), pha, name) \

/**
 * @brief Get a DT_DRV_COMPAT instance's node identifier for a phandle in
 * a property.
 * @param inst instance number
 * @param prop lowercase-and-underscores property name in "inst"
 *             with type "phandle", "phandles" or "phandle-array"
 * @param idx index into "prop"
 * @return a node identifier for the phandle at index "idx" in "prop"
 */
#define DT_INST_PHANDLE_BY_IDX(inst, prop, idx) \
	DT_PHANDLE_BY_IDX(DT_DRV_INST(inst), prop, idx)

/**
 * @brief Get a DT_DRV_COMPAT instance's node identifier for a phandle
 * property's value
 * @param inst instance number
 * @param prop lowercase-and-underscores property of "inst"
 *             with type "phandle"
 * @return a node identifier for the node pointed to by "ph"
 */
#define DT_INST_PHANDLE(inst, prop) DT_INST_PHANDLE_BY_IDX(inst, prop, 0)

/**
 * @brief is "idx" a valid register block index on a DT_DRV_COMPAT instance?
 * @param inst instance number
 * @param idx index to check
 * @return 1 if "idx" is a valid register block index,
 *         0 otherwise.
 */
#define DT_INST_REG_HAS_IDX(inst, idx) DT_REG_HAS_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get a DT_DRV_COMPAT instance's idx-th register block's address
 * @param inst instance number
 * @param idx index of the register whose address to return
 * @return address of the instance's idx-th register block
 */
#define DT_INST_REG_ADDR_BY_IDX(inst, idx) DT_REG_ADDR_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get a DT_DRV_COMPAT instance's idx-th register block's size
 * @param inst instance number
 * @param idx index of the register whose size to return
 * @return size of the instance's idx-th register block
 */
#define DT_INST_REG_SIZE_BY_IDX(inst, idx) \
	DT_REG_SIZE_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get a DT_DRV_COMPAT's register block address by name
 * @param inst instance number
 * @param name lowercase-and-underscores register specifier name
 * @return address of the register block with the given name
 */
#define DT_INST_REG_ADDR_BY_NAME(inst, name) \
	DT_REG_ADDR_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get a DT_DRV_COMPAT's register block size by name
 * @param inst instance number
 * @param name lowercase-and-underscores register specifier name
 * @return size of the register block with the given name
 */
#define DT_INST_REG_SIZE_BY_NAME(inst, name) \
	DT_REG_SIZE_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get a DT_DRV_COMPAT's (only) register block address
 * @param inst instance number
 * @return instance's register block address
 */
#define DT_INST_REG_ADDR(inst) DT_INST_REG_ADDR_BY_IDX(inst, 0)

/**
 * @brief Get a DT_DRV_COMPAT's (only) register block size
 * @param inst instance number
 * @return instance's register block size
 */
#define DT_INST_REG_SIZE(inst) DT_INST_REG_SIZE_BY_IDX(inst, 0)

/**
 * @brief Get a DT_DRV_COMPAT interrupt specifier value at an index
 * @param inst instance number
 * @param idx logical index into the interrupt specifier array
 * @param cell cell name specifier
 * @return the named value at the specifier given by the index
 */
#define DT_INST_IRQ_BY_IDX(inst, idx, cell) \
	DT_IRQ_BY_IDX(DT_DRV_INST(inst), idx, cell)

/**
 * @brief Get a DT_DRV_COMPAT interrupt specifier value by name
 * @param inst instance number
 * @param name lowercase-and-underscores interrupt specifier name
 * @param cell cell name specifier
 * @return the named value at the specifier given by the index
 */
#define DT_INST_IRQ_BY_NAME(inst, name, cell) \
	DT_IRQ_BY_NAME(DT_DRV_INST(inst), name, cell)

/**
 * @brief Get a DT_DRV_COMPAT interrupt specifier's value
 * @param inst instance number
 * @param cell cell name specifier
 * @return the named value at that index
 */
#define DT_INST_IRQ(inst, cell) DT_INST_IRQ_BY_IDX(inst, 0, cell)

/**
 * @brief Get a DT_DRV_COMPAT's (only) irq number
 * @param inst instance number
 * @return the interrupt number for the node's only interrupt
 */
#define DT_INST_IRQN(inst) DT_INST_IRQ(inst, irq)

/**
 * @brief Get a DT_DRV_COMPAT's bus node identifier
 * @param inst instance number
 * @return node identifier for the instance's bus node
 */
#define DT_INST_BUS(inst) DT_BUS(DT_DRV_INST(inst))

/**
 * @brief Get a DT_DRV_COMPAT's bus node's label property
 * @param inst instance number
 * @return the label property of the instance's bus controller
 */
#define DT_INST_BUS_LABEL(inst) DT_BUS_LABEL(DT_DRV_INST(inst))

/**
 * @brief Test if a DT_DRV_COMPAT's bus type is a given type
 * @param inst instance number
 * @param bus a binding's bus type as a C token, lowercased and without quotes
 * @return 1 if the given instance is on a bus of the given type,
 *         0 otherwise
 */
#define DT_INST_ON_BUS(inst, bus) DT_ON_BUS(DT_DRV_INST(inst), bus)

/**
 * @brief Like DT_INST_STRING_TOKEN(), but with a fallback to default_value
 * @param inst instance number
 * @param name lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return if @p prop exists, its value as a token, i.e. without any quotes and
 *         with special characters converted to underscores. Othewise
 *         @p default_value
 */
#define DT_INST_STRING_TOKEN_OR(inst, name, default_value) \
	DT_STRING_TOKEN_OR(DT_DRV_INST(inst), name, default_value)

/**
 * @brief Like DT_INST_STRING_UPPER_TOKEN(), but with a fallback to default_value
 * @param inst instance number
 * @param name lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return the property's value as an uppercased token, or @p default_value
 */
#define DT_INST_STRING_UPPER_TOKEN_OR(inst, name, default_value) \
	DT_STRING_UPPER_TOKEN_OR(DT_DRV_INST(inst), name, default_value)

/**
 * @brief Test if any DT_DRV_COMPAT node is on a bus of a given type
 *        and has status okay
 *
 * This is a special-purpose macro which can be useful when writing
 * drivers for devices which can appear on multiple buses. One example
 * is a sensor device which may be wired on an I2C or SPI bus.
 *
 * Example devicetree overlay:
 *
 *     &i2c0 {
 *            temp: temperature-sensor@76 {
 *                     compatible = "vnd,some-sensor";
 *                     reg = <0x76>;
 *            };
 *     };
 *
 * Example usage, assuming "i2c0" is an I2C bus controller node, and
 * therefore "temp" is on an I2C bus:
 *
 *     #define DT_DRV_COMPAT vnd_some_sensor
 *
 *     DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) // 1
 *
 * @param bus a binding's bus type as a C token, lowercased and without quotes
 * @return 1 if any enabled node with that compatible is on that bus type,
 *         0 otherwise
 */
#define DT_ANY_INST_ON_BUS_STATUS_OKAY(bus) \
	DT_COMPAT_ON_BUS_INTERNAL(DT_DRV_COMPAT, bus)

/**
 * @brief Call "fn" on all nodes with compatible DT_DRV_COMPAT
 *        and status "okay"
 *
 * This macro calls "fn(inst)" on each "inst" number that refers to a
 * node with status "okay". Whitespace is added between invocations.
 *
 * Example devicetree fragment:
 *
 *     a {
 *             compatible = "vnd,device";
 *             status = "okay";
 *             label = "DEV_A";
 *     };
 *
 *     b {
 *             compatible = "vnd,device";
 *             status = "okay";
 *             label = "DEV_B";
 *     };
 *
 *     c {
 *             compatible = "vnd,device";
 *             status = "disabled";
 *             label = "DEV_C";
 *     };
 *
 * Example usage:
 *
 *     #define DT_DRV_COMPAT vnd_device
 *     #define MY_FN(inst) DT_INST_LABEL(inst),
 *
 *     DT_INST_FOREACH_STATUS_OKAY(MY_FN)
 *
 * This expands to:
 *
 *     MY_FN(0) MY_FN(1)
 *
 * and from there, to either this:
 *
 *     "DEV_A", "DEV_B",
 *
 * or this:
 *
 *     "DEV_B", "DEV_A",
 *
 * No guarantees are made about the order that a and b appear in the
 * expansion.
 *
 * Note that "fn" is responsible for adding commas, semicolons, or
 * other separators or terminators.
 *
 * Device drivers should use this macro whenever possible to
 * instantiate a struct device for each enabled node in the devicetree
 * of the driver's compatible DT_DRV_COMPAT.
 *
 * @param fn Macro to call for each enabled node. Must accept an
 *           instance number as its only parameter.
 */
#define DT_INST_FOREACH_STATUS_OKAY(fn) \
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT),	\
		    (UTIL_CAT(DT_FOREACH_OKAY_INST_,		\
			      DT_DRV_COMPAT)(fn)),		\
		    ())

/**
 * @brief Call "fn" on all nodes with compatible DT_DRV_COMPAT
 *        and status "okay" with multiple arguments
 *
 *
 * @param fn Macro to call for each enabled node. Must accept an
 *           instance number as its only parameter.
 * @param ... variable number of arguments to pass to fn
 *
 * @see DT_INST_FOREACH_STATUS_OKAY
 */
#define DT_INST_FOREACH_STATUS_OKAY_VARGS(fn, ...) \
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT),	\
		    (UTIL_CAT(DT_FOREACH_OKAY_INST_VARGS_,	\
			      DT_DRV_COMPAT)(fn, __VA_ARGS__)),	\
		    ())

/**
 * @brief Invokes "fn" for each element of property "prop" for
 *        a DT_DRV_COMPAT instance.
 *
 * Equivalent to DT_FOREACH_PROP_ELEM(DT_DRV_INST(inst), prop, fn).
 *
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 */
#define DT_INST_FOREACH_PROP_ELEM(inst, prop, fn) \
	DT_FOREACH_PROP_ELEM(DT_DRV_INST(inst), prop, fn)

/**
 * @brief Invokes "fn" for each element of property "prop" for
 *        a DT_DRV_COMPAT instance with multiple arguments.
 *
 * Equivalent to
 *      DT_FOREACH_PROP_ELEM_VARGS(DT_DRV_INST(inst), prop, fn, __VA_ARGS__)
 *
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @param ... variable number of arguments to pass to fn
 *
 * @see DT_INST_FOREACH_PROP_ELEM
 */
#define DT_INST_FOREACH_PROP_ELEM_VARGS(inst, prop, fn, ...) \
	DT_FOREACH_PROP_ELEM_VARGS(DT_DRV_INST(inst), prop, fn, __VA_ARGS__)

/**
 * @brief Does a DT_DRV_COMPAT instance have a property?
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return 1 if the instance has the property, 0 otherwise.
 */
#define DT_INST_NODE_HAS_PROP(inst, prop) \
	DT_NODE_HAS_PROP(DT_DRV_INST(inst), prop)

/**
 * @brief Does a phandle array have a named cell specifier at an index
 *        for a DT_DRV_COMPAT instance?
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param idx index to check
 * @param cell named cell value whose existence to check
 * @return 1 if the named cell exists in the specifier at index idx,
 *         0 otherwise.
 */
#define DT_INST_PHA_HAS_CELL_AT_IDX(inst, pha, idx, cell) \
	DT_PHA_HAS_CELL_AT_IDX(DT_DRV_INST(inst), pha, idx, cell)

/**
 * @brief Does a phandle array have a named cell specifier at index 0
 *        for a DT_DRV_COMPAT instance?
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param cell named cell value whose existence to check
 * @return 1 if the named cell exists in the specifier at index 0,
 *         0 otherwise.
 */
#define DT_INST_PHA_HAS_CELL(inst, pha, cell) \
	DT_INST_PHA_HAS_CELL_AT_IDX(inst, pha, 0, cell)

/**
 * @brief is index valid for interrupt property on a DT_DRV_COMPAT instance?
 * @param inst instance number
 * @param idx logical index into the interrupt specifier array
 * @return 1 if the idx is valid for the interrupt property
 *         0 otherwise.
 */
#define DT_INST_IRQ_HAS_IDX(inst, idx) DT_IRQ_HAS_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Does a DT_DRV_COMPAT instance have an interrupt named cell specifier?
 * @param inst instance number
 * @param idx index to check
 * @param cell named cell value whose existence to check
 * @return 1 if the named cell exists in the interrupt specifier at index idx
 *         0 otherwise.
 */
#define DT_INST_IRQ_HAS_CELL_AT_IDX(inst, idx, cell) \
	DT_IRQ_HAS_CELL_AT_IDX(DT_DRV_INST(inst), idx, cell)

/**
 * @brief Does a DT_DRV_COMPAT instance have an interrupt value?
 * @param inst instance number
 * @param cell named cell value whose existence to check
 * @return 1 if the named cell exists in the interrupt specifier at index 0
 *         0 otherwise.
 */
#define DT_INST_IRQ_HAS_CELL(inst, cell) \
	DT_INST_IRQ_HAS_CELL_AT_IDX(inst, 0, cell)

/**
 * @brief Does a DT_DRV_COMPAT instance have an interrupt value?
 * @param inst instance number
 * @param name lowercase-and-underscores interrupt specifier name
 * @return 1 if "name" is a valid named specifier
 */
#define DT_INST_IRQ_HAS_NAME(inst, name) \
	DT_IRQ_HAS_NAME(DT_DRV_INST(inst), name)

/**
 * @}
 */

/** @internal pay no attention to the man behind the curtain! */
#define DT_PATH_INTERNAL(...) \
	UTIL_CAT(DT_ROOT, MACRO_MAP_CAT(DT_S_PREFIX, __VA_ARGS__))
/** @internal helper for DT_PATH(): prepends _S_ to a node name */
#define DT_S_PREFIX(name) _S_##name

/**
 * @internal concatenation helper, 2 arguments
 *
 * This and the following macros are used to paste things together
 * with "##" *after* forcing expansion on each argument.
 *
 * We could try to use something like UTIL_CAT(), but the compiler
 * error messages from the util macros can be extremely long when they
 * are misused. This unfortunately happens often with devicetree.h,
 * since its macro-based API is fiddly and can be hard to get right.
 *
 * Keeping things brutally simple here hopefully makes some errors
 * easier to read.
 */
#define DT_CAT(a1, a2) a1 ## a2
/** @internal concatenation helper, 3 arguments */
#define DT_CAT3(a1, a2, a3) a1 ## a2 ## a3
/** @internal concatenation helper, 4 arguments */
#define DT_CAT4(a1, a2, a3, a4) a1 ## a2 ## a3 ## a4
/** @internal concatenation helper, 5 arguments */
#define DT_CAT5(a1, a2, a3, a4, a5) a1 ## a2 ## a3 ## a4 ## a5
/** @internal concatenation helper, 6 arguments */
#define DT_CAT6(a1, a2, a3, a4, a5, a6) a1 ## a2 ## a3 ## a4 ## a5 ## a6
/*
 * If you need to define a bigger DT_CATN(), do so here. Don't leave
 * any "holes" of undefined macros, please.
 */

/** @internal helper for node identifier macros to expand args */
#define DT_DASH(...) MACRO_MAP_CAT(DT_DASH_PREFIX, __VA_ARGS__)
/** @internal helper for DT_DASH(): prepends _ to a name */
#define DT_DASH_PREFIX(name) _##name
/** @internal helper for DT_NODE_HAS_STATUS */
#define DT_NODE_HAS_STATUS_INTERNAL(node_id, status) \
	IS_ENABLED(DT_CAT(node_id, _STATUS_ ## status))
/** @internal helper for test cases and DT_ANY_INST_ON_BUS_STATUS_OKAY() */
#define DT_COMPAT_ON_BUS_INTERNAL(compat, bus) \
	IS_ENABLED(UTIL_CAT(DT_CAT(DT_COMPAT_, compat), _BUS_##bus))

/* have these last so they have access to all previously defined macros */
#include <zephyr/devicetree/io-channels.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/devicetree/gpio.h>
#include <zephyr/devicetree/spi.h>
#include <zephyr/devicetree/dma.h>
#include <zephyr/devicetree/pwms.h>
#include <zephyr/devicetree/fixed-partitions.h>
#include <zephyr/devicetree/zephyr.h>
#include <zephyr/devicetree/ordinals.h>
#include <zephyr/devicetree/pinctrl.h>
#include <zephyr/devicetree/can.h>
#include <zephyr/devicetree/reset.h>
#include <zephyr/devicetree/mbox.h>

#endif /* DEVICETREE_H */
