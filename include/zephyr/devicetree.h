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

#include <zephyr/devicetree_generated.h>
#include <zephyr/irq_multilevel.h>

#if !defined(_LINKER) && !defined(_ASMLANGUAGE)
#include <stdint.h>
#endif

#include <zephyr/sys/util.h>

/**
 * @brief devicetree.h API
 * @defgroup devicetree Devicetree
 * @since 2.2
 * @version 1.2.0
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
 * _ENUM_VAL_<val>_EXISTS property's value as a token exists
 * _EXISTS: property is defined
 * _FOREACH_PROP_ELEM: helper for "iterating" over values in the property
 * _FOREACH_PROP_ELEM_VARGS: foreach functions with variable number of arguments
 * _IDX_<i>: logical index into property
 * _IDX_<i>_EXISTS: logical index into property is defined
 * _IDX_<i>_PH: phandle array's phandle by index (or phandle, phandles)
 * _IDX_<i>_STRING_TOKEN: string array element value as a token
 * _IDX_<i>_STRING_UPPER_TOKEN: string array element value as a uppercased token
 * _IDX <i>_STRING_UNQUOTED: string array element value as a sequence of tokens, with no quotes
 * _IDX_<i>_VAL_<val>: phandle array's specifier value by index
 * _IDX_<i>_VAL_<val>_EXISTS: cell value exists, by index
 * _LEN: property logical length
 * _NAME_<name>_PH: phandle array's phandle by name
 * _NAME_<name>_VAL_<val>: phandle array's property specifier by name
 * _NAME_<name>_VAL_<val>_EXISTS: cell value exists, by name
 * _STRING_TOKEN: string property's value as a token
 * _STRING_UPPER_TOKEN: like _STRING_TOKEN, but uppercased
 * _STRING_UNQUOTED: string property's value as a sequence of tokens, with no quotes
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
 * devicetree data may or may not be available. It is a preprocessor identifier
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
 * @note This macro returns a node identifier from path components. To get
 *       a path string from a node identifier, use DT_NODE_PATH() instead.
 *
 * The arguments to this macro are the names of non-root nodes in the
 * tree required to reach the desired node, starting from the root.
 * Non-alphanumeric characters in each name must be converted to
 * underscores to form valid C tokens, and letters must be lowercased.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     / {
 *             soc {
 *                     serial1: serial@40001000 {
 *                             status = "okay";
 *                             current-speed = <115200>;
 *                             ...
 *                     };
 *             };
 *     };
 * @endcode
 *
 * You can use `DT_PATH(soc, serial_40001000)` to get a node identifier
 * for the `serial@40001000` node. Node labels like `serial1` cannot be
 * used as DT_PATH() arguments; use DT_NODELABEL() for those instead.
 *
 * Example usage with DT_PROP() to get the `current-speed` property:
 *
 * @code{.c}
 *     DT_PROP(DT_PATH(soc, serial_40001000), current_speed) // 115200
 * @endcode
 *
 * (The `current-speed` property is also in `lowercase-and-underscores`
 * form when used with this API.)
 *
 * When determining arguments to DT_PATH():
 *
 * - the first argument corresponds to a child node of the root (`soc` above)
 * - a second argument corresponds to a child of the first argument
 *   (`serial_40001000` above, from the node name `serial@40001000`
 *   after lowercasing and changing `@` to `_`)
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
 * @code{.dts}
 *     serial1: serial@40001000 {
 *             label = "UART_0";
 *             status = "okay";
 *             current-speed = <115200>;
 *             ...
 *     };
 * @endcode
 *
 * The only node label in this example is `serial1`.
 *
 * The string `UART_0` is *not* a node label; it's the value of a
 * property named label.
 *
 * You can use `DT_NODELABEL(serial1)` to get a node identifier for the
 * `serial@40001000` node. Example usage with DT_PROP() to get the
 * current-speed property:
 *
 * @code{.c}
 *     DT_PROP(DT_NODELABEL(serial1), current_speed) // 115200
 * @endcode
 *
 * Another example devicetree fragment:
 *
 * @code{.dts}
 *     cpu@0 {
 *            L2_0: l2-cache {
 *                    cache-level = <2>;
 *                    ...
 *            };
 *     };
 * @endcode
 *
 * Example usage to get the cache-level property:
 *
 * @code{.c}
 *     DT_PROP(DT_NODELABEL(l2_0), cache_level) // 2
 * @endcode
 *
 * Notice how `L2_0` in the devicetree is lowercased to `l2_0` in the
 * DT_NODELABEL() argument.
 *
 * @param label lowercase-and-underscores node label name
 * @return node identifier for the node with that label
 */
#define DT_NODELABEL(label) DT_CAT(DT_N_NODELABEL_, label)

/**
 * @brief Get a node identifier from /aliases
 *
 * This macro's argument is a property of the `/aliases` node. It
 * returns a node identifier for the node which is aliased. Convert
 * non-alphanumeric characters in the alias property to underscores to
 * form valid C tokens, and lowercase all letters.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
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
 * @endcode
 *
 * You can use DT_ALIAS(my_serial) to get a node identifier for the
 * `serial@40001000` node. Notice how `my-serial` in the devicetree
 * becomes `my_serial` in the DT_ALIAS() argument. Example usage with
 * DT_PROP() to get the current-speed property:
 *
 * @code{.c}
 *     DT_PROP(DT_ALIAS(my_serial), current_speed) // 115200
 * @endcode
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
 * passing DT_INST() an instance number, @p inst, along with the
 * lowercase-and-underscores version of the compatible, @p compat.
 *
 * Instance numbers have the following properties:
 *
 * - for each compatible, instance numbers start at 0 and are contiguous
 * - exactly one instance number is assigned for each node with a compatible,
 *   **including disabled nodes**
 * - enabled nodes (status property is `okay` or missing) are assigned the
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
 * @code{.dts}
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
 * @endcode
 *
 * Assuming no other nodes in the devicetree have compatible
 * `"vnd,soc-serial"`, that compatible has nodes with instance numbers
 * 0, 1, and 2.
 *
 * The nodes `serial@40002000` and `serial@40003000` are both enabled, so
 * their instance numbers are 0 and 1, but no guarantees are made
 * regarding which node has which instance number.
 *
 * Since `serial@40001000` is the only disabled node, it has instance
 * number 2, since disabled nodes are assigned the largest instance
 * numbers. Therefore:
 *
 * @code{.c}
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
 *     // disabled nodes are "at the end" of the instance
 *     // number "list".
 *     DT_PROP(DT_INST(2, vnd_soc_serial), current_speed)
 * @endcode
 *
 * Notice how `"vnd,soc-serial"` in the devicetree becomes `vnd_soc_serial`
 * (without quotes) in the DT_INST() arguments. (As usual, `current-speed`
 * in the devicetree becomes `current_speed` as well.)
 *
 * Nodes whose `compatible` property has multiple values are assigned
 * independent instance numbers for each compatible.
 *
 * @param inst instance number for compatible @p compat
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
 * @code{.dts}
 *     parent: parent-node {
 *             child: child-node {
 *                     ...
 *             };
 *     };
 * @endcode
 *
 * The following are equivalent ways to get the same node identifier:
 *
 * @code{.c}
 *     DT_NODELABEL(parent)
 *     DT_PARENT(DT_NODELABEL(child))
 * @endcode
 *
 * @param node_id node identifier
 * @return a node identifier for the node's parent
 */
#define DT_PARENT(node_id) DT_CAT(node_id, _PARENT)

/**
 * @brief Get a node identifier for a grandparent node
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     gparent: grandparent-node {
 *             parent: parent-node {
 *                     child: child-node { ... }
 *             };
 *     };
 * @endcode
 *
 * The following are equivalent ways to get the same node identifier:
 *
 * @code{.c}
 *     DT_GPARENT(DT_NODELABEL(child))
 *     DT_PARENT(DT_PARENT(DT_NODELABEL(child))
 * @endcode
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
 * @code{.dts}
 *     / {
 *             soc-label: soc {
 *                     serial1: serial@40001000 {
 *                             status = "okay";
 *                             current-speed = <115200>;
 *                             ...
 *                     };
 *             };
 *     };
 * @endcode
 *
 * Example usage with DT_PROP() to get the status of the
 * `serial@40001000` node:
 *
 * @code{.c}
 *     #define SOC_NODE DT_NODELABEL(soc_label)
 *     DT_PROP(DT_CHILD(SOC_NODE, serial_40001000), status) // "okay"
 * @endcode
 *
 * Node labels like `serial1` cannot be used as the @p child argument
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
 * @brief Get a node identifier for a status `okay` node with a compatible
 *
 * Use this if you want to get an arbitrary enabled node with a given
 * compatible, and you do not care which one you get. If any enabled
 * nodes with the given compatible exist, a node identifier for one
 * of them is returned. Otherwise, @ref DT_INVALID_NODE is returned.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
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
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_COMPAT_GET_ANY_STATUS_OKAY(vnd_device)
 * @endcode
 *
 * This expands to a node identifier for either `node-a` or `node-b`.
 * It will not expand to a node identifier for `node-c`, because that
 * node does not have status `okay`.
 *
 * @param compat lowercase-and-underscores compatible, without quotes
 * @return node identifier for a node with that compatible, or
 *         @ref DT_INVALID_NODE
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
 * @code{.dts}
 *     / {
 *             soc {
 *                     node: my-node@12345678 { ... };
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *    DT_NODE_PATH(DT_NODELABEL(node)) // "/soc/my-node@12345678"
 *    DT_NODE_PATH(DT_PATH(soc))       // "/soc"
 *    DT_NODE_PATH(DT_ROOT)            // "/"
 * @endcode
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
 * @code{.dts}
 *     / {
 *             soc {
 *                     node: my-node@12345678 { ... };
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *    DT_NODE_FULL_NAME(DT_NODELABEL(node)) // "my-node@12345678"
 * @endcode
 *
 * @param node_id node identifier
 * @return the node's name with unit-address as a string in the devicetree
 */
#define DT_NODE_FULL_NAME(node_id) DT_CAT(node_id, _FULL_NAME)

/**
 * @brief Get the node's full name, including the unit-address, as an unquoted
 *        sequence of tokens
 *
 * This macro returns removed "the quotes" from the node's full name.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     / {
 *             soc {
 *                     node: my-node@12345678 { ... };
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *    DT_NODE_FULL_NAME_UNQUOTED(DT_NODELABEL(node)) // my-node@12345678
 * @endcode
 *
 * @param node_id node identifier
 * @return the node's full name with unit-address as a sequence of tokens,
 *         with no quotes
 */
#define DT_NODE_FULL_NAME_UNQUOTED(node_id) DT_CAT(node_id, _FULL_NAME_UNQUOTED)

/**
 * @brief Get the node's full name, including the unit-address, as a token.
 *
 * This macro returns removed "the quotes" from the node's full name and
 * converting any non-alphanumeric characters to underscores.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     / {
 *             soc {
 *                     node: my-node@12345678 { ... };
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *    DT_NODE_FULL_NAME_TOKEN(DT_NODELABEL(node)) // my_node_12345678
 * @endcode
 *
 * @param node_id node identifier
 * @return the node's full name with unit-address as a token, i.e. without any quotes
 *         and with special characters converted to underscores
 */
#define DT_NODE_FULL_NAME_TOKEN(node_id) DT_CAT(node_id, _FULL_NAME_TOKEN)

/**
 * @brief Like DT_NODE_FULL_NAME_TOKEN(), but uppercased.
 *
 * This macro returns removed "the quotes" from the node's full name,
 * converting any non-alphanumeric characters to underscores, and
 * capitalizing the result.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     / {
 *             soc {
 *                     node: my-node@12345678 { ... };
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *    DT_NODE_FULL_NAME_UPPER_TOKEN(DT_NODELABEL(node)) // MY_NODE_12345678
 * @endcode
 *
 * @param node_id node identifier
 * @return the node's full name with unit-address as an uppercased token,
 *         i.e. without any quotes and with special characters converted
 *         to underscores
 */
#define DT_NODE_FULL_NAME_UPPER_TOKEN(node_id) DT_CAT(node_id, _FULL_NAME_UPPER_TOKEN)

/**
 * @brief Get a devicetree node's index into its parent's list of children
 *
 * Indexes are zero-based.
 *
 * It is an error to use this macro with the root node.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     parent {
 *             c1: child-1 {};
 *             c2: child-2 {};
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_NODE_CHILD_IDX(DT_NODELABEL(c1)) // 0
 *     DT_NODE_CHILD_IDX(DT_NODELABEL(c2)) // 1
 * @endcode
 *
 * @param node_id node identifier
 * @return the node's index in its parent node's list of children
 */
#define DT_NODE_CHILD_IDX(node_id) DT_CAT(node_id, _CHILD_IDX)

/**
 * @brief Get the number of child nodes of a given node
 *
 * @param node_id a node identifier
 * @return Number of child nodes
 */
#define DT_CHILD_NUM(node_id) DT_CAT(node_id, _CHILD_NUM)


/**
 * @brief Get the number of child nodes of a given node
 *        which child nodes' status are okay
 *
 * @param node_id a node identifier
 * @return Number of child nodes which status are okay
 */
#define DT_CHILD_NUM_STATUS_OKAY(node_id) \
	DT_CAT(node_id, _CHILD_NUM_STATUS_OKAY)

/**
 * @brief Do @p node_id1 and @p node_id2 refer to the same node?
 *
 * Both @p node_id1 and @p node_id2 must be node identifiers for nodes
 * that exist in the devicetree (if unsure, you can check with
 * DT_NODE_EXISTS()).
 *
 * The expansion evaluates to 0 or 1, but may not be a literal integer
 * 0 or 1.
 *
 * @internal
 * Implementation note: distinct nodes have distinct node identifiers.
 * See include/zephyr/devicetree/ordinals.h.
 * @endinternal
 *
 * @param node_id1 first node identifier
 * @param node_id2 second node identifier
 * @return an expression that evaluates to 1 if the node identifiers
 *         refer to the same node, and evaluates to 0 otherwise
 */
#define DT_SAME_NODE(node_id1, node_id2) \
	(DT_DEP_ORD(node_id1) == (DT_DEP_ORD(node_id2)))

/**
 * @brief Get a devicetree node's node labels as an array of strings
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     foo: bar: node@deadbeef {};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_NODELABEL_STRING_ARRAY(DT_NODELABEL(foo))
 * @endcode
 *
 * This expands to:
 *
 * @code{.c}
 *     { "foo", "bar", }
 * @endcode
 *
 * @param node_id node identifier
 * @return an array initializer for an array of the node's node labels as strings
 */
#define DT_NODELABEL_STRING_ARRAY(node_id) \
	{ DT_FOREACH_NODELABEL(node_id, DT_NODELABEL_STRING_ARRAY_ENTRY_INTERNAL) }

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
 * - boolean: `0` if the property is false, or `1` if it is true
 * - int: the property's value as an integer literal
 * - array, uint8-array, string-array: an initializer expression in braces,
 *   whose elements are integer or string literals (like `{0, 1, 2}`,
 *   `{"hello", "world"}`, etc.)
 * - phandle: a node identifier for the node with that phandle
 *
 * A property's type is usually defined by its binding. In some
 * special cases, it has an assumed type defined by the devicetree
 * specification even when no binding is available: `compatible` has
 * type string-array, `status` has type string, and
 * `interrupt-controller` has type boolean.
 *
 * For other properties or properties with unknown type due to a
 * missing binding, behavior is undefined.
 *
 * For usage examples, see DT_PATH(), DT_ALIAS(), DT_NODELABEL(),
 * and DT_INST() above.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @return a representation of the property's value
 */
#define DT_PROP(node_id, prop) DT_CAT3(node_id, _P_, prop)

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
 * - for type phandle, this expands to 1 (so that a phandle
 *   can be treated as a degenerate case of phandles with length 1)
 * - for type string, this expands to 1 (so that a string can be
 *   treated as a degenerate case of string-array with length 1)
 *
 * These properties are handled as special cases:
 *
 * - reg property: use `DT_NUM_REGS(node_id)` instead
 * - interrupts property: use `DT_NUM_IRQS(node_id)` instead
 *
 * It is an error to use this macro with the `ranges`, `dma-ranges`, `reg`
 * or `interrupts` properties.
 *
 * For other properties, behavior is undefined.
 *
 * @param node_id node identifier
 * @param prop a lowercase-and-underscores property with a logical length
 * @return the property's length
 */
#define DT_PROP_LEN(node_id, prop) DT_CAT4(node_id, _P_, prop, _LEN)

/**
 * @brief Like DT_PROP_LEN(), but with a fallback to @p default_value
 *
 * If the property is defined (as determined by DT_NODE_HAS_PROP()),
 * this expands to DT_PROP_LEN(node_id, prop). The @p default_value
 * parameter is not expanded in this case.
 *
 * Otherwise, this expands to @p default_value.
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
 * @brief Is index @p idx valid for an array type property?
 *
 * If this returns 1, then DT_PROP_BY_IDX(node_id, prop, idx) or
 * DT_PHA_BY_IDX(node_id, prop, idx, ...) are valid at index @p idx.
 * If it returns 0, it is an error to use those macros with that index.
 *
 * These properties are handled as special cases:
 *
 * - `reg` property: use DT_REG_HAS_IDX(node_id, idx) instead
 * - `interrupts` property: use DT_IRQ_HAS_IDX(node_id, idx) instead
 *
 * It is an error to use this macro with the `reg` or `interrupts` properties.
 *
 * @param node_id node identifier
 * @param prop a lowercase-and-underscores property with a logical length
 * @param idx index to check
 * @return An expression which evaluates to 1 if @p idx is a valid index
 *         into the given property, and 0 otherwise.
 */
#define DT_PROP_HAS_IDX(node_id, prop, idx) \
	IS_ENABLED(DT_CAT6(node_id, _P_, prop, _IDX_, idx, _EXISTS))

/**
 * @brief Is name @p name available in a `foo-names` property?
 *
 * This property is handled as special case:
 *
 * - `interrupts` property: use DT_IRQ_HAS_NAME(node_id, idx) instead
 *
 * It is an error to use this macro with the `interrupts` property.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	nx: node-x {
 *		foos = <&bar xx yy>, <&baz xx zz>;
 *		foo-names = "event", "error";
 *		status = "okay";
 *	};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_PROP_HAS_NAME(DT_NODELABEL(nx), foos, event)    // 1
 *     DT_PROP_HAS_NAME(DT_NODELABEL(nx), foos, failure)  // 0
 * @endcode
 *
 * @param node_id node identifier
 * @param prop a lowercase-and-underscores `prop-names` type property
 * @param name a lowercase-and-underscores name to check
 * @return An expression which evaluates to 1 if "name" is an available
 *         name into the given property, and 0 otherwise.
 */
#define DT_PROP_HAS_NAME(node_id, prop, name) \
	IS_ENABLED(DT_CAT6(node_id, _P_, prop, _NAME_, name, _EXISTS))

/**
 * @brief Get the value at index @p idx in an array type property
 *
 * It might help to read the argument order as being similar to
 * `node->property[index]`.
 *
 * The return value depends on the property's type:
 *
 * - for types array, string-array, uint8-array, and phandles,
 *   this expands to the idx-th array element as an
 *   integer, string literal, integer, and node identifier
 *   respectively
 *
 * - for type phandle, idx must be 0 and the expansion is a node
 *   identifier (this treats phandle like a phandles of length 1)
 *
 * - for type string, idx must be 0 and the expansion is the
 *   entire string (this treats string like string-array of length 1)
 *
 * These properties are handled as special cases:
 *
 * - `reg`: use DT_REG_ADDR_BY_IDX() or DT_REG_SIZE_BY_IDX() instead
 * - `interrupts`: use DT_IRQ_BY_IDX()
 * - `ranges`: use DT_NUM_RANGES()
 * - `dma-ranges`: it is an error to use this property with
 *   DT_PROP_BY_IDX()
 *
 * For properties of other types, behavior is undefined.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return a representation of the idx-th element of the property
 */
#define DT_PROP_BY_IDX(node_id, prop, idx) \
	DT_CAT5(node_id, _P_, prop, _IDX_, idx)

/**
 * @brief Like DT_PROP(), but with a fallback to @p default_value
 *
 * If the value exists, this expands to DT_PROP(node_id, prop).
 * The @p default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to @p default_value.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return the property's value or @p default_value
 */
#define DT_PROP_OR(node_id, prop, default_value) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop), \
		    (DT_PROP(node_id, prop)), (default_value))

/**
 * @brief Get a property array value's index into its enumeration values
 *
 * The return values start at zero.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     some_node: some-node {
 *         compat = "vend,enum-string-array";
 *         foos =
 *             <&phandle val1>,
 *             <&phandle val2>,
 *             <&phandle val3>;
 *         foo-names = "default", "option3", "option1";
 *     };
 * @endcode
 *
 * Example bindings fragment:
 *
 * @code{.yaml}
 * compatible: vend,enum-string-array
 * properties:
 *   foos:
 *     type: phandle-array
 *     description: |
 *       Explanation about what this phandle-array exactly is for.
 *
 *   foo-names:
 *     type: string-array
 *     description: |
 *       Some explanation about the available options
 *       default: explain default
 *       option1: explain option1
 *       option2: explain option2
 *       option3: explain option3
 *     enum:
 *       - default
 *       - option1
 *       - option2
 *       - option3
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_ENUM_IDX_BY_IDX(DT_NODELABEL(some_node), foo_names, 0) // 0
 *     DT_ENUM_IDX_BY_IDX(DT_NODELABEL(some_node), foo_names, 2) // 1
 * @endcode
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return zero-based index of the property's value in its enum: list
 */
#define DT_ENUM_IDX_BY_IDX(node_id, prop, idx) \
	DT_CAT6(node_id, _P_, prop, _IDX_, idx, _ENUM_IDX)

/**
 * @brief Equivalent to @ref DT_ENUM_IDX_BY_IDX(node_id, prop, 0).
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @return zero-based index of the property's value in its enum: list
 */
#define DT_ENUM_IDX(node_id, prop) DT_ENUM_IDX_BY_IDX(node_id, prop, 0)

/**
 * @brief Like DT_ENUM_IDX_BY_IDX(), but with a fallback to a default enum index
 *
 * If the value exists, this expands to its zero based index value thanks to
 * DT_ENUM_IDX_BY_IDX(node_id, prop, idx).
 *
 * Otherwise, this expands to provided default index enum value.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @param default_idx_value a fallback index value to expand to
 * @return zero-based index of the property's value in its enum if present,
 *         default_idx_value otherwise
 */
#define DT_ENUM_IDX_BY_IDX_OR(node_id, prop, idx, default_idx_value) \
	COND_CODE_1(DT_PROP_HAS_IDX(node_id, prop, idx), \
		    (DT_ENUM_IDX_BY_IDX(node_id, prop, idx)), (default_idx_value))

/**
 * @brief Equivalent to DT_ENUM_IDX_BY_IDX_OR(node_id, prop, 0, default_idx_value).
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param default_idx_value a fallback index value to expand to
 * @return zero-based index of the property's value in its enum if present,
 *         default_idx_value otherwise
 */
#define DT_ENUM_IDX_OR(node_id, prop, default_idx_value) \
	DT_ENUM_IDX_BY_IDX_OR(node_id, prop, 0, default_idx_value)

/**
 * @brief Does a node enumeration property array have a given value?
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @param value lowercase-and-underscores enumeration value
 * @return 1 if the node property has the value @a value, 0 otherwise.
 */
#define DT_ENUM_HAS_VALUE_BY_IDX(node_id, prop, idx, value) \
	IS_ENABLED(DT_CAT8(node_id, _P_, prop, _IDX_, idx, _ENUM_VAL_, value, _EXISTS))

/**
 * @brief Equivalent to DT_ENUM_HAS_VALUE_BY_IDX(node_id, prop, 0, value).
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param value lowercase-and-underscores enumeration value
 * @return 1 if the node property has the value @a value, 0 otherwise.
 */
#define DT_ENUM_HAS_VALUE(node_id, prop, value) \
	DT_ENUM_HAS_VALUE_BY_IDX(node_id, prop, 0, value)

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
 * @code{.dts}
 *     n1: node-1 {
 *             prop = "foo";
 *     };
 *     n2: node-2 {
 *             prop = "FOO";
 *     }
 *     n3: node-3 {
 *             prop = "123 foo";
 *     };
 * @endcode
 *
 * Example bindings fragment:
 *
 * @code{.yaml}
 *     properties:
 *       prop:
 *         type: string
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_STRING_TOKEN(DT_NODELABEL(n1), prop) // foo
 *     DT_STRING_TOKEN(DT_NODELABEL(n2), prop) // FOO
 *     DT_STRING_TOKEN(DT_NODELABEL(n3), prop) // 123_foo
 * @endcode
 *
 * Notice how:
 *
 * - Unlike C identifiers, the property values may begin with a
 *   number. It's the user's responsibility not to use such values as
 *   the name of a C identifier.
 *
 * - The uppercased `"FOO"` in the DTS remains `FOO` as a token. It is
 *   *not* converted to `foo`.
 *
 * - The whitespace in the DTS `"123 foo"` string is converted to
 *   `123_foo` as a token.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @return the value of @p prop as a token, i.e. without any quotes
 *         and with special characters converted to underscores
 */
#define DT_STRING_TOKEN(node_id, prop) \
	DT_CAT4(node_id, _P_, prop, _STRING_TOKEN)

/**
 * @brief Like DT_STRING_TOKEN(), but with a fallback to @p default_value
 *
 * If the value exists, this expands to DT_STRING_TOKEN(node_id, prop).
 * The @p default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to @p default_value.
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
 * @code{.dts}
 *     n1: node-1 {
 *             prop = "foo";
 *     };
 *     n2: node-2 {
 *             prop = "123 foo";
 *     };
 * @endcode
 *
 * Example bindings fragment:
 *
 * @code{.yaml}
 *     properties:
 *       prop:
 *         type: string
 *
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_STRING_UPPER_TOKEN(DT_NODELABEL(n1), prop) // FOO
 *     DT_STRING_UPPER_TOKEN(DT_NODELABEL(n2), prop) // 123_FOO
 * @endcode
 *
 * Notice how:
 *
 * - Unlike C identifiers, the property values may begin with a
 *   number. It's the user's responsibility not to use such values as
 *   the name of a C identifier.
 *
 * - The lowercased `"foo"` in the DTS becomes `FOO` as a token, i.e.
 *   it is uppercased.
 *
 * - The whitespace in the DTS `"123 foo"` string is converted to
 *   `123_FOO` as a token, i.e. it is uppercased and whitespace becomes
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
 * @brief Like DT_STRING_UPPER_TOKEN(), but with a fallback to @p default_value
 *
 * If the value exists, this expands to DT_STRING_UPPER_TOKEN(node_id, prop).
 * The @p default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to @p default_value.
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

/**
 * @brief Get a string property's value as an unquoted sequence of tokens
 *
 * This removes "the quotes" from string-valued properties.
 * That can be useful, for example,
 * when defining floating point values as a string in devicetree
 * that you would like to use to initialize a float or double variable in C.
 *
 * DT_STRING_UNQUOTED() can only be used for properties with string type.
 *
 * It is an error to use DT_STRING_UNQUOTED() in other circumstances.
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             prop = "12.7";
 *     };
 *     n2: node-2 {
 *             prop = "0.5";
 *     }
 *     n3: node-3 {
 *             prop = "A B C";
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
 *     DT_STRING_UNQUOTED(DT_NODELABEL(n1), prop) // 12.7
 *     DT_STRING_UNQUOTED(DT_NODELABEL(n2), prop) // 0.5
 *     DT_STRING_UNQUOTED(DT_NODELABEL(n3), prop) // A B C
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @return the property's value as a sequence of tokens, with no quotes
 */
#define DT_STRING_UNQUOTED(node_id, prop) \
	DT_CAT4(node_id, _P_, prop, _STRING_UNQUOTED)

/**
 * @brief Like DT_STRING_UNQUOTED(), but with a fallback to @p default_value
 *
 * If the value exists, this expands to DT_STRING_UNQUOTED(node_id, prop).
 * The @p default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to @p default_value.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return the property's value as a sequence of tokens, with no quotes,
 *         or @p default_value
 */
#define DT_STRING_UNQUOTED_OR(node_id, prop, default_value) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop), \
		(DT_STRING_UNQUOTED(node_id, prop)), (default_value))

/**
 * @brief Get an element out of a string-array property as a token.
 *
 * This removes "the quotes" from an element in the array, and converts
 * non-alphanumeric characters to underscores. That can be useful, for example,
 * when programmatically using the value to form a C variable or code.
 *
 * DT_STRING_TOKEN_BY_IDX() can only be used for properties with
 * string-array type.
 *
 * It is an error to use DT_STRING_TOKEN_BY_IDX() in other circumstances.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     n1: node-1 {
 *             prop = "f1", "F2";
 *     };
 *     n2: node-2 {
 *             prop = "123 foo", "456 FOO";
 *     };
 * @endcode
 *
 * Example bindings fragment:
 *
 * @code{.yaml}
 *     properties:
 *       prop:
 *         type: string-array
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_STRING_TOKEN_BY_IDX(DT_NODELABEL(n1), prop, 0) // f1
 *     DT_STRING_TOKEN_BY_IDX(DT_NODELABEL(n1), prop, 1) // F2
 *     DT_STRING_TOKEN_BY_IDX(DT_NODELABEL(n2), prop, 0) // 123_foo
 *     DT_STRING_TOKEN_BY_IDX(DT_NODELABEL(n2), prop, 1) // 456_FOO
 * @endcode
 *
 * For more information, see @ref DT_STRING_TOKEN.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return the element in @p prop at index @p idx as a token
 */
#define DT_STRING_TOKEN_BY_IDX(node_id, prop, idx) \
	DT_CAT6(node_id, _P_, prop, _IDX_, idx, _STRING_TOKEN)

/**
 * @brief Like DT_STRING_TOKEN_BY_IDX(), but uppercased.
 *
 * This removes "the quotes" and capitalizes an element in the array, and
 * converts non-alphanumeric characters to underscores. That can be useful, for
 * example, when programmatically using the value to form a C variable or code.
 *
 * DT_STRING_UPPER_TOKEN_BY_IDX() can only be used for properties with
 * string-array type.
 *
 * It is an error to use DT_STRING_UPPER_TOKEN_BY_IDX() in other circumstances.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     n1: node-1 {
 *             prop = "f1", "F2";
 *     };
 *     n2: node-2 {
 *             prop = "123 foo", "456 FOO";
 *     };
 * @endcode
 *
 * Example bindings fragment:
 *
 * @code{.yaml}
 *     properties:
 *       prop:
 *         type: string-array
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_STRING_UPPER_TOKEN_BY_IDX(DT_NODELABEL(n1), prop, 0) // F1
 *     DT_STRING_UPPER_TOKEN_BY_IDX(DT_NODELABEL(n1), prop, 1) // F2
 *     DT_STRING_UPPER_TOKEN_BY_IDX(DT_NODELABEL(n2), prop, 0) // 123_FOO
 *     DT_STRING_UPPER_TOKEN_BY_IDX(DT_NODELABEL(n2), prop, 1) // 456_FOO
 * @endcode
 *
 * For more information, see @ref DT_STRING_UPPER_TOKEN.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return the element in @p prop at index @p idx as an uppercased token
 */
#define DT_STRING_UPPER_TOKEN_BY_IDX(node_id, prop, idx) \
	DT_CAT6(node_id, _P_, prop, _IDX_, idx, _STRING_UPPER_TOKEN)

/**
 * @brief Get a string array item value as an unquoted sequence of tokens.
 *
 * This removes "the quotes" from string-valued item.
 * That can be useful, for example,
 * when defining floating point values as a string in devicetree
 * that you would like to use to initialize a float or double variable in C.
 *
 * DT_STRING_UNQUOTED_BY_IDX() can only be used for properties with
 * string-array type.
 *
 * It is an error to use DT_STRING_UNQUOTED_BY_IDX() in other circumstances.
 *
 * Example devicetree fragment:
 *
 *     n1: node-1 {
 *             prop = "12.7", "34.1";
 *     };
 *     n2: node-2 {
 *             prop = "A B", "C D";
 *     }
 *
 * Example bindings fragment:
 *
 *     properties:
 *       prop:
 *         type: string-array
 *
 * Example usage:
 *
 *     DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(n1), prop, 0) // 12.7
 *     DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(n1), prop, 1) // 34.1
 *     DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(n2), prop, 0) // A B
 *     DT_STRING_UNQUOTED_BY_IDX(DT_NODELABEL(n2), prop, 1) // C D
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return the property's value as a sequence of tokens, with no quotes
 */
#define DT_STRING_UNQUOTED_BY_IDX(node_id, prop, idx) \
	DT_CAT6(node_id, _P_, prop, _IDX_, idx, _STRING_UNQUOTED)

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
 * @code{.c}
 *     DT_PROP(DT_PHANDLE_BY_IDX(node_id, phs, idx), prop)
 * @endcode
 *
 * That is, @p prop is a property of the phandle's node, not a
 * property of @p node_id.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
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
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     #define N1 DT_NODELABEL(n1)
 *
 *     DT_PROP_BY_PHANDLE_IDX(N1, foo, 0, bar) // 42
 *     DT_PROP_BY_PHANDLE_IDX(N1, foo, 1, baz) // 43
 * @endcode
 *
 * @param node_id node identifier
 * @param phs lowercase-and-underscores property with type `phandle`,
 *            `phandles`, or `phandle-array`
 * @param idx logical index into @p phs, which must be zero if @p phs
 *            has type `phandle`
 * @param prop lowercase-and-underscores property of the phandle's node
 * @return the property's value
 */
#define DT_PROP_BY_PHANDLE_IDX(node_id, phs, idx, prop) \
	DT_PROP(DT_PHANDLE_BY_IDX(node_id, phs, idx), prop)

/**
 * @brief Like DT_PROP_BY_PHANDLE_IDX(), but with a fallback to
 * @p default_value.
 *
 * If the value exists, this expands to DT_PROP_BY_PHANDLE_IDX(node_id, phs,
 * idx, prop). The @p default_value parameter is not expanded in this
 * case.
 *
 * Otherwise, this expands to @p default_value.
 *
 * @param node_id node identifier
 * @param phs lowercase-and-underscores property with type `phandle`,
 *            `phandles`, or `phandle-array`
 * @param idx logical index into @p phs, which must be zero if @p phs
 *            has type `phandle`
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
 * @param ph lowercase-and-underscores property of @p node_id
 *           with type `phandle`
 * @param prop lowercase-and-underscores property of the phandle's node
 * @return the property's value
 */
#define DT_PROP_BY_PHANDLE(node_id, ph, prop) \
	DT_PROP_BY_PHANDLE_IDX(node_id, ph, 0, prop)

/**
 * @brief Get a phandle-array specifier cell value at an index
 *
 * It might help to read the argument order as being similar to
 * `node->phandle_array[index].cell`. That is, the cell value is in
 * the @p pha property of @p node_id, inside the specifier at index
 * @p idx.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     gpio0: gpio@abcd1234 {
 *             #gpio-cells = <2>;
 *     };
 *
 *     gpio1: gpio@1234abcd {
 *             #gpio-cells = <2>;
 *     };
 *
 *     led: led_0 {
 *             gpios = <&gpio0 17 0x1>, <&gpio1 5 0x3>;
 *     };
 * @endcode
 *
 * Bindings fragment for the `gpio0` and `gpio1` nodes:
 *
 * @code{.yaml}
 *     gpio-cells:
 *       - pin
 *       - flags
 * @endcode
 *
 * Above, `gpios` has two elements:
 *
 * - index 0 has specifier <17 0x1>, so its `pin` cell is 17, and its
 *   `flags` cell is 0x1
 * - index 1 has specifier <5 0x3>, so `pin` is 5 and `flags` is 0x3
 *
 * Example usage:
 *
 * @code{.c}
 *     #define LED DT_NODELABEL(led)
 *
 *     DT_PHA_BY_IDX(LED, gpios, 0, pin)   // 17
 *     DT_PHA_BY_IDX(LED, gpios, 1, flags) // 0x3
 * @endcode
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param idx logical index into @p pha
 * @param cell lowercase-and-underscores cell name within the specifier
 *             at @p pha index @p idx
 * @return the cell's value
 */
#define DT_PHA_BY_IDX(node_id, pha, idx, cell) \
	DT_CAT7(node_id, _P_, pha, _IDX_, idx, _VAL_, cell)

/**
 * @brief Like DT_PHA_BY_IDX(), but with a fallback to @p default_value.
 *
 * If the value exists, this expands to DT_PHA_BY_IDX(node_id, pha,
 * idx, cell). The @p default_value parameter is not expanded in this
 * case.
 *
 * Otherwise, this expands to @p default_value.
 *
 * @internal
 * Implementation note: the _IDX_##idx##_VAL_##cell##_EXISTS macros are
 * defined, so it's safe to use DT_PROP_OR() here, because that uses an
 * IS_ENABLED() on the _EXISTS macro.
 * @endinternal
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param idx logical index into @p pha
 * @param cell lowercase-and-underscores cell name within the specifier
 *             at @p pha index @p idx
 * @param default_value a fallback value to expand to
 * @return the cell's value or @p default_value
 */
#define DT_PHA_BY_IDX_OR(node_id, pha, idx, cell, default_value) \
	DT_PROP_OR(node_id, DT_CAT5(pha, _IDX_, idx, _VAL_, cell), default_value)

/**
 * @brief Equivalent to DT_PHA_BY_IDX(node_id, pha, 0, cell)
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param cell lowercase-and-underscores cell name
 * @return the cell's value
 */
#define DT_PHA(node_id, pha, cell) DT_PHA_BY_IDX(node_id, pha, 0, cell)

/**
 * @brief Like DT_PHA(), but with a fallback to @p default_value
 *
 * If the value exists, this expands to DT_PHA(node_id, pha, cell).
 * The @p default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to @p default_value.
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param cell lowercase-and-underscores cell name
 * @param default_value a fallback value to expand to
 * @return the cell's value or @p default_value
 */
#define DT_PHA_OR(node_id, pha, cell, default_value) \
	DT_PHA_BY_IDX_OR(node_id, pha, 0, cell, default_value)

/**
 * @brief Get a value within a phandle-array specifier by name
 *
 * This is like DT_PHA_BY_IDX(), except it treats @p pha as a structure
 * where each array element has a name.
 *
 * It might help to read the argument order as being similar to
 * `node->phandle_struct.name.cell`. That is, the cell value is in the
 * @p pha property of @p node_id, treated as a data structure where
 * each array element has a name.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     n: node {
 *             io-channels = <&adc1 10>, <&adc2 20>;
 *             io-channel-names = "SENSOR", "BANDGAP";
 *     };
 * @endcode
 *
 * Bindings fragment for the "adc1" and "adc2" nodes:
 *
 * @code{.yaml}
 *     io-channel-cells:
 *       - input
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_PHA_BY_NAME(DT_NODELABEL(n), io_channels, sensor, input)  // 10
 *     DT_PHA_BY_NAME(DT_NODELABEL(n), io_channels, bandgap, input) // 20
 * @endcode
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param name lowercase-and-underscores name of a specifier in @p pha
 * @param cell lowercase-and-underscores cell name in the named specifier
 * @return the cell's value
 */
#define DT_PHA_BY_NAME(node_id, pha, name, cell) \
	DT_CAT7(node_id, _P_, pha, _NAME_, name, _VAL_, cell)

/**
 * @brief Like DT_PHA_BY_NAME(), but with a fallback to @p default_value
 *
 * If the value exists, this expands to DT_PHA_BY_NAME(node_id, pha,
 * name, cell). The @p default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to @p default_value.
 *
 * @internal
 * Implementation note: the `_NAME_##name##_VAL_##cell##_EXISTS` macros are
 * defined, so it's safe to use DT_PROP_OR() here, because that uses an
 * IS_ENABLED() on the `_EXISTS` macro.
 * @endinternal
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param name lowercase-and-underscores name of a specifier in @p pha
 * @param cell lowercase-and-underscores cell name in the named specifier
 * @param default_value a fallback value to expand to
 * @return the cell's value or @p default_value
 */
#define DT_PHA_BY_NAME_OR(node_id, pha, name, cell, default_value) \
	DT_PROP_OR(node_id, DT_CAT5(pha, _NAME_, name, _VAL_, cell), default_value)

/**
 * @brief Get a phandle's node identifier from a phandle array by @p name
 *
 * It might help to read the argument order as being similar to
 * `node->phandle_struct.name.phandle`. That is, the phandle array is
 * treated as a structure with named elements. The return value is
 * the node identifier for a phandle inside the structure.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     adc1: adc@abcd1234 {
 *             foobar = "ADC_1";
 *     };
 *
 *     adc2: adc@1234abcd {
 *             foobar = "ADC_2";
 *     };
 *
 *     n: node {
 *             io-channels = <&adc1 10>, <&adc2 20>;
 *             io-channel-names = "SENSOR", "BANDGAP";
 *     };
 * @endcode
 *
 * Above, "io-channels" has two elements:
 *
 * - the element named `"SENSOR"` has phandle `&adc1`
 * - the element named `"BANDGAP"` has phandle `&adc2`
 *
 * Example usage:
 *
 * @code{.c}
 *     #define NODE DT_NODELABEL(n)
 *
 *     DT_PROP(DT_PHANDLE_BY_NAME(NODE, io_channels, sensor), foobar)  // "ADC_1"
 *     DT_PROP(DT_PHANDLE_BY_NAME(NODE, io_channels, bandgap), foobar) // "ADC_2"
 * @endcode
 *
 * Notice how devicetree properties and names are lowercased, and
 * non-alphanumeric characters are converted to underscores.
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param name lowercase-and-underscores name of an element in @p pha
 * @return a node identifier for the node with that phandle
 */
#define DT_PHANDLE_BY_NAME(node_id, pha, name) \
	DT_CAT6(node_id, _P_, pha, _NAME_, name, _PH)

/**
 * @brief Get a node identifier for a phandle in a property.
 *
 * When a node's value at a logical index contains a phandle, this
 * macro returns a node identifier for the node with that phandle.
 *
 * Therefore, if @p prop has type `phandle`, @p idx must be zero. (A
 * `phandle` type is treated as a `phandles` with a fixed length of
 * 1).
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     n1: node-1 {
 *             foo = <&n2 &n3>;
 *     };
 *
 *     n2: node-2 { ... };
 *     n3: node-3 { ... };
 * @endcode
 *
 * Above, `foo` has type phandles and has two elements:
 *
 * - index 0 has phandle `&n2`, which is `node-2`'s phandle
 * - index 1 has phandle `&n3`, which is `node-3`'s phandle
 *
 * Example usage:
 *
 * @code{.c}
 *     #define N1 DT_NODELABEL(n1)
 *
 *     DT_PHANDLE_BY_IDX(N1, foo, 0) // node identifier for node-2
 *     DT_PHANDLE_BY_IDX(N1, foo, 1) // node identifier for node-3
 * @endcode
 *
 * Behavior is analogous for phandle-arrays.
 *
 * @internal
 * Implementation note: using DT_CAT6 above defers concatenation until
 * after expansion of each parameter. This is important when 'idx' is
 * expandable to a number, but it isn't one "yet".
 * @endinternal
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name in @p node_id
 *             with type `phandle`, `phandles` or `phandle-array`
 * @param idx index into @p prop
 * @return node identifier for the node with the phandle at that index
 */
#define DT_PHANDLE_BY_IDX(node_id, prop, idx) \
	DT_CAT6(node_id, _P_, prop, _IDX_, idx, _PH)

/**
 * @brief Get a node identifier for a phandle property's value
 *
 * This is equivalent to DT_PHANDLE_BY_IDX(node_id, prop, 0). Its primary
 * benefit is readability when @p prop has type `phandle`.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property of @p node_id
 *             with type `phandle`
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
 * @code{.dts}
 *     pcie0: pcie@0 {
 *             compatible = "pcie-controller";
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
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_NUM_RANGES(DT_NODELABEL(pcie0)) // 3
 *     DT_NUM_RANGES(DT_NODELABEL(other)) // 2
 * @endcode
 *
 * @param node_id node identifier
 */
#define DT_NUM_RANGES(node_id) DT_CAT(node_id, _RANGES_NUM)

/**
 * @brief Is @p idx a valid range block index?
 *
 * If this returns 1, then DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(node_id, idx),
 * DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(node_id, idx) or
 * DT_RANGES_LENGTH_BY_IDX(node_id, idx) are valid.
 * For DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(node_id, idx) the return value
 * of DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(node_id, idx) will indicate
 * validity.
 * If it returns 0, it is an error to use those macros with index @p idx,
 * including DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(node_id, idx).
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     pcie0: pcie@0 {
 *             compatible = "pcie-controller";
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
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(pcie0), 0) // 1
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(pcie0), 1) // 1
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(pcie0), 2) // 1
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(pcie0), 3) // 0
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(other), 0) // 1
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(other), 1) // 1
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(other), 2) // 0
 *     DT_RANGES_HAS_IDX(DT_NODELABEL(other), 3) // 0
 * @endcode
 *
 * @param node_id node identifier
 * @param idx index to check
 * @return 1 if @p idx is a valid register block index,
 *         0 otherwise.
 */
#define DT_RANGES_HAS_IDX(node_id, idx) \
	IS_ENABLED(DT_CAT4(node_id, _RANGES_IDX_, idx, _EXISTS))

/**
 * @brief Does a ranges property have child bus flags at index?
 *
 * If this returns 1, then DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(node_id, idx) is valid.
 * If it returns 0, it is an error to use this macro with index @p idx.
 * This macro only returns 1 for PCIe buses (i.e. nodes whose bindings specify they
 * are "pcie" bus nodes.)
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     parent {
 *             #address-cells = <2>;
 *
 *             pcie0: pcie@0 {
 *                     compatible = "pcie-controller";
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
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(pcie0), 0) // 1
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(pcie0), 1) // 1
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(pcie0), 2) // 1
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(pcie0), 3) // 0
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(other), 0) // 0
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(other), 1) // 0
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(other), 2) // 0
 *     DT_RANGES_HAS_CHILD_BUS_FLAGS_AT_IDX(DT_NODELABEL(other), 3) // 0
 * @endcode
 *
 * @param node_id node identifier
 * @param idx logical index into the ranges array
 * @return 1 if @p idx is a valid child bus flags index,
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
 * @code{.dts}
 *     parent {
 *             #address-cells = <2>;
 *
 *             pcie0: pcie@0 {
 *                     compatible = "pcie-controller";
 *                     reg = <0 0 1>;
 *                     #address-cells = <3>;
 *                     #size-cells = <2>;
 *
 *                     ranges = <0x1000000 0 0 0 0x3eff0000 0 0x10000>,
 *                              <0x2000000 0 0x10000000 0 0x10000000 0 0x2eff0000>,
 *                              <0x3000000 0x80 0 0x80 0 0x80 0>;
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(DT_NODELABEL(pcie0), 0) // 0x1000000
 *     DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(DT_NODELABEL(pcie0), 1) // 0x2000000
 *     DT_RANGES_CHILD_BUS_FLAGS_BY_IDX(DT_NODELABEL(pcie0), 2) // 0x3000000
 * @endcode
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
 * @code{.dts}
 *     parent {
 *             #address-cells = <2>;
 *
 *             pcie0: pcie@0 {
 *                     compatible = "pcie-controller";
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
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_NODELABEL(pcie0), 0) // 0
 *     DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_NODELABEL(pcie0), 1) // 0x10000000
 *     DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_NODELABEL(pcie0), 2) // 0x8000000000
 *     DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_NODELABEL(other), 0) // 0
 *     DT_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_NODELABEL(other), 1) // 0x10000000
 * @endcode
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
 * @code{.dts}
 *     parent {
 *             #address-cells = <2>;
 *
 *             pcie0: pcie@0 {
 *                     compatible = "pcie-controller";
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
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(DT_NODELABEL(pcie0), 0) // 0x3eff0000
 *     DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(DT_NODELABEL(pcie0), 1) // 0x10000000
 *     DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(DT_NODELABEL(pcie0), 2) // 0x8000000000
 *     DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(DT_NODELABEL(other), 0) // 0x3eff0000
 *     DT_RANGES_PARENT_BUS_ADDRESS_BY_IDX(DT_NODELABEL(other), 1) // 0x10000000
 * @endcode
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
 * @code{.dts}
 *     parent {
 *             #address-cells = <2>;
 *
 *             pcie0: pcie@0 {
 *                     compatible = "pcie-controller";
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
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_RANGES_LENGTH_BY_IDX(DT_NODELABEL(pcie0), 0) // 0x10000
 *     DT_RANGES_LENGTH_BY_IDX(DT_NODELABEL(pcie0), 1) // 0x2eff0000
 *     DT_RANGES_LENGTH_BY_IDX(DT_NODELABEL(pcie0), 2) // 0x8000000000
 *     DT_RANGES_LENGTH_BY_IDX(DT_NODELABEL(other), 0) // 0x10000
 *     DT_RANGES_LENGTH_BY_IDX(DT_NODELABEL(other), 1) // 0x2eff0000
 * @endcode
 *
 * @param node_id node identifier
 * @param idx logical index into the ranges array
 * @returns range length field at idx
 */
#define DT_RANGES_LENGTH_BY_IDX(node_id, idx) \
	DT_CAT4(node_id, _RANGES_IDX_, idx, _VAL_LENGTH)

/**
 * @brief Invokes @p fn for each entry of @p node_id ranges property
 *
 * The macro @p fn must take two parameters, @p node_id which will be the node
 * identifier of the node with the ranges property and @p idx the index of
 * the ranges block.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     n: node@0 {
 *             reg = <0 0 1>;
 *
 *             ranges = <0x0 0x0 0x0 0x3eff0000 0x10000>,
 *                      <0x0 0x10000000 0x0 0x10000000 0x2eff0000>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     #define RANGE_LENGTH(node_id, idx) DT_RANGES_LENGTH_BY_IDX(node_id, idx),
 *
 *     const uint64_t *ranges_length[] = {
 *             DT_FOREACH_RANGE(DT_NODELABEL(n), RANGE_LENGTH)
 *     };
 * @endcode
 *
 * This expands to:
 *
 * @code{.c}
 *     const char *ranges_length[] = {
 *         0x10000, 0x2eff0000,
 *     };
 * @endcode
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
 * @defgroup devicetree-generic-vendor Vendor and model name helpers
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get the vendor at index @p idx as a string literal
 *
 * The vendor is a string extracted from vendor prefixes if an entry exists
 * that matches the node's compatible prefix. There may be as many as one
 * vendor prefixes file per directory in DTS_ROOT.
 *
 * Example vendor-prefixes.txt:
 *
 *	vnd	A stand-in for a real vendor
 *	zephyr	Zephyr-specific binding
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *	n1: node-1 {
 *		compatible = "vnd,model1", "gpio", "zephyr,model2";
 *	};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *	DT_NODE_VENDOR_BY_IDX(DT_NODELABEL(n1), 0) // "A stand-in for a real vendor"
 *	DT_NODE_VENDOR_BY_IDX(DT_NODELABEL(n1), 2) // "Zephyr-specific binding"
 * @endcode
 *
 * Notice that the compatible at index 1 doesn't match any entries in the
 * vendor prefix file and therefore index 1 is not a valid vendor index. Use
 * DT_NODE_VENDOR_HAS_IDX(node_id, idx) to determine if an index is valid.
 *
 * @param node_id node identifier
 * @param idx index of the vendor to return
 * @return string literal of the idx-th vendor
 */
#define DT_NODE_VENDOR_BY_IDX(node_id, idx) \
	DT_CAT3(node_id, _COMPAT_VENDOR_IDX_, idx)

/**
 * @brief Does a node's compatible property have a vendor at an index?
 *
 * If this returns 1, then DT_NODE_VENDOR_BY_IDX(node_id, idx) is valid. If it
 * returns 0, it is an error to use DT_NODE_VENDOR_BY_IDX(node_id, idx) with
 * index @p idx.
 *
 * @param node_id node identifier
 * @param idx index of the vendor to check
 * @return 1 if @p idx is a valid vendor index,
 *         0 otherwise.
 */
#define DT_NODE_VENDOR_HAS_IDX(node_id, idx) \
	IS_ENABLED(DT_CAT4(node_id, _COMPAT_VENDOR_IDX_, idx, _EXISTS))

/**
 * @brief Like DT_NODE_VENDOR_BY_IDX(), but with a fallback to default_value.
 *
 * If the value exists, this expands to DT_NODE_VENDOR_BY_IDX(node_id, idx).
 * The default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to default_value.
 *
 * @param node_id node identifier
 * @param idx index of the vendor to return
 * @return string literal of the idx-th vendor
 * @param default_value a fallback value to expand to
 * @return string literal of the idx-th vendor or "default_value"
 */
#define DT_NODE_VENDOR_BY_IDX_OR(node_id, idx, default_value) \
	COND_CODE_1(DT_NODE_VENDOR_HAS_IDX(node_id, idx), \
		    (DT_NODE_VENDOR_BY_IDX(node_id, idx)), (default_value))

/**
 * @brief Get the node's (only) vendor as a string literal
 *
 * Equivalent to DT_NODE_VENDOR_BY_IDX_OR(node_id, 0, default_value).
 *
 * @param node_id node identifier
 * @param default_value a fallback value to expand to
 */
#define DT_NODE_VENDOR_OR(node_id, default_value) \
	DT_NODE_VENDOR_BY_IDX_OR(node_id, 0, default_value)

/**
 * @brief Get the model at index "idx" as a string literal
 *
 * The model is a string extracted from the compatible after the vendor prefix.
 *
 * Example vendor-prefixes.txt:
 *
 *	vnd	A stand-in for a real vendor
 *	zephyr	Zephyr-specific binding
 *
 * Example devicetree fragment:
 *
 *	n1: node-1 {
 *		compatible = "vnd,model1", "gpio", "zephyr,model2";
 *	};
 *
 * Example usage:
 *
 *	DT_NODE_MODEL_BY_IDX(DT_NODELABEL(n1), 0) // "model1"
 *	DT_NODE_MODEL_BY_IDX(DT_NODELABEL(n1), 2) // "model2"
 *
 * Notice that the compatible at index 1 doesn't match any entries in the
 * vendor prefix file and therefore index 1 is not a valid model index. Use
 * DT_NODE_MODEL_HAS_IDX(node_id, idx) to determine if an index is valid.
 *
 * @param node_id node identifier
 * @param idx index of the model to return
 * @return string literal of the idx-th model
 */
#define DT_NODE_MODEL_BY_IDX(node_id, idx) \
	DT_CAT3(node_id, _COMPAT_MODEL_IDX_, idx)

/**
 * @brief Does a node's compatible property have a model at an index?
 *
 * If this returns 1, then DT_NODE_MODEL_BY_IDX(node_id, idx) is valid. If it
 * returns 0, it is an error to use DT_NODE_MODEL_BY_IDX(node_id, idx) with
 * index "idx".
 *
 * @param node_id node identifier
 * @param idx index of the model to check
 * @return 1 if "idx" is a valid model index,
 *         0 otherwise.
 */
#define DT_NODE_MODEL_HAS_IDX(node_id, idx) \
	IS_ENABLED(DT_CAT4(node_id, _COMPAT_MODEL_IDX_, idx, _EXISTS))

/**
 * @brief Like DT_NODE_MODEL_BY_IDX(), but with a fallback to default_value.
 *
 * If the value exists, this expands to DT_NODE_MODEL_BY_IDX(node_id, idx).
 * The default_value parameter is not expanded in this case.
 *
 * Otherwise, this expands to default_value.
 *
 * @param node_id node identifier
 * @param idx index of the model to return
 * @return string literal of the idx-th model
 * @param default_value a fallback value to expand to
 * @return string literal of the idx-th model or "default_value"
 */
#define DT_NODE_MODEL_BY_IDX_OR(node_id, idx, default_value) \
	COND_CODE_1(DT_NODE_MODEL_HAS_IDX(node_id, idx), \
		    (DT_NODE_MODEL_BY_IDX(node_id, idx)), (default_value))

/**
 * @brief Get the node's (only) model as a string literal
 *
 * Equivalent to DT_NODE_MODEL_BY_IDX_OR(node_id, 0, default_value).
 *
 * @param node_id node identifier
 * @param default_value a fallback value to expand to
 */
#define DT_NODE_MODEL_OR(node_id, default_value) \
	DT_NODE_MODEL_BY_IDX_OR(node_id, 0, default_value)

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
 * @brief Is @p idx a valid register block index?
 *
 * If this returns 1, then DT_REG_ADDR_BY_IDX(node_id, idx) or
 * DT_REG_SIZE_BY_IDX(node_id, idx) are valid.
 * If it returns 0, it is an error to use those macros with index @p idx.
 * @param node_id node identifier
 * @param idx index to check
 * @return 1 if @p idx is a valid register block index,
 *         0 otherwise.
 */
#define DT_REG_HAS_IDX(node_id, idx) \
	IS_ENABLED(DT_CAT4(node_id, _REG_IDX_, idx, _EXISTS))

/**
 * @brief Is @p name a valid register block name?
 *
 * If this returns 1, then DT_REG_ADDR_BY_NAME(node_id, name) or
 * DT_REG_SIZE_BY_NAME(node_id, name) are valid.
 * If it returns 0, it is an error to use those macros with name @p name.
 * @param node_id node identifier
 * @param name name to check
 * @return 1 if @p name is a valid register block name,
 *         0 otherwise.
 */
#define DT_REG_HAS_NAME(node_id, name) \
	IS_ENABLED(DT_CAT4(node_id, _REG_NAME_, name, _EXISTS))

/**
 * @brief Get the base raw address of the register block at index @p idx
 *
 * Get the base address of the register block at index @p idx without any
 * type suffix. This can be used to index other devicetree properties, use the
 * non _RAW macros for assigning values in actual code.
 *
 * @param node_id node identifier
 * @param idx index of the register whose address to return
 * @return address of the idx-th register block
 */
#define DT_REG_ADDR_BY_IDX_RAW(node_id, idx) \
	DT_CAT4(node_id, _REG_IDX_, idx, _VAL_ADDRESS)

/**
 * @brief Get a node's (only) register block raw address
 *
 * Get a node's only register block address without any type suffix. This can
 * be used to index other devicetree properties, use the non _RAW macros for
 * assigning values in actual code.
 *
 * Equivalent to DT_REG_ADDR_BY_IDX_RAW(node_id, 0).
 * @param node_id node identifier
 * @return node's register block address
 */
#define DT_REG_ADDR_RAW(node_id) \
	DT_REG_ADDR_BY_IDX_RAW(node_id, 0)

/**
 * @brief Get the base address of the register block at index @p idx
 * @param node_id node identifier
 * @param idx index of the register whose address to return
 * @return address of the idx-th register block
 */
#define DT_REG_ADDR_BY_IDX(node_id, idx) \
	DT_U32_C(DT_REG_ADDR_BY_IDX_RAW(node_id, idx))

/**
 * @brief Get the size of the register block at index @p idx
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
	DT_U32_C(DT_CAT4(node_id, _REG_IDX_, idx, _VAL_SIZE))

/**
 * @brief Get a node's (only) register block address
 *
 * Equivalent to DT_REG_ADDR_BY_IDX(node_id, 0).
 * @param node_id node identifier
 * @return node's register block address
 */
#define DT_REG_ADDR(node_id) DT_REG_ADDR_BY_IDX(node_id, 0)

/**
 * @brief 64-bit version of DT_REG_ADDR()
 *
 * This macro version adds the appropriate suffix for 64-bit unsigned
 * integer literals.
 * Note that this macro is equivalent to DT_REG_ADDR() in linker/ASM context.
 *
 * @param node_id node identifier
 * @return node's register block address
 */
#define DT_REG_ADDR_U64(node_id) DT_U64_C(DT_REG_ADDR_BY_IDX_RAW(node_id, 0))

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
	DT_U32_C(DT_CAT4(node_id, _REG_NAME_, name, _VAL_ADDRESS))

/**
 * @brief Like DT_REG_ADDR_BY_NAME(), but with a fallback to @p default_value
 * @param node_id node identifier
 * @param name lowercase-and-underscores register specifier name
 * @param default_value a fallback value to expand to
 * @return address of the register block specified by name if present,
 *         @p default_value otherwise
 */
#define DT_REG_ADDR_BY_NAME_OR(node_id, name, default_value) \
	COND_CODE_1(DT_REG_HAS_NAME(node_id, name), \
		    (DT_REG_ADDR_BY_NAME(node_id, name)), (default_value))

/**
 * @brief 64-bit version of DT_REG_ADDR_BY_NAME()
 *
 * This macro version adds the appropriate suffix for 64-bit unsigned
 * integer literals.
 * Note that this macro is equivalent to DT_REG_ADDR_BY_NAME() in
 * linker/ASM context.
 *
 * @param node_id node identifier
 * @param name lowercase-and-underscores register specifier name
 * @return address of the register block specified by name
 */
#define DT_REG_ADDR_BY_NAME_U64(node_id, name) \
	DT_U64_C(DT_CAT4(node_id, _REG_NAME_, name, _VAL_ADDRESS))

/**
 * @brief Get a register block's size by name
 * @param node_id node identifier
 * @param name lowercase-and-underscores register specifier name
 * @return size of the register block specified by name
 */
#define DT_REG_SIZE_BY_NAME(node_id, name) \
	DT_U32_C(DT_CAT4(node_id, _REG_NAME_, name, _VAL_SIZE))

/**
 * @brief Like DT_REG_SIZE_BY_NAME(), but with a fallback to @p default_value
 * @param node_id node identifier
 * @param name lowercase-and-underscores register specifier name
 * @param default_value a fallback value to expand to
 * @return size of the register block specified by name if present,
 *         @p default_value otherwise
 */
#define DT_REG_SIZE_BY_NAME_OR(node_id, name, default_value) \
	COND_CODE_1(DT_REG_HAS_NAME(node_id, name), \
		    (DT_REG_SIZE_BY_NAME(node_id, name)), (default_value))


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
 * @brief Get the number of node labels that a node has
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     / {
 *         foo {};
 *         bar: bar@1000 {};
 *         baz: baz2: baz@2000 {};
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_NUM_NODELABELS(DT_PATH(foo))      // 0
 *     DT_NUM_NODELABELS(DT_NODELABEL(bar)) // 1
 *     DT_NUM_NODELABELS(DT_NODELABEL(baz)) // 2
 * @endcode
 *
 * @param node_id node identifier
 * @return number of node labels that the node has
 */
#define DT_NUM_NODELABELS(node_id) DT_CAT(node_id, _NODELABEL_NUM)

/**
 * @brief Get the interrupt level for the node
 *
 * @param node_id node identifier
 * @return interrupt level
 */
#define DT_IRQ_LEVEL(node_id) DT_CAT(node_id, _IRQ_LEVEL)

/**
 * @brief Is @p idx a valid interrupt index?
 *
 * If this returns 1, then DT_IRQ_BY_IDX(node_id, idx) is valid.
 * If it returns 0, it is an error to use that macro with this index.
 * @param node_id node identifier
 * @param idx index to check
 * @return 1 if the idx is valid for the interrupt property
 *         0 otherwise.
 */
#define DT_IRQ_HAS_IDX(node_id, idx) \
	IS_ENABLED(DT_CAT4(node_id, _IRQ_IDX_, idx, _EXISTS))

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
	IS_ENABLED(DT_CAT6(node_id, _IRQ_IDX_, idx, _VAL_, cell, _EXISTS))

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
	IS_ENABLED(DT_CAT4(node_id, _IRQ_NAME_, name, _VAL_irq_EXISTS))

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
 * @code{.dts}
 *     my-serial: serial@abcd1234 {
 *             interrupts = < 33 0 >, < 34 1 >;
 *     };
 * @endcode
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
	DT_CAT5(node_id, _IRQ_IDX_, idx, _VAL_, cell)

/**
 * @brief Get a value within an interrupt specifier by name
 *
 * It might help to read the argument order as being similar to
 * `node->interrupts.name.cell`.
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
	DT_CAT5(node_id, _IRQ_NAME_, name, _VAL_, cell)

/**
 * @brief Get an interrupt specifier's value
 * Equivalent to DT_IRQ_BY_IDX(node_id, 0, cell).
 * @param node_id node identifier
 * @param cell cell name specifier
 * @return the named value at that index
 */
#define DT_IRQ(node_id, cell) DT_IRQ_BY_IDX(node_id, 0, cell)

/**
 * @brief Get an interrupt specifier's interrupt controller by index
 *
 * @code{.dts}
 *     gpio0: gpio0 {
 *             interrupt-controller;
 *             #interrupt-cells = <2>;
 *     };
 *
 *     foo: foo {
 *             interrupt-parent = <&gpio0>;
 *             interrupts = <1 1>, <2 2>;
 *     };
 *
 *     bar: bar {
 *             interrupts-extended = <&gpio0 3 3>, <&pic0 4>;
 *     };
 *
 *     pic0: pic0 {
 *             interrupt-controller;
 *             #interrupt-cells = <1>;
 *
 *             qux: qux {
 *                     interrupts = <5>, <6>;
 *                     interrupt-names = "int1", "int2";
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     DT_IRQ_INTC_BY_IDX(DT_NODELABEL(foo), 0) // &gpio0
 *     DT_IRQ_INTC_BY_IDX(DT_NODELABEL(foo), 1) // &gpio0
 *     DT_IRQ_INTC_BY_IDX(DT_NODELABEL(bar), 0) // &gpio0
 *     DT_IRQ_INTC_BY_IDX(DT_NODELABEL(bar), 1) // &pic0
 *     DT_IRQ_INTC_BY_IDX(DT_NODELABEL(qux), 0) // &pic0
 *     DT_IRQ_INTC_BY_IDX(DT_NODELABEL(qux), 1) // &pic0
 *
 * @param node_id node identifier
 * @param idx interrupt specifier's index
 * @return node_id of interrupt specifier's interrupt controller
 */
#define DT_IRQ_INTC_BY_IDX(node_id, idx) \
	DT_CAT4(node_id, _IRQ_IDX_, idx, _CONTROLLER)

/**
 * @brief Get an interrupt specifier's interrupt controller by name
 *
 * @code{.dts}
 *     gpio0: gpio0 {
 *             interrupt-controller;
 *             #interrupt-cells = <2>;
 *     };
 *
 *     foo: foo {
 *             interrupt-parent = <&gpio0>;
 *             interrupts = <1 1>, <2 2>;
 *             interrupt-names = "int1", "int2";
 *     };
 *
 *     bar: bar {
 *             interrupts-extended = <&gpio0 3 3>, <&pic0 4>;
 *             interrupt-names = "int1", "int2";
 *     };
 *
 *     pic0: pic0 {
 *             interrupt-controller;
 *             #interrupt-cells = <1>;
 *
 *             qux: qux {
 *                     interrupts = <5>, <6>;
 *                     interrupt-names = "int1", "int2";
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     DT_IRQ_INTC_BY_NAME(DT_NODELABEL(foo), int1) // &gpio0
 *     DT_IRQ_INTC_BY_NAME(DT_NODELABEL(foo), int2) // &gpio0
 *     DT_IRQ_INTC_BY_NAME(DT_NODELABEL(bar), int1) // &gpio0
 *     DT_IRQ_INTC_BY_NAME(DT_NODELABEL(bar), int2) // &pic0
 *     DT_IRQ_INTC_BY_NAME(DT_NODELABEL(qux), int1) // &pic0
 *     DT_IRQ_INTC_BY_NAME(DT_NODELABEL(qux), int2) // &pic0
 *
 * @param node_id node identifier
 * @param name interrupt specifier's name
 * @return node_id of interrupt specifier's interrupt controller
 */
#define DT_IRQ_INTC_BY_NAME(node_id, name) \
	DT_CAT4(node_id, _IRQ_NAME_, name, _CONTROLLER)

/**
 * @brief Get an interrupt specifier's interrupt controller
 * @note Equivalent to DT_IRQ_INTC_BY_IDX(node_id, 0)
 *
 * @code{.dts}
 *     gpio0: gpio0 {
 *             interrupt-controller;
 *             #interrupt-cells = <2>;
 *     };
 *
 *     foo: foo {
 *             interrupt-parent = <&gpio0>;
 *             interrupts = <1 1>;
 *     };
 *
 *     bar: bar {
 *             interrupts-extended = <&gpio0 3 3>;
 *     };
 *
 *     pic0: pic0 {
 *             interrupt-controller;
 *             #interrupt-cells = <1>;
 *
 *             qux: qux {
 *                     interrupts = <5>;
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 *     DT_IRQ_INTC(DT_NODELABEL(foo)) // &gpio0
 *     DT_IRQ_INTC(DT_NODELABEL(bar)) // &gpio0
 *     DT_IRQ_INTC(DT_NODELABEL(qux)) // &pic0
 *
 * @param node_id node identifier
 * @return node_id of interrupt specifier's interrupt controller
 * @see DT_IRQ_INTC_BY_IDX()
 */
#define DT_IRQ_INTC(node_id) \
	DT_IRQ_INTC_BY_IDX(node_id, 0)

/**
 * @cond INTERNAL_HIDDEN
 */

/* DT helper macro to encode a node's IRQN to level 1 according to the multi-level scheme */
#define DT_IRQN_L1_INTERNAL(node_id, idx) DT_IRQ_BY_IDX(node_id, idx, irq)
/* DT helper macro to encode a node's IRQN to level 2 according to the multi-level scheme */
#define DT_IRQN_L2_INTERNAL(node_id, idx)                                                          \
	(IRQ_TO_L2(DT_IRQN_L1_INTERNAL(node_id, idx)) |                                            \
	 DT_IRQ(DT_IRQ_INTC_BY_IDX(node_id, idx), irq))
/* DT helper macro to encode a node's IRQN to level 3 according to the multi-level scheme */
#define DT_IRQN_L3_INTERNAL(node_id, idx)                                                          \
	(IRQ_TO_L3(DT_IRQN_L1_INTERNAL(node_id, idx)) |                                            \
	 IRQ_TO_L2(DT_IRQ(DT_IRQ_INTC_BY_IDX(node_id, idx), irq)) |                                \
	 DT_IRQ(DT_IRQ_INTC(DT_IRQ_INTC_BY_IDX(node_id, idx)), irq))
/* DT helper macro for the macros above */
#define DT_IRQN_LVL_INTERNAL(node_id, idx, level) DT_CAT3(DT_IRQN_L, level, _INTERNAL)(node_id, idx)

/**
 * DT helper macro to encode a node's interrupt number according to the Zephyr's multi-level scheme
 * See doc/kernel/services/interrupts.rst for details
 */
#define DT_MULTI_LEVEL_IRQN_INTERNAL(node_id, idx)                                                 \
	DT_IRQN_LVL_INTERNAL(node_id, idx, DT_IRQ_LEVEL(node_id))

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Get the node's Zephyr interrupt number at index
 * If @kconfig{CONFIG_MULTI_LEVEL_INTERRUPTS} is enabled, the interrupt number at index will be
 * multi-level encoded
 * @param node_id node identifier
 * @param idx logical index into the interrupt specifier array
 * @return the Zephyr interrupt number
 */
#define DT_IRQN_BY_IDX(node_id, idx)                                                               \
	COND_CODE_1(IS_ENABLED(CONFIG_MULTI_LEVEL_INTERRUPTS),                                     \
		    (DT_MULTI_LEVEL_IRQN_INTERNAL(node_id, idx)),                                  \
		    (DT_IRQ_BY_IDX(node_id, idx, irq)))

/**
 * @brief Get a node's (only) irq number
 *
 * Equivalent to DT_IRQ(node_id, irq). This is provided as a convenience
 * for the common case where a node generates exactly one interrupt,
 * and the IRQ number is in a cell named `irq`.
 *
 * @param node_id node identifier
 * @return the interrupt number for the node's only interrupt
 */
#define DT_IRQN(node_id) DT_IRQN_BY_IDX(node_id, 0)

/**
 * @}
 */

/**
 * @defgroup devicetree-generic-chosen Chosen nodes
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get a node identifier for a `/chosen` node property
 *
 * This is only valid to call if `DT_HAS_CHOSEN(prop)` is 1.
 * @param prop lowercase-and-underscores property name for
 *             the /chosen node
 * @return a node identifier for the chosen node property
 */
#define DT_CHOSEN(prop) DT_CAT(DT_CHOSEN_, prop)

/**
 * @brief Test if the devicetree has a `/chosen` node
 * @param prop lowercase-and-underscores devicetree property
 * @return 1 if the chosen property exists and refers to a node,
 *         0 otherwise
 */
#define DT_HAS_CHOSEN(prop) IS_ENABLED(DT_CAT3(DT_CHOSEN_, prop, _EXISTS))

/**
 * @}
 */

/**
 * @defgroup devicetree-generic-foreach "For-each" macros
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Invokes @p fn for every node in the tree.
 *
 * The macro @p fn must take one parameter, which will be a node
 * identifier. The macro is expanded once for each node in the tree.
 * The order that nodes are visited in is not specified.
 *
 * @param fn macro to invoke
 */
#define DT_FOREACH_NODE(fn) DT_FOREACH_HELPER(fn)

/**
 * @brief Invokes @p fn for every node in the tree with multiple arguments.
 *
 * The macro @p fn takes multiple arguments. The first should be the node
 * identifier for the node. The remaining are passed-in by the caller.
 *
 * The macro is expanded once for each node in the tree. The order that nodes
 * are visited in is not specified.
 *
 * @param fn macro to invoke
 * @param ... variable number of arguments to pass to @p fn
 */
#define DT_FOREACH_NODE_VARGS(fn, ...) DT_FOREACH_VARGS_HELPER(fn, __VA_ARGS__)

/**
 * @brief Invokes @p fn for every status `okay` node in the tree.
 *
 * The macro @p fn must take one parameter, which will be a node
 * identifier. The macro is expanded once for each node in the tree
 * with status `okay` (as usual, a missing status property is treated
 * as status `okay`). The order that nodes are visited in is not
 * specified.
 *
 * @param fn macro to invoke
 */
#define DT_FOREACH_STATUS_OKAY_NODE(fn) DT_FOREACH_OKAY_HELPER(fn)

/**
 * @brief Invokes @p fn for every status `okay` node in the tree with multiple
 *        arguments.
 *
 * The macro @p fn takes multiple arguments. The first should be the node
 * identifier for the node. The remaining are passed-in by the caller.
 *
 * The macro is expanded once for each node in the tree with status `okay` (as
 * usual, a missing status property is treated as status `okay`). The order
 * that nodes are visited in is not specified.
 *
 * @param fn macro to invoke
 * @param ... variable number of arguments to pass to @p fn
 */
#define DT_FOREACH_STATUS_OKAY_NODE_VARGS(fn, ...) DT_FOREACH_OKAY_VARGS_HELPER(fn, __VA_ARGS__)

/**
 * @brief Invokes @p fn for each child of @p node_id
 *
 * The macro @p fn must take one parameter, which will be the node
 * identifier of a child node of @p node_id.
 *
 * The children will be iterated over in the same order as they
 * appear in the final devicetree.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     n: node {
 *             child-1 {
 *                     foobar = "foo";
 *             };
 *             child-2 {
 *                     foobar = "bar";
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     #define FOOBAR_AND_COMMA(node_id) DT_PROP(node_id, foobar),
 *
 *     const char *child_foobars[] = {
 *         DT_FOREACH_CHILD(DT_NODELABEL(n), FOOBAR_AND_COMMA)
 *     };
 * @endcode
 *
 * This expands to:
 *
 * @code{.c}
 *     const char *child_foobars[] = {
 *         "foo", "bar",
 *     };
 * @endcode
 *
 * @param node_id node identifier
 * @param fn macro to invoke
 */
#define DT_FOREACH_CHILD(node_id, fn) \
	DT_CAT(node_id, _FOREACH_CHILD)(fn)

/**
 * @brief Invokes @p fn for each child of @p node_id with a separator
 *
 * The macro @p fn must take one parameter, which will be the node
 * identifier of a child node of @p node_id.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     n: node {
 *             child-1 {
 *                     ...
 *             };
 *             child-2 {
 *                     ...
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     const char *child_names[] = {
 *         DT_FOREACH_CHILD_SEP(DT_NODELABEL(n), DT_NODE_FULL_NAME, (,))
 *     };
 * @endcode
 *
 * This expands to:
 *
 * @code{.c}
 *     const char *child_names[] = {
 *         "child-1", "child-2"
 *     };
 * @endcode
 *
 * @param node_id node identifier
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 */
#define DT_FOREACH_CHILD_SEP(node_id, fn, sep) \
	DT_CAT(node_id, _FOREACH_CHILD_SEP)(fn, sep)

/**
 * @brief Invokes @p fn for each child of @p node_id with multiple arguments
 *
 * The macro @p fn takes multiple arguments. The first should be the node
 * identifier for the child node. The remaining are passed-in by the caller.
 *
 * The children will be iterated over in the same order as they
 * appear in the final devicetree.
 *
 * @param node_id node identifier
 * @param fn macro to invoke
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_CHILD
 */
#define DT_FOREACH_CHILD_VARGS(node_id, fn, ...) \
	DT_CAT(node_id, _FOREACH_CHILD_VARGS)(fn, __VA_ARGS__)

/**
 * @brief Invokes @p fn for each child of @p node_id with separator and multiple
 *        arguments.
 *
 * The macro @p fn takes multiple arguments. The first should be the node
 * identifier for the child node. The remaining are passed-in by the caller.
 *
 * @param node_id node identifier
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_CHILD_VARGS
 */
#define DT_FOREACH_CHILD_SEP_VARGS(node_id, fn, sep, ...) \
	DT_CAT(node_id, _FOREACH_CHILD_SEP_VARGS)(fn, sep, __VA_ARGS__)

/**
 * @brief Call @p fn on the child nodes with status `okay`
 *
 * The macro @p fn should take one argument, which is the node
 * identifier for the child node.
 *
 * As usual, both a missing status and an `ok` status are
 * treated as `okay`.
 *
 * The children will be iterated over in the same order as they
 * appear in the final devicetree.
 *
 * @param node_id node identifier
 * @param fn macro to invoke
 */
#define DT_FOREACH_CHILD_STATUS_OKAY(node_id, fn) \
	DT_CAT(node_id, _FOREACH_CHILD_STATUS_OKAY)(fn)

/**
 * @brief Call @p fn on the child nodes with status `okay` with separator
 *
 * The macro @p fn should take one argument, which is the node
 * identifier for the child node.
 *
 * As usual, both a missing status and an `ok` status are
 * treated as `okay`.
 *
 * @param node_id node identifier
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 *
 * @see DT_FOREACH_CHILD_STATUS_OKAY
 */
#define DT_FOREACH_CHILD_STATUS_OKAY_SEP(node_id, fn, sep) \
	DT_CAT(node_id, _FOREACH_CHILD_STATUS_OKAY_SEP)(fn, sep)

/**
 * @brief Call @p fn on the child nodes with status `okay` with multiple
 * arguments
 *
 * The macro @p fn takes multiple arguments. The first should be the node
 * identifier for the child node. The remaining are passed-in by the caller.
 *
 * As usual, both a missing status and an `ok` status are
 * treated as `okay`.
 *
 * The children will be iterated over in the same order as they
 * appear in the final devicetree.
 *
 * @param node_id node identifier
 * @param fn macro to invoke
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_CHILD_STATUS_OKAY
 */
#define DT_FOREACH_CHILD_STATUS_OKAY_VARGS(node_id, fn, ...) \
	DT_CAT(node_id, _FOREACH_CHILD_STATUS_OKAY_VARGS)(fn, __VA_ARGS__)

/**
 * @brief Call @p fn on the child nodes with status `okay` with separator and
 * multiple arguments
 *
 * The macro @p fn takes multiple arguments. The first should be the node
 * identifier for the child node. The remaining are passed-in by the caller.
 *
 * As usual, both a missing status and an `ok` status are
 * treated as `okay`.
 *
 * @param node_id node identifier
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_CHILD_SEP_STATUS_OKAY
 */
#define DT_FOREACH_CHILD_STATUS_OKAY_SEP_VARGS(node_id, fn, sep, ...) \
	DT_CAT(node_id, _FOREACH_CHILD_STATUS_OKAY_SEP_VARGS)(fn, sep, __VA_ARGS__)

/**
 * @brief Invokes @p fn for each element in the value of property @p prop.
 *
 * The macro @p fn must take three parameters: fn(node_id, prop, idx).
 * @p node_id and @p prop are the same as what is passed to
 * DT_FOREACH_PROP_ELEM(), and @p idx is the current index into the array.
 * The @p idx values are integer literals starting from 0.
 *
 * The @p prop argument must refer to a property that can be passed to
 * DT_PROP_LEN().
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     n: node {
 *             my-ints = <1 2 3>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     #define TIMES_TWO(node_id, prop, idx) \
 *	       (2 * DT_PROP_BY_IDX(node_id, prop, idx)),
 *
 *     int array[] = {
 *             DT_FOREACH_PROP_ELEM(DT_NODELABEL(n), my_ints, TIMES_TWO)
 *     };
 * @endcode
 *
 * This expands to:
 *
 * @code{.c}
 *     int array[] = {
 *             (2 * 1), (2 * 2), (2 * 3),
 *     };
 * @endcode
 *
 * In general, this macro expands to:
 *
 *     fn(node_id, prop, 0) fn(node_id, prop, 1) [...] fn(node_id, prop, n-1)
 *
 * where `n` is the number of elements in @p prop, as it would be
 * returned by `DT_PROP_LEN(node_id, prop)`.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @see DT_PROP_LEN
 */
#define DT_FOREACH_PROP_ELEM(node_id, prop, fn)		\
	DT_CAT4(node_id, _P_, prop, _FOREACH_PROP_ELEM)(fn)

/**
 * @brief Invokes @p fn for each element in the value of property @p prop with
 *        separator.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     n: node {
 *             my-gpios = <&gpioa 0 GPIO_ACTICE_HIGH>,
 *                        <&gpiob 1 GPIO_ACTIVE_HIGH>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     struct gpio_dt_spec specs[] = {
 *             DT_FOREACH_PROP_ELEM_SEP(DT_NODELABEL(n), my_gpios,
 *                                      GPIO_DT_SPEC_GET_BY_IDX, (,))
 *     };
 * @endcode
 *
 * This expands as a first step to:
 *
 * @code{.c}
 *     struct gpio_dt_spec specs[] = {
 *             GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(n), my_gpios, 0),
 *             GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(n), my_gpios, 1)
 *     };
 * @endcode
 *
 * The @p prop parameter has the same restrictions as the same parameter
 * given to DT_FOREACH_PROP_ELEM().
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 *
 * @see DT_FOREACH_PROP_ELEM
 */
#define DT_FOREACH_PROP_ELEM_SEP(node_id, prop, fn, sep) \
	DT_CAT4(node_id, _P_, prop, _FOREACH_PROP_ELEM_SEP)(fn, sep)

/**
 * @brief Invokes @p fn for each element in the value of property @p prop with
 * multiple arguments.
 *
 * The macro @p fn must take multiple parameters:
 * `fn(node_id, prop, idx, ...)`. @p node_id and @p prop are the same as what
 * is passed to DT_FOREACH_PROP_ELEM(), and @p idx is the current index into
 * the array. The @p idx values are integer literals starting from 0. The
 * remaining arguments are passed-in by the caller.
 *
 * The @p prop parameter has the same restrictions as the same parameter
 * given to DT_FOREACH_PROP_ELEM().
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_PROP_ELEM
 */
#define DT_FOREACH_PROP_ELEM_VARGS(node_id, prop, fn, ...)		\
	DT_CAT4(node_id, _P_, prop, _FOREACH_PROP_ELEM_VARGS)(fn, __VA_ARGS__)

/**
 * @brief Invokes @p fn for each element in the value of property @p prop with
 * multiple arguments and a separator.
 *
 * The @p prop parameter has the same restrictions as the same parameter
 * given to DT_FOREACH_PROP_ELEM().
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 * @param ... variable number of arguments to pass to fn
 *
 * @see DT_FOREACH_PROP_ELEM_VARGS
 */
#define DT_FOREACH_PROP_ELEM_SEP_VARGS(node_id, prop, fn, sep, ...)	\
	DT_CAT4(node_id, _P_, prop, _FOREACH_PROP_ELEM_SEP_VARGS)(	\
		fn, sep, __VA_ARGS__)

/**
 * @brief Invokes @p fn for each status `okay` node of a compatible.
 *
 * This macro expands to:
 *
 *     fn(node_id_1) fn(node_id_2) ... fn(node_id_n)
 *
 * where each `node_id_<i>` is a node identifier for some node with
 * compatible @p compat and status `okay`. Whitespace is added between
 * expansions as shown above.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
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
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_FOREACH_STATUS_OKAY(foo, DT_NODE_PATH)
 * @endcode
 *
 * This expands to one of the following:
 *
 *     "/a" "/c"
 *     "/c" "/a"
 *
 * "One of the following" is because no guarantees are made about the
 * order that node identifiers are passed to @p fn in the expansion.
 *
 * (The `/c` string literal is present because a missing status
 * property is always treated as if the status were set to `okay`.)
 *
 * Note also that @p fn is responsible for adding commas, semicolons,
 * or other terminators as needed.
 *
 * @param compat lowercase-and-underscores devicetree compatible
 * @param fn Macro to call for each enabled node. Must accept a
 *           node_id as its only parameter.
 */
#define DT_FOREACH_STATUS_OKAY(compat, fn)				\
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(compat),			\
		    (UTIL_CAT(DT_FOREACH_OKAY_, compat)(fn)),		\
		    ())

/**
 * @brief Invokes @p fn for each status `okay` node of a compatible
 *        with multiple arguments.
 *
 * This is like DT_FOREACH_STATUS_OKAY() except you can also pass
 * additional arguments to @p fn.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
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
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     #define MY_FN(node_id, operator) DT_PROP(node_id, val) operator
 *     x = DT_FOREACH_STATUS_OKAY_VARGS(foo, MY_FN, +) 0;
 * @endcode
 *
 * This expands to one of the following:
 *
 * @code{.c}
 *     x = 3 + 4 + 0;
 *     x = 4 + 3 + 0;
 * @endcode
 *
 * i.e. it sets `x` to 7. As with DT_FOREACH_STATUS_OKAY(), there are no
 * guarantees about the order nodes appear in the expansion.
 *
 * @param compat lowercase-and-underscores devicetree compatible
 * @param fn Macro to call for each enabled node. Must accept a
 *           node_id as its only parameter.
 * @param ... Additional arguments to pass to @p fn
 */
#define DT_FOREACH_STATUS_OKAY_VARGS(compat, fn, ...)			\
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(compat),			\
		    (DT_CAT(DT_FOREACH_OKAY_VARGS_,			\
			      compat)(fn, __VA_ARGS__)),		\
		    ())

/**
 * @brief Call @p fn on all nodes with compatible `compat`
 *        and status `okay` with multiple arguments
 *
 *
 * @param compat lowercase-and-underscores devicetree compatible
 * @param fn Macro to call for each enabled node. Must accept a
 *           devicetree compatible and instance number.
 * @param ... Additional arguments to pass to @p fn
 *
 * @see DT_INST_FOREACH_STATUS_OKAY_VARGS
 */
#define DT_COMPAT_FOREACH_STATUS_OKAY_VARGS(compat, fn, ...)		\
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(compat),			\
		    (UTIL_CAT(DT_FOREACH_OKAY_INST_VARGS_,		\
			      compat)(fn, compat, __VA_ARGS__)),	\
		    ())


/**
 * @brief Invokes @p fn for each node label of a given node
 *
 * The order of the node labels in this macro's expansion matches
 * the order in the final devicetree, with duplicates removed.
 *
 * Node labels are passed to @p fn as tokens. Note that devicetree
 * node labels are always valid C tokens (see "6.2 Labels" in
 * Devicetree Specification v0.4 for details). The node labels are
 * passed as tokens to @p fn as-is, without any lowercasing or
 * conversion of special characters to underscores.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     foo: bar: FOO: node@deadbeef {};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     int foo = 1;
 *     int bar = 2;
 *     int FOO = 3;
 *
 *     #define FN(nodelabel) + nodelabel
 *     int sum = 0 DT_FOREACH_NODELABEL(DT_NODELABEL(foo), FN)
 * @endcode
 *
 * This expands to:
 *
 * @code{.c}
 *     int sum = 0 + 1 + 2 + 3;
 * @endcode
 *
 * @param node_id node identifier whose node labels to use
 * @param fn macro which will be passed each node label in order
 */
#define DT_FOREACH_NODELABEL(node_id, fn) DT_CAT(node_id, _FOREACH_NODELABEL)(fn)

/**
 * @brief Invokes @p fn for each node label of a given node with
 *        multiple arguments.
 *
 * This is like DT_FOREACH_NODELABEL() except you can also pass
 * additional arguments to @p fn.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     foo: bar: node@deadbeef {};
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     int foo = 0;
 *     int bar = 1;
 *
 *     #define VAR_PLUS(nodelabel, to_add) int nodelabel ## _added = nodelabel + to_add;
 *
 *     DT_FOREACH_NODELABEL_VARGS(DT_NODELABEL(foo), VAR_PLUS, 1)
 * @endcode
 *
 * This expands to:
 *
 * @code{.c}
 *     int foo = 0;
 *     int bar = 1;
 *     int foo_added = foo + 1;
 *     int bar_added = bar + 1;
 * @endcode
 *
 * @param node_id node identifier whose node labels to use
 * @param fn macro which will be passed each node label in order
 * @param ... additional arguments to pass to @p fn
 */
#define DT_FOREACH_NODELABEL_VARGS(node_id, fn, ...) \
	DT_CAT(node_id, _FOREACH_NODELABEL_VARGS)(fn, __VA_ARGS__)

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
 * @code{.c}
 *     DT_NODE_HAS_STATUS(DT_PATH(soc, i2c_12340000), okay)
 *     DT_NODE_HAS_STATUS(DT_PATH(soc, i2c_12340000), disabled)
 * @endcode
 *
 * Tests whether a node identifier refers to a node which:
 *
 * - exists in the devicetree, and
 * - has a status property matching the second argument
 *   (except that either a missing status or an `ok` status
 *   in the devicetree is treated as if it were `okay` instead)
 *
 * @param node_id a node identifier
 * @param status a status as one of the tokens okay or disabled, not a string
 * @return 1 if the node has the given status, 0 otherwise.
 */
#define DT_NODE_HAS_STATUS(node_id, status) \
	DT_NODE_HAS_STATUS_INTERNAL(node_id, status)

/**
 * @brief Does a node identifier refer to a node with a status `okay`?
 *
 * Example uses:
 *
 * @code{.c}
 *     DT_NODE_HAS_STATUS_OKAY(DT_PATH(soc, i2c_12340000))
 * @endcode
 *
 * Tests whether a node identifier refers to a node which:
 *
 * - exists in the devicetree, and
 * - has a status property as `okay`
 *
 * As usual, both a missing status and an `ok` status are treated as
 * `okay`.
 *
 * @param node_id a node identifier
 * @return 1 if the node has status as `okay`, 0 otherwise.
 */
#define DT_NODE_HAS_STATUS_OKAY(node_id) DT_NODE_HAS_STATUS(node_id, okay)

/**
 * @brief Does the devicetree have a status `okay` node with a compatible?
 *
 * Test for whether the devicetree has any nodes with status `okay`
 * and the given compatible. That is, this returns 1 if and only if
 * there is at least one @p node_id for which both of these
 * expressions return 1:
 *
 * @code{.c}
 *     DT_NODE_HAS_STATUS(node_id, okay)
 *     DT_NODE_HAS_COMPAT(node_id, compat)
 * @endcode
 *
 * As usual, both a missing status and an `ok` status are treated as
 * `okay`.
 *
 * @param compat lowercase-and-underscores compatible, without quotes
 * @return 1 if both of the above conditions are met, 0 otherwise
 */
#define DT_HAS_COMPAT_STATUS_OKAY(compat) \
	IS_ENABLED(DT_CAT(DT_COMPAT_HAS_OKAY_, compat))

/**
 * @brief Get the number of instances of a given compatible with
 *        status `okay`
 * @param compat lowercase-and-underscores compatible, without quotes
 * @return Number of instances with status `okay`
 */
#define DT_NUM_INST_STATUS_OKAY(compat)			\
	UTIL_AND(DT_HAS_COMPAT_STATUS_OKAY(compat),		\
		 UTIL_CAT(DT_N_INST, DT_DASH(compat, NUM_OKAY)))

/**
 * @brief Does a devicetree node match a compatible?
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     n: node {
 *             compatible = "vnd,specific-device", "generic-device";
 *     }
 * @endcode
 *
 * Example usages which evaluate to 1:
 *
 * @code{.c}
 *     DT_NODE_HAS_COMPAT(DT_NODELABEL(n), vnd_specific_device)
 *     DT_NODE_HAS_COMPAT(DT_NODELABEL(n), generic_device)
 * @endcode
 *
 * This macro only uses the value of the compatible property. Whether
 * or not a particular compatible has a matching binding has no effect
 * on its value, nor does the node's status.
 *
 * @param node_id node identifier
 * @param compat lowercase-and-underscores compatible, without quotes
 * @return 1 if the node's compatible property contains @p compat,
 *         0 otherwise.
 */
#define DT_NODE_HAS_COMPAT(node_id, compat) \
	IS_ENABLED(DT_CAT3(node_id, _COMPAT_MATCHES_, compat))

/**
 * @brief Does a devicetree node have a compatible and status?
 *
 * This is equivalent to:
 *
 * @code{.c}
 *     (DT_NODE_HAS_COMPAT(node_id, compat) &&
 *      DT_NODE_HAS_STATUS(node_id, status))
 * @endcode
 *
 * @param node_id node identifier
 * @param compat lowercase-and-underscores compatible, without quotes
 * @param status okay or disabled as a token, not a string
 */
#define DT_NODE_HAS_COMPAT_STATUS(node_id, compat, status) \
	UTIL_AND(DT_NODE_HAS_COMPAT(node_id, compat), DT_NODE_HAS_STATUS(node_id, status))

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
	IS_ENABLED(DT_CAT4(node_id, _P_, prop, _EXISTS))


/**
 * @brief Does a phandle array have a named cell specifier at an index?
 *
 * If this returns 1, then the phandle-array property @p pha has a cell
 * named @p cell at index @p idx, and therefore DT_PHA_BY_IDX(node_id,
 * pha, idx, cell) is valid. If it returns 0, it's an error to use
 * DT_PHA_BY_IDX() with the same arguments.
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param idx index to check within @p pha
 * @param cell lowercase-and-underscores cell name whose existence to check
 *             at index @p idx
 * @return 1 if the named cell exists in the specifier at index idx,
 *         0 otherwise.
 */
#define DT_PHA_HAS_CELL_AT_IDX(node_id, pha, idx, cell)             \
	IS_ENABLED(DT_CAT8(node_id, _P_, pha,			    \
			   _IDX_, idx, _VAL_, cell, _EXISTS))

/**
 * @brief Equivalent to DT_PHA_HAS_CELL_AT_IDX(node_id, pha, 0, cell)
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param cell lowercase-and-underscores cell name whose existence to check
 *             at index @p idx
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
 * used with DT_PROP() to get properties of the bus controller.
 *
 * It is an error to use this with nodes which do not have bus
 * controllers.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     i2c@deadbeef {
 *             status = "okay";
 *             clock-frequency = < 100000 >;
 *
 *             i2c_device: accelerometer@12 {
 *                     ...
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     DT_PROP(DT_BUS(DT_NODELABEL(i2c_device)), clock_frequency) // 100000
 * @endcode
 *
 * @param node_id node identifier
 * @return a node identifier for the node's bus controller
 */
#define DT_BUS(node_id) DT_CAT(node_id, _BUS)

/**
 * @brief Is a node on a bus of a given type?
 *
 * Example devicetree overlay:
 *
 * @code{.dts}
 *     &i2c0 {
 *            temp: temperature-sensor@76 {
 *                     compatible = "vnd,some-sensor";
 *                     reg = <0x76>;
 *            };
 *     };
 * @endcode
 *
 * Example usage, assuming `i2c0` is an I2C bus controller node, and
 * therefore `temp` is on an I2C bus:
 *
 * @code{.c}
 *     DT_ON_BUS(DT_NODELABEL(temp), i2c) // 1
 *     DT_ON_BUS(DT_NODELABEL(temp), spi) // 0
 * @endcode
 *
 * @param node_id node identifier
 * @param bus lowercase-and-underscores bus type as a C token (i.e.
 *            without quotes)
 * @return 1 if the node is on a bus of the given type,
 *         0 otherwise
 */
#define DT_ON_BUS(node_id, bus) IS_ENABLED(DT_CAT3(node_id, _BUS_, bus))

/**
 * @}
 */

/**
 * @defgroup devicetree-inst Instance-based devicetree APIs
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Node identifier for an instance of a `DT_DRV_COMPAT` compatible
 * @param inst instance number
 * @return a node identifier for the node with `DT_DRV_COMPAT` compatible and
 *         instance number @p inst
 */
#define DT_DRV_INST(inst) DT_INST(inst, DT_DRV_COMPAT)

/**
 * @brief Get a `DT_DRV_COMPAT` parent's node identifier
 * @param inst instance number
 * @return a node identifier for the instance's parent
 *
 * @see DT_PARENT
 */
#define DT_INST_PARENT(inst) DT_PARENT(DT_DRV_INST(inst))

/**
 * @brief Get a `DT_DRV_COMPAT` grandparent's node identifier
 * @param inst instance number
 * @return a node identifier for the instance's grandparent
 *
 * @see DT_GPARENT
 */
#define DT_INST_GPARENT(inst) DT_GPARENT(DT_DRV_INST(inst))

/**
 * @brief Get a node identifier for a child node of DT_DRV_INST(inst)
 *
 * @param inst instance number
 * @param child lowercase-and-underscores child node name
 * @return node identifier for the node with the name referred to by 'child'
 *
 * @see DT_CHILD
 */
#define DT_INST_CHILD(inst, child) \
	DT_CHILD(DT_DRV_INST(inst), child)

/**
 * @brief Get the number of child nodes of a given node
 *
 * This is equivalent to @see
 * <tt>DT_CHILD_NUM(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 * @return Number of child nodes
 */
#define DT_INST_CHILD_NUM(inst) DT_CHILD_NUM(DT_DRV_INST(inst))

/**
 * @brief Get the number of child nodes of a given node
 *
 * This is equivalent to @see
 * <tt>DT_CHILD_NUM_STATUS_OKAY(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 * @return Number of child nodes which status are okay
 */
#define DT_INST_CHILD_NUM_STATUS_OKAY(inst) \
	DT_CHILD_NUM_STATUS_OKAY(DT_DRV_INST(inst))

/**
 * @brief Get a string array of DT_DRV_INST(inst)'s node labels
 *
 * Equivalent to DT_NODELABEL_STRING_ARRAY(DT_DRV_INST(inst)).
 *
 * @param inst instance number
 * @return an array initializer for an array of the instance's node labels as strings
 */
#define DT_INST_NODELABEL_STRING_ARRAY(inst) DT_NODELABEL_STRING_ARRAY(DT_DRV_INST(inst))

/**
 * @brief Get the number of node labels by instance number
 *
 * Equivalent to DT_NUM_NODELABELS(DT_DRV_INST(inst)).
 *
 * @param inst instance number
 * @return the number of node labels that the node with that instance number has
 */
#define DT_INST_NUM_NODELABELS(inst) DT_NUM_NODELABELS(DT_DRV_INST(inst))

/**
 * @brief Call @p fn on all child nodes of DT_DRV_INST(inst).
 *
 * The macro @p fn should take one argument, which is the node
 * identifier for the child node.
 *
 * The children will be iterated over in the same order as they
 * appear in the final devicetree.
 *
 * @param inst instance number
 * @param fn macro to invoke on each child node identifier
 *
 * @see DT_FOREACH_CHILD
 */
#define DT_INST_FOREACH_CHILD(inst, fn) \
	DT_FOREACH_CHILD(DT_DRV_INST(inst), fn)

/**
 * @brief Call @p fn on all child nodes of DT_DRV_INST(inst) with a separator
 *
 * The macro @p fn should take one argument, which is the node
 * identifier for the child node.
 *
 * @param inst instance number
 * @param fn macro to invoke on each child node identifier
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 *
 * @see DT_FOREACH_CHILD_SEP
 */
#define DT_INST_FOREACH_CHILD_SEP(inst, fn, sep) \
	DT_FOREACH_CHILD_SEP(DT_DRV_INST(inst), fn, sep)

/**
 * @brief Call @p fn on all child nodes of DT_DRV_INST(inst).
 *
 * The macro @p fn takes multiple arguments. The first should be the node
 * identifier for the child node. The remaining are passed-in by the caller.
 *
 * The children will be iterated over in the same order as they
 * appear in the final devicetree.
 *
 * @param inst instance number
 * @param fn macro to invoke on each child node identifier
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_CHILD
 */
#define DT_INST_FOREACH_CHILD_VARGS(inst, fn, ...) \
	DT_FOREACH_CHILD_VARGS(DT_DRV_INST(inst), fn, __VA_ARGS__)

/**
 * @brief Call @p fn on all child nodes of DT_DRV_INST(inst) with separator.
 *
 * The macro @p fn takes multiple arguments. The first should be the node
 * identifier for the child node. The remaining are passed-in by the caller.
 *
 * @param inst instance number
 * @param fn macro to invoke on each child node identifier
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_CHILD_SEP_VARGS
 */
#define DT_INST_FOREACH_CHILD_SEP_VARGS(inst, fn, sep, ...) \
	DT_FOREACH_CHILD_SEP_VARGS(DT_DRV_INST(inst), fn, sep, __VA_ARGS__)

/**
 * @brief Call @p fn on all child nodes of DT_DRV_INST(inst) with status `okay`.
 *
 * The macro @p fn should take one argument, which is the node
 * identifier for the child node.
 *
 * @param inst instance number
 * @param fn macro to invoke on each child node identifier
 *
 * @see DT_FOREACH_CHILD_STATUS_OKAY
 */
#define DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, fn) \
	DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(inst), fn)

/**
 * @brief Call @p fn on all child nodes of DT_DRV_INST(inst) with status `okay`
 * and with separator.
 *
 * The macro @p fn should take one argument, which is the node
 * identifier for the child node.
 *
 * @param inst instance number
 * @param fn macro to invoke on each child node identifier
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 *
 * @see DT_FOREACH_CHILD_STATUS_OKAY_SEP
 */
#define DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(inst, fn, sep) \
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_DRV_INST(inst), fn, sep)

/**
 * @brief Call @p fn on all child nodes of DT_DRV_INST(inst) with status `okay`
 * and multiple arguments.
 *
 * The macro @p fn takes multiple arguments. The first should be the node
 * identifier for the child node. The remaining are passed-in by the caller.
 *
 * @param inst instance number
 * @param fn macro to invoke on each child node identifier
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_CHILD_STATUS_OKAY_VARGS
 */
#define DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(inst, fn, ...) \
	DT_FOREACH_CHILD_STATUS_OKAY_VARGS(DT_DRV_INST(inst), fn, __VA_ARGS__)

/**
 * @brief Call @p fn on all child nodes of DT_DRV_INST(inst) with status `okay`
 * and with separator and multiple arguments.
 *
 * The macro @p fn takes multiple arguments. The first should be the node
 * identifier for the child node. The remaining are passed-in by the caller.
 *
 * @param inst instance number
 * @param fn macro to invoke on each child node identifier
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_CHILD_STATUS_OKAY_SEP_VARGS
 */
#define DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP_VARGS(inst, fn, sep, ...) \
	DT_FOREACH_CHILD_STATUS_OKAY_SEP_VARGS(DT_DRV_INST(inst), fn, sep, __VA_ARGS__)

/**
 * @brief Get a `DT_DRV_COMPAT` property array value's index into its enumeration values
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return zero-based index of the property's value in its enum: list
 */
#define DT_INST_ENUM_IDX_BY_IDX(inst, prop, idx) \
	DT_ENUM_IDX_BY_IDX(DT_DRV_INST(inst), prop, idx)

/**
 * @brief Get a `DT_DRV_COMPAT` value's index into its enumeration values
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return zero-based index of the property's value in its enum: list
 */
#define DT_INST_ENUM_IDX(inst, prop) \
	DT_ENUM_IDX(DT_DRV_INST(inst), prop)

/**
 * @brief Like DT_INST_ENUM_IDX_BY_IDX(), but with a fallback to a default enum index
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @param default_idx_value a fallback index value to expand to
 * @return zero-based index of the property's value in its enum if present,
 *         default_idx_value otherwise
 */
#define DT_INST_ENUM_IDX_BY_IDX_OR(inst, prop, idx, default_idx_value) \
	DT_ENUM_IDX_BY_IDX_OR(DT_DRV_INST(inst), prop, idx, default_idx_value)

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
 * @brief Does a `DT_DRV_COMPAT` enumeration property have a given value by index?
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @param value lowercase-and-underscores enumeration value
 * @return zero-based index of the property's value in its enum
 */
#define DT_INST_ENUM_HAS_VALUE_BY_IDX(inst, prop, idx, value) \
	DT_ENUM_HAS_VALUE_BY_IDX(DT_DRV_INST(inst), prop, idx, value)

/**
 * @brief Does a `DT_DRV_COMPAT` enumeration property have a given value?
 *
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param value lowercase-and-underscores enumeration value
 * @return 1 if the node property has the value @a value, 0 otherwise.
 */
#define DT_INST_ENUM_HAS_VALUE(inst, prop, value) \
	DT_ENUM_HAS_VALUE(DT_DRV_INST(inst), prop, value)

/**
 * @brief Get a `DT_DRV_COMPAT` instance property
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return a representation of the property's value
 */
#define DT_INST_PROP(inst, prop) DT_PROP(DT_DRV_INST(inst), prop)

/**
 * @brief Get a `DT_DRV_COMPAT` property length
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return logical length of the property
 */
#define DT_INST_PROP_LEN(inst, prop) DT_PROP_LEN(DT_DRV_INST(inst), prop)

/**
 * @brief Is index @p idx valid for an array type property
 *        on a `DT_DRV_COMPAT` instance?
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param idx index to check
 * @return 1 if @p idx is a valid index into the given property,
 *         0 otherwise.
 */
#define DT_INST_PROP_HAS_IDX(inst, prop, idx) \
	DT_PROP_HAS_IDX(DT_DRV_INST(inst), prop, idx)

/**
 * @brief Is name @p name available in a `foo-names` property?
 * @param inst instance number
 * @param prop a lowercase-and-underscores `prop-names` type property
 * @param name a lowercase-and-underscores name to check
 * @return An expression which evaluates to 1 if @p name is an available
 *         name into the given property, and 0 otherwise.
 */
#define DT_INST_PROP_HAS_NAME(inst, prop, name) \
	DT_PROP_HAS_NAME(DT_DRV_INST(inst), prop, name)

/**
 * @brief Get a `DT_DRV_COMPAT` element value in an array property
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return a representation of the idx-th element of the property
 */
#define DT_INST_PROP_BY_IDX(inst, prop, idx) \
	DT_PROP_BY_IDX(DT_DRV_INST(inst), prop, idx)

/**
 * @brief Like DT_INST_PROP(), but with a fallback to @p default_value
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return DT_INST_PROP(inst, prop) or @p default_value
 */
#define DT_INST_PROP_OR(inst, prop, default_value) \
	DT_PROP_OR(DT_DRV_INST(inst), prop, default_value)

/**
 * @brief Like DT_INST_PROP_LEN(), but with a fallback to @p default_value
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return DT_INST_PROP_LEN(inst, prop) or @p default_value
 */
#define DT_INST_PROP_LEN_OR(inst, prop, default_value) \
	DT_PROP_LEN_OR(DT_DRV_INST(inst), prop, default_value)

/**
 * @brief Get a `DT_DRV_COMPAT` instance's string property's value as a
 *        token.
 *
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return the value of @p prop as a token, i.e. without any quotes
 *         and with special characters converted to underscores
 */
#define DT_INST_STRING_TOKEN(inst, prop) \
	DT_STRING_TOKEN(DT_DRV_INST(inst), prop)

/**
 * @brief Like DT_INST_STRING_TOKEN(), but uppercased.
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return the value of @p prop as an uppercased token, i.e. without
 *         any quotes and with special characters converted to underscores
 */
#define DT_INST_STRING_UPPER_TOKEN(inst, prop) \
	DT_STRING_UPPER_TOKEN(DT_DRV_INST(inst), prop)

/**
 * @brief Get a `DT_DRV_COMPAT` instance's string property's value as
 *        an unquoted sequence of tokens.
 *
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return the value of @p prop as a sequence of tokens, with no quotes
 */
#define DT_INST_STRING_UNQUOTED(inst, prop) \
	DT_STRING_UNQUOTED(DT_DRV_INST(inst), prop)

/**
 * @brief Get an element out of string-array property as a token.
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return the element in @p prop at index @p idx as a token
 */
#define DT_INST_STRING_TOKEN_BY_IDX(inst, prop, idx) \
	DT_STRING_TOKEN_BY_IDX(DT_DRV_INST(inst), prop, idx)

/**
 * @brief Like DT_INST_STRING_TOKEN_BY_IDX(), but uppercased.
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return the element in @p prop at index @p idx as an uppercased token
 */
#define DT_INST_STRING_UPPER_TOKEN_BY_IDX(inst, prop, idx) \
	DT_STRING_UPPER_TOKEN_BY_IDX(DT_DRV_INST(inst), prop, idx)

/**
 * @brief Get an element out of string-array property as an unquoted sequence of tokens.
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return the value of @p prop at index @p idx as a sequence of tokens, with no quotes
 */
#define DT_INST_STRING_UNQUOTED_BY_IDX(inst, prop, idx) \
	DT_STRING_UNQUOTED_BY_IDX(DT_DRV_INST(inst), prop, idx)

/**
 * @brief Get a `DT_DRV_COMPAT` instance's property value from a phandle's node
 * @param inst instance number
 * @param ph lowercase-and-underscores property of @p inst
 *           with type `phandle`
 * @param prop lowercase-and-underscores property of the phandle's node
 * @return the value of @p prop as described in the DT_PROP() documentation
 */
#define DT_INST_PROP_BY_PHANDLE(inst, ph, prop) \
	DT_INST_PROP_BY_PHANDLE_IDX(inst, ph, 0, prop)

/**
 * @brief Get a `DT_DRV_COMPAT` instance's property value from a phandle in a
 * property.
 * @param inst instance number
 * @param phs lowercase-and-underscores property with type `phandle`,
 *            `phandles`, or `phandle-array`
 * @param idx logical index into "phs", which must be zero if "phs"
 *            has type `phandle`
 * @param prop lowercase-and-underscores property of the phandle's node
 * @return the value of @p prop as described in the DT_PROP() documentation
 */
#define DT_INST_PROP_BY_PHANDLE_IDX(inst, phs, idx, prop) \
	DT_PROP_BY_PHANDLE_IDX(DT_DRV_INST(inst), phs, idx, prop)

/**
 * @brief Get a `DT_DRV_COMPAT` instance's phandle-array specifier value at an index
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param idx logical index into the property @p pha
 * @param cell binding's cell name within the specifier at index @p idx
 * @return the value of the cell inside the specifier at index @p idx
 */
#define DT_INST_PHA_BY_IDX(inst, pha, idx, cell) \
	DT_PHA_BY_IDX(DT_DRV_INST(inst), pha, idx, cell)

/**
 * @brief Like DT_INST_PHA_BY_IDX(), but with a fallback to default_value
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param idx logical index into the property @p pha
 * @param cell binding's cell name within the specifier at index @p idx
 * @param default_value a fallback value to expand to
 * @return DT_INST_PHA_BY_IDX(inst, pha, idx, cell) or default_value
 */
#define DT_INST_PHA_BY_IDX_OR(inst, pha, idx, cell, default_value) \
	DT_PHA_BY_IDX_OR(DT_DRV_INST(inst), pha, idx, cell, default_value)

/**
 * @brief Get a `DT_DRV_COMPAT` instance's phandle-array specifier value
 * Equivalent to DT_INST_PHA_BY_IDX(inst, pha, 0, cell)
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param cell binding's cell name for the specifier at @p pha index 0
 * @return the cell value
 */
#define DT_INST_PHA(inst, pha, cell) DT_INST_PHA_BY_IDX(inst, pha, 0, cell)

/**
 * @brief Like DT_INST_PHA(), but with a fallback to default_value
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param cell binding's cell name for the specifier at @p pha index 0
 * @param default_value a fallback value to expand to
 * @return DT_INST_PHA(inst, pha, cell) or default_value
 */
#define DT_INST_PHA_OR(inst, pha, cell, default_value) \
	DT_INST_PHA_BY_IDX_OR(inst, pha, 0, cell, default_value)

/**
 * @brief Get a `DT_DRV_COMPAT` instance's value within a phandle-array
 * specifier by name
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param name lowercase-and-underscores name of a specifier in @p pha
 * @param cell binding's cell name for the named specifier
 * @return the cell value
 */
#define DT_INST_PHA_BY_NAME(inst, pha, name, cell) \
	DT_PHA_BY_NAME(DT_DRV_INST(inst), pha, name, cell)

/**
 * @brief Like DT_INST_PHA_BY_NAME(), but with a fallback to default_value
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param name lowercase-and-underscores name of a specifier in @p pha
 * @param cell binding's cell name for the named specifier
 * @param default_value a fallback value to expand to
 * @return DT_INST_PHA_BY_NAME(inst, pha, name, cell) or default_value
 */
#define DT_INST_PHA_BY_NAME_OR(inst, pha, name, cell, default_value) \
	DT_PHA_BY_NAME_OR(DT_DRV_INST(inst), pha, name, cell, default_value)

/**
 * @brief Get a `DT_DRV_COMPAT` instance's phandle node identifier from a
 * phandle array by name
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param name lowercase-and-underscores name of an element in @p pha
 * @return node identifier for the phandle at the element named "name"
 */
#define DT_INST_PHANDLE_BY_NAME(inst, pha, name) \
	DT_PHANDLE_BY_NAME(DT_DRV_INST(inst), pha, name) \

/**
 * @brief Get a `DT_DRV_COMPAT` instance's node identifier for a phandle in
 * a property.
 * @param inst instance number
 * @param prop lowercase-and-underscores property name in @p inst
 *             with type `phandle`, `phandles` or `phandle-array`
 * @param idx index into @p prop
 * @return a node identifier for the phandle at index @p idx in @p prop
 */
#define DT_INST_PHANDLE_BY_IDX(inst, prop, idx) \
	DT_PHANDLE_BY_IDX(DT_DRV_INST(inst), prop, idx)

/**
 * @brief Get a `DT_DRV_COMPAT` instance's node identifier for a phandle
 * property's value
 * @param inst instance number
 * @param prop lowercase-and-underscores property of @p inst
 *             with type `phandle`
 * @return a node identifier for the node pointed to by "ph"
 */
#define DT_INST_PHANDLE(inst, prop) DT_INST_PHANDLE_BY_IDX(inst, prop, 0)

/**
 * @brief is @p idx a valid register block index on a `DT_DRV_COMPAT` instance?
 * @param inst instance number
 * @param idx index to check
 * @return 1 if @p idx is a valid register block index,
 *         0 otherwise.
 */
#define DT_INST_REG_HAS_IDX(inst, idx) DT_REG_HAS_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief is @p name a valid register block name on a `DT_DRV_COMPAT` instance?
 * @param inst instance number
 * @param name name to check
 * @return 1 if @p name is a valid register block name,
 *         0 otherwise.
 */
#define DT_INST_REG_HAS_NAME(inst, name) DT_REG_HAS_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get a `DT_DRV_COMPAT` instance's idx-th register block's raw address
 * @param inst instance number
 * @param idx index of the register whose address to return
 * @return address of the instance's idx-th register block
 */
#define DT_INST_REG_ADDR_BY_IDX_RAW(inst, idx) DT_REG_ADDR_BY_IDX_RAW(DT_DRV_INST(inst), idx)

/**
 * @brief Get a `DT_DRV_COMPAT` instance's idx-th register block's address
 * @param inst instance number
 * @param idx index of the register whose address to return
 * @return address of the instance's idx-th register block
 */
#define DT_INST_REG_ADDR_BY_IDX(inst, idx) DT_REG_ADDR_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get a `DT_DRV_COMPAT` instance's idx-th register block's size
 * @param inst instance number
 * @param idx index of the register whose size to return
 * @return size of the instance's idx-th register block
 */
#define DT_INST_REG_SIZE_BY_IDX(inst, idx) \
	DT_REG_SIZE_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get a `DT_DRV_COMPAT`'s register block address by  name
 * @param inst instance number
 * @param name lowercase-and-underscores register specifier name
 * @return address of the register block with the given @p name
 */
#define DT_INST_REG_ADDR_BY_NAME(inst, name) \
	DT_REG_ADDR_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Like DT_INST_REG_ADDR_BY_NAME(), but with a fallback to @p default_value
 * @param inst instance number
 * @param name lowercase-and-underscores register specifier name
 * @param default_value a fallback value to expand to
 * @return address of the register block specified by name if present,
 *         @p default_value otherwise
 */
#define DT_INST_REG_ADDR_BY_NAME_OR(inst, name, default_value) \
	DT_REG_ADDR_BY_NAME_OR(DT_DRV_INST(inst), name, default_value)

/**
 * @brief 64-bit version of DT_INST_REG_ADDR_BY_NAME()
 *
 * This macro version adds the appropriate suffix for 64-bit unsigned
 * integer literals.
 * Note that this macro is equivalent to DT_INST_REG_ADDR_BY_NAME() in
 * linker/ASM context.
 *
 * @param inst instance number
 * @param name lowercase-and-underscores register specifier name
 * @return address of the register block with the given @p name
 */
#define DT_INST_REG_ADDR_BY_NAME_U64(inst, name) \
	DT_REG_ADDR_BY_NAME_U64(DT_DRV_INST(inst), name)

/**
 * @brief Get a `DT_DRV_COMPAT`'s register block size by name
 * @param inst instance number
 * @param name lowercase-and-underscores register specifier name
 * @return size of the register block with the given @p name
 */
#define DT_INST_REG_SIZE_BY_NAME(inst, name) \
	DT_REG_SIZE_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Like DT_INST_REG_SIZE_BY_NAME(), but with a fallback to @p default_value
 * @param inst instance number
 * @param name lowercase-and-underscores register specifier name
 * @param default_value a fallback value to expand to
 * @return size of the register block specified by name if present,
 *         @p default_value otherwise
 */
#define DT_INST_REG_SIZE_BY_NAME_OR(inst, name, default_value) \
	DT_REG_SIZE_BY_NAME_OR(DT_DRV_INST(inst), name, default_value)

/**
 * @brief Get a `DT_DRV_COMPAT`'s (only) register block raw address
 * @param inst instance number
 * @return instance's register block address
 */
#define DT_INST_REG_ADDR_RAW(inst) DT_INST_REG_ADDR_BY_IDX_RAW(inst, 0)

/**
 * @brief Get a `DT_DRV_COMPAT`'s (only) register block address
 * @param inst instance number
 * @return instance's register block address
 */
#define DT_INST_REG_ADDR(inst) DT_INST_REG_ADDR_BY_IDX(inst, 0)

/**
 * @brief 64-bit version of DT_INST_REG_ADDR()
 *
 * This macro version adds the appropriate suffix for 64-bit unsigned
 * integer literals.
 * Note that this macro is equivalent to DT_INST_REG_ADDR() in
 * linker/ASM context.
 *
 * @param inst instance number
 * @return instance's register block address
 */
#define DT_INST_REG_ADDR_U64(inst) DT_REG_ADDR_U64(DT_DRV_INST(inst))

/**
 * @brief Get a `DT_DRV_COMPAT`'s (only) register block size
 * @param inst instance number
 * @return instance's register block size
 */
#define DT_INST_REG_SIZE(inst) DT_INST_REG_SIZE_BY_IDX(inst, 0)

/**
 * @brief Get a `DT_DRV_COMPAT`'s number of interrupts
 *
 * @param inst instance number
 * @return number of interrupts
 */
#define DT_INST_NUM_IRQS(inst) DT_NUM_IRQS(DT_DRV_INST(inst))

/**
 * @brief Get a `DT_DRV_COMPAT` interrupt level
 *
 * @param inst instance number
 * @return interrupt level
 */
#define DT_INST_IRQ_LEVEL(inst) DT_IRQ_LEVEL(DT_DRV_INST(inst))

/**
 * @brief Get a `DT_DRV_COMPAT` interrupt specifier value at an index
 * @param inst instance number
 * @param idx logical index into the interrupt specifier array
 * @param cell cell name specifier
 * @return the named value at the specifier given by the index
 */
#define DT_INST_IRQ_BY_IDX(inst, idx, cell) \
	DT_IRQ_BY_IDX(DT_DRV_INST(inst), idx, cell)

/**
 * @brief Get a `DT_DRV_COMPAT` interrupt specifier's interrupt controller by index
 * @param inst instance number
 * @param idx interrupt specifier's index
 * @return node_id of interrupt specifier's interrupt controller
 */
#define DT_INST_IRQ_INTC_BY_IDX(inst, idx) \
	DT_IRQ_INTC_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get a `DT_DRV_COMPAT` interrupt specifier's interrupt controller by name
 * @param inst instance number
 * @param name interrupt specifier's name
 * @return node_id of interrupt specifier's interrupt controller
 */
#define DT_INST_IRQ_INTC_BY_NAME(inst, name) \
	DT_IRQ_INTC_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Get a `DT_DRV_COMPAT` interrupt specifier's interrupt controller
 * @note Equivalent to DT_INST_IRQ_INTC_BY_IDX(node_id, 0)
 * @param inst instance number
 * @return node_id of interrupt specifier's interrupt controller
 * @see DT_INST_IRQ_INTC_BY_IDX()
 */
#define DT_INST_IRQ_INTC(inst) \
	DT_INST_IRQ_INTC_BY_IDX(inst, 0)

/**
 * @brief Get a `DT_DRV_COMPAT` interrupt specifier value by name
 * @param inst instance number
 * @param name lowercase-and-underscores interrupt specifier name
 * @param cell cell name specifier
 * @return the named value at the specifier given by the index
 */
#define DT_INST_IRQ_BY_NAME(inst, name, cell) \
	DT_IRQ_BY_NAME(DT_DRV_INST(inst), name, cell)

/**
 * @brief Get a `DT_DRV_COMPAT` interrupt specifier's value
 * @param inst instance number
 * @param cell cell name specifier
 * @return the named value at that index
 */
#define DT_INST_IRQ(inst, cell) DT_INST_IRQ_BY_IDX(inst, 0, cell)

/**
 * @brief Get a `DT_DRV_COMPAT`'s (only) irq number
 * @param inst instance number
 * @return the interrupt number for the node's only interrupt
 */
#define DT_INST_IRQN(inst) DT_IRQN(DT_DRV_INST(inst))

/**
 * @brief Get a `DT_DRV_COMPAT`'s irq number at index
 * @param inst instance number
 * @param idx logical index into the interrupt specifier array
 * @return the interrupt number for the node's idx-th interrupt
 */
#define DT_INST_IRQN_BY_IDX(inst, idx) DT_IRQN_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get a `DT_DRV_COMPAT`'s bus node identifier
 * @param inst instance number
 * @return node identifier for the instance's bus node
 */
#define DT_INST_BUS(inst) DT_BUS(DT_DRV_INST(inst))

/**
 * @brief Test if a `DT_DRV_COMPAT`'s bus type is a given type
 * @param inst instance number
 * @param bus a binding's bus type as a C token, lowercased and without quotes
 * @return 1 if the given instance is on a bus of the given type,
 *         0 otherwise
 */
#define DT_INST_ON_BUS(inst, bus) DT_ON_BUS(DT_DRV_INST(inst), bus)

/**
 * @brief Like DT_INST_STRING_TOKEN(), but with a fallback to @p default_value
 * @param inst instance number
 * @param name lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return if @p prop exists, its value as a token, i.e. without any quotes and
 *         with special characters converted to underscores. Otherwise
 *         @p default_value
 */
#define DT_INST_STRING_TOKEN_OR(inst, name, default_value) \
	DT_STRING_TOKEN_OR(DT_DRV_INST(inst), name, default_value)

/**
 * @brief Like DT_INST_STRING_UPPER_TOKEN(), but with a fallback to
 *        @p default_value
 * @param inst instance number
 * @param name lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return the property's value as an uppercased token, or @p default_value
 */
#define DT_INST_STRING_UPPER_TOKEN_OR(inst, name, default_value) \
	DT_STRING_UPPER_TOKEN_OR(DT_DRV_INST(inst), name, default_value)

/**
 * @brief Like DT_INST_STRING_UNQUOTED(), but with a fallback to
 *        @p default_value
 * @param inst instance number
 * @param name lowercase-and-underscores property name
 * @param default_value a fallback value to expand to
 * @return the property's value as a sequence of tokens, with no quotes, or @p default_value
 */
#define DT_INST_STRING_UNQUOTED_OR(inst, name, default_value) \
	DT_STRING_UNQUOTED_OR(DT_DRV_INST(inst), name, default_value)

/*
 * @brief Test if any enabled node with the given compatible is on
 *        the given bus type
 *
 * This is like DT_ANY_INST_ON_BUS_STATUS_OKAY(), except it can also
 * be useful for handling multiple compatibles in single source file.
 *
 * Example devicetree overlay:
 *
 * @code{.dts}
 *     &i2c0 {
 *            temp: temperature-sensor@76 {
 *                     compatible = "vnd,some-sensor";
 *                     reg = <0x76>;
 *            };
 *     };
 * @endcode
 *
 * Example usage, assuming `i2c0` is an I2C bus controller node, and
 * therefore `temp` is on an I2C bus:
 *
 * @code{.c}
 *     DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(vnd_some_sensor, i2c) // 1
 * @endcode
 *
 * @param compat lowercase-and-underscores compatible, without quotes
 * @param bus a binding's bus type as a C token, lowercased and without quotes
 * @return 1 if any enabled node with that compatible is on that bus type,
 *         0 otherwise
 */
#define DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(compat, bus) \
	IS_ENABLED(DT_CAT4(DT_COMPAT_, compat, _BUS_, bus))

/**
 * @brief Test if any `DT_DRV_COMPAT` node is on a bus of a given type
 *        and has status okay
 *
 * This is a special-purpose macro which can be useful when writing
 * drivers for devices which can appear on multiple buses. One example
 * is a sensor device which may be wired on an I2C or SPI bus.
 *
 * Example devicetree overlay:
 *
 * @code{.dts}
 *     &i2c0 {
 *            temp: temperature-sensor@76 {
 *                     compatible = "vnd,some-sensor";
 *                     reg = <0x76>;
 *            };
 *     };
 * @endcode
 *
 * Example usage, assuming `i2c0` is an I2C bus controller node, and
 * therefore `temp` is on an I2C bus:
 *
 * @code{.c}
 *     #define DT_DRV_COMPAT vnd_some_sensor
 *
 *     DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) // 1
 * @endcode
 *
 * @param bus a binding's bus type as a C token, lowercased and without quotes
 * @return 1 if any enabled node with that compatible is on that bus type,
 *         0 otherwise
 */
#define DT_ANY_INST_ON_BUS_STATUS_OKAY(bus) \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(DT_DRV_COMPAT, bus)

/**
 * @brief Check if any `DT_DRV_COMPAT` node with status `okay` has a given
 *        property.
 *
 * @param prop lowercase-and-underscores property name
 *
 * Example devicetree overlay:
 *
 * @code{.dts}
 *     &i2c0 {
 *         sensor0: sensor@0 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <0>;
 *             foo = <1>;
 *             bar = <2>;
 *         };
 *
 *         sensor1: sensor@1 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <1>;
 *             foo = <2>;
 *         };
 *
 *         sensor2: sensor@2 {
 *             compatible = "vnd,some-sensor";
 *             status = "disabled";
 *             reg = <2>;
 *             baz = <1>;
 *         };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     #define DT_DRV_COMPAT vnd_some_sensor
 *
 *     DT_ANY_INST_HAS_PROP_STATUS_OKAY(foo) // 1
 *     DT_ANY_INST_HAS_PROP_STATUS_OKAY(bar) // 1
 *     DT_ANY_INST_HAS_PROP_STATUS_OKAY(baz) // 0
 * @endcode
 */
#define DT_ANY_INST_HAS_PROP_STATUS_OKAY(prop) \
	COND_CODE_1(IS_EMPTY(DT_ANY_INST_HAS_PROP_STATUS_OKAY_(prop)), (0), (1))

/**
 * @brief Check if any device node with status `okay` has a given
 *        property.
 *
 * @param compat lowercase-and-underscores devicetree compatible
 * @param prop lowercase-and-underscores property name
 *
 * Example devicetree overlay:
 *
 * @code{.dts}
 *     &i2c0 {
 *         sensor0: sensor@0 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <0>;
 *             foo = <1>;
 *             bar = <2>;
 *         };
 *
 *         sensor1: sensor@1 {
 *             compatible = "vnd,some-sensor";
 *             status = "okay";
 *             reg = <1>;
 *             foo = <2>;
 *         };
 *
 *         sensor2: sensor@2 {
 *             compatible = "vnd,some-sensor";
 *             status = "disabled";
 *             reg = <2>;
 *             baz = <1>;
 *         };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *
 *     DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(vnd_some_sensor, foo) // 1
 *     DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(vnd_some_sensor, bar) // 1
 *     DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(vnd_some_sensor, baz) // 0
 * @endcode
 */
#define DT_ANY_COMPAT_HAS_PROP_STATUS_OKAY(compat, prop) \
	(DT_COMPAT_FOREACH_STATUS_OKAY_VARGS(compat, DT_COMPAT_NODE_HAS_PROP_AND_OR, prop) 0)

/**
 * @brief Call @p fn on all nodes with compatible `DT_DRV_COMPAT`
 *        and status `okay`
 *
 * This macro calls `fn(inst)` on each `inst` number that refers to a
 * node with status `okay`. Whitespace is added between invocations.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     a {
 *             compatible = "vnd,device";
 *             status = "okay";
 *             foobar = "DEV_A";
 *     };
 *
 *     b {
 *             compatible = "vnd,device";
 *             status = "okay";
 *             foobar = "DEV_B";
 *     };
 *
 *     c {
 *             compatible = "vnd,device";
 *             status = "disabled";
 *             foobar = "DEV_C";
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     #define DT_DRV_COMPAT vnd_device
 *     #define MY_FN(inst) DT_INST_PROP(inst, foobar),
 *
 *     DT_INST_FOREACH_STATUS_OKAY(MY_FN)
 * @endcode
 *
 * This expands to:
 *
 * @code{.c}
 *     MY_FN(0) MY_FN(1)
 * @endcode
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
 * Note that @p fn is responsible for adding commas, semicolons, or
 * other separators or terminators.
 *
 * Device drivers should use this macro whenever possible to
 * instantiate a struct device for each enabled node in the devicetree
 * of the driver's compatible `DT_DRV_COMPAT`.
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
 * @brief Call @p fn on all nodes with compatible `DT_DRV_COMPAT`
 *        and status `okay` with multiple arguments
 *
 *
 * @param fn Macro to call for each enabled node. Must accept an
 *           instance number.
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_INST_FOREACH_STATUS_OKAY
 * @see DT_COMPAT_FOREACH_STATUS_OKAY_VARGS
 */
#define DT_INST_FOREACH_STATUS_OKAY_VARGS(fn, ...) \
	COND_CODE_1(DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT),   \
		    (UTIL_CAT(DT_FOREACH_OKAY_INST_VARGS_,      \
			      DT_DRV_COMPAT)(fn, __VA_ARGS__)), \
		    ())

/**
 * @brief Call @p fn on all node labels for a given `DT_DRV_COMPAT` instance
 *
 * Equivalent to DT_FOREACH_NODELABEL(DT_DRV_INST(inst), fn).
 *
 * @param inst instance number
 * @param fn macro which will be passed each node label for the node
 *           with that instance number
 */
#define DT_INST_FOREACH_NODELABEL(inst, fn) \
	DT_FOREACH_NODELABEL(DT_DRV_INST(inst), fn)

/**
 * @brief Call @p fn on all node labels for a given `DT_DRV_COMPAT` instance
 *        with multiple arguments
 *
 * Equivalent to DT_FOREACH_NODELABEL_VARGS(DT_DRV_INST(inst), fn, ...).
 *
 * @param inst instance number
 * @param fn macro which will be passed each node label for the node
 *           with that instance number
 * @param ... additional arguments to pass to @p fn
 */
#define DT_INST_FOREACH_NODELABEL_VARGS(inst, fn, ...) \
	DT_FOREACH_NODELABEL_VARGS(DT_DRV_INST(inst), fn, __VA_ARGS__)

/**
 * @brief Invokes @p fn for each element of property @p prop for
 *        a `DT_DRV_COMPAT` instance.
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
 * @brief Invokes @p fn for each element of property @p prop for
 *        a `DT_DRV_COMPAT` instance with a separator.
 *
 * Equivalent to DT_FOREACH_PROP_ELEM_SEP(DT_DRV_INST(inst), prop, fn, sep).
 *
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 */
#define DT_INST_FOREACH_PROP_ELEM_SEP(inst, prop, fn, sep) \
	DT_FOREACH_PROP_ELEM_SEP(DT_DRV_INST(inst), prop, fn, sep)

/**
 * @brief Invokes @p fn for each element of property @p prop for
 *        a `DT_DRV_COMPAT` instance with multiple arguments.
 *
 * Equivalent to
 *      DT_FOREACH_PROP_ELEM_VARGS(DT_DRV_INST(inst), prop, fn, __VA_ARGS__)
 *
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_INST_FOREACH_PROP_ELEM
 */
#define DT_INST_FOREACH_PROP_ELEM_VARGS(inst, prop, fn, ...) \
	DT_FOREACH_PROP_ELEM_VARGS(DT_DRV_INST(inst), prop, fn, __VA_ARGS__)

/**
 * @brief Invokes @p fn for each element of property @p prop for
 *        a `DT_DRV_COMPAT` instance with multiple arguments and a separator.
 *
 * Equivalent to
 *      DT_FOREACH_PROP_ELEM_SEP_VARGS(DT_DRV_INST(inst), prop, fn, sep,
 *                                     __VA_ARGS__)
 *
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 * @param ... variable number of arguments to pass to fn
 *
 * @see DT_INST_FOREACH_PROP_ELEM
 */
#define DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(inst, prop, fn, sep, ...)		\
	DT_FOREACH_PROP_ELEM_SEP_VARGS(DT_DRV_INST(inst), prop, fn, sep,	\
				       __VA_ARGS__)

/**
 * @brief Does a DT_DRV_COMPAT instance have a property?
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return 1 if the instance has the property, 0 otherwise.
 */
#define DT_INST_NODE_HAS_PROP(inst, prop) \
	DT_NODE_HAS_PROP(DT_DRV_INST(inst), prop)

/**
 * @brief Does a DT_DRV_COMPAT instance have the compatible?
 * @param inst instance number
 * @param compat lowercase-and-underscores compatible, without quotes
 * @return 1 if the instance matches the compatible, 0 otherwise.
 */
#define DT_INST_NODE_HAS_COMPAT(inst, compat) \
	DT_NODE_HAS_COMPAT(DT_DRV_INST(inst), compat)

/**
 * @brief Does a phandle array have a named cell specifier at an index
 *        for a `DT_DRV_COMPAT` instance?
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param idx index to check
 * @param cell named cell value whose existence to check
 * @return 1 if the named @p cell exists in the specifier at index @p idx,
 *         0 otherwise.
 */
#define DT_INST_PHA_HAS_CELL_AT_IDX(inst, pha, idx, cell) \
	DT_PHA_HAS_CELL_AT_IDX(DT_DRV_INST(inst), pha, idx, cell)

/**
 * @brief Does a phandle array have a named cell specifier at index 0
 *        for a `DT_DRV_COMPAT` instance?
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type `phandle-array`
 * @param cell named cell value whose existence to check
 * @return 1 if the named @p cell exists in the specifier at index 0,
 *         0 otherwise.
 */
#define DT_INST_PHA_HAS_CELL(inst, pha, cell) \
	DT_INST_PHA_HAS_CELL_AT_IDX(inst, pha, 0, cell)

/**
 * @brief is index valid for interrupt property on a `DT_DRV_COMPAT` instance?
 * @param inst instance number
 * @param idx logical index into the interrupt specifier array
 * @return 1 if the @p idx is valid for the interrupt property
 *         0 otherwise.
 */
#define DT_INST_IRQ_HAS_IDX(inst, idx) DT_IRQ_HAS_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Does a `DT_DRV_COMPAT` instance have an interrupt named cell specifier?
 * @param inst instance number
 * @param idx index to check
 * @param cell named cell value whose existence to check
 * @return 1 if the named @p cell exists in the interrupt specifier at index
 *         @p idx 0 otherwise.
 */
#define DT_INST_IRQ_HAS_CELL_AT_IDX(inst, idx, cell) \
	DT_IRQ_HAS_CELL_AT_IDX(DT_DRV_INST(inst), idx, cell)

/**
 * @brief Does a `DT_DRV_COMPAT` instance have an interrupt value?
 * @param inst instance number
 * @param cell named cell value whose existence to check
 * @return 1 if the named @p cell exists in the interrupt specifier at index 0
 *         0 otherwise.
 */
#define DT_INST_IRQ_HAS_CELL(inst, cell) \
	DT_INST_IRQ_HAS_CELL_AT_IDX(inst, 0, cell)

/**
 * @brief Does a `DT_DRV_COMPAT` instance have an interrupt value?
 * @param inst instance number
 * @param name lowercase-and-underscores interrupt specifier name
 * @return 1 if @p name is a valid named specifier
 */
#define DT_INST_IRQ_HAS_NAME(inst, name) \
	DT_IRQ_HAS_NAME(DT_DRV_INST(inst), name)

/**
 * @}
 */

/** @cond INTERNAL_HIDDEN */

/** @brief Helper for DT_ANY_INST_HAS_PROP_STATUS_OKAY_
 *
 * This macro generates token "1," for instance of a device,
 * identified by index @p idx, if instance has property @p prop.
 *
 * @param idx instance number
 * @param prop property to check for
 *
 * @return Macro evaluates to `1,` if instance has the property,
 * otherwise it evaluates to literal nothing.
 */
#define DT_ANY_INST_HAS_PROP_STATUS_OKAY__(idx, prop)	\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, prop), (1,), ())
/** @brief Helper for DT_ANY_INST_HAS_PROP_STATUS_OKAY
 *
 * This macro uses DT_ANY_INST_HAS_PROP_STATUS_OKAY_ with
 * DT_INST_FOREACH_STATUS_OKAY_VARG to generate comma separated list of 1,
 * where each 1 on the list represents instance that has a property
 * @p prop; the list may be empty, and the upper bound on number of
 * list elements is number of device instances.
 *
 * @param prop property to check
 *
 * @return Evaluates to list of 1s (e.g: 1,1,1,) or nothing.
 */
#define DT_ANY_INST_HAS_PROP_STATUS_OKAY_(prop)	\
	DT_INST_FOREACH_STATUS_OKAY_VARGS(DT_ANY_INST_HAS_PROP_STATUS_OKAY__, prop)

#define DT_PATH_INTERNAL(...) \
	UTIL_CAT(DT_ROOT, MACRO_MAP_CAT(DT_S_PREFIX, __VA_ARGS__))
/** @brief DT_PATH_INTERNAL() helper: prepends _S_ to a node name
 * We don't want to expand 'name' recursively before expansion
 * in this case. The MACRO_MAP_CAT above is giving us the exact
 * tokens it wants prefixed with _S_.
 */
#define DT_S_PREFIX(name) _S_##name

/**
 * @brief Concatenation helper, 2 arguments
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
/** @brief Concatenation helper, 3 arguments */
#define DT_CAT3(a1, a2, a3) a1 ## a2 ## a3
/** @brief Concatenation helper, 4 arguments */
#define DT_CAT4(a1, a2, a3, a4) a1 ## a2 ## a3 ## a4
/** @brief Internal concatenation helper, 5 arguments */
#define DT_CAT5(a1, a2, a3, a4, a5) a1 ## a2 ## a3 ## a4 ## a5
/** @brief Concatenation helper, 6 arguments */
#define DT_CAT6(a1, a2, a3, a4, a5, a6) a1 ## a2 ## a3 ## a4 ## a5 ## a6
/** @brief concatenation helper, 7 arguments */
#define DT_CAT7(a1, a2, a3, a4, a5, a6, a7) \
	a1 ## a2 ## a3 ## a4 ## a5 ## a6 ## a7
/** @brief concatenation helper, 8 arguments */
#define DT_CAT8(a1, a2, a3, a4, a5, a6, a7, a8) \
	a1 ## a2 ## a3 ## a4 ## a5 ## a6 ## a7 ## a8
/*
 * If you need to define a bigger DT_CATN(), do so here. Don't leave
 * any "holes" of undefined macros, please.
 */

/** @brief Helper for node identifier macros to expand args */
#define DT_DASH(...) MACRO_MAP_CAT(DT_DASH_PREFIX, __VA_ARGS__)
/** @brief Helper for DT_DASH(): prepends _ to a name */
#define DT_DASH_PREFIX(name) _##name
/** @brief Helper for DT_NODE_HAS_STATUS */
#define DT_NODE_HAS_STATUS_INTERNAL(node_id, status) \
	IS_ENABLED(DT_CAT3(node_id, _STATUS_, status))

/** @brief Helper macro to OR multiple has property checks in a loop macro
 *         (for the specified device)
 */
#define DT_COMPAT_NODE_HAS_PROP_AND_OR(inst, compat, prop) \
	DT_NODE_HAS_PROP(DT_INST(inst, compat), prop) ||

/**
 * @def DT_U32_C
 * @brief Macro to add 32bit unsigned postfix to the devicetree address constants
 */
#if defined(_LINKER) || defined(_ASMLANGUAGE)
#define DT_U32_C(_v) (_v)
#else
#define DT_U32_C(_v) UINT32_C(_v)
#endif

/**
 * @def DT_U64_C
 * @brief Macro to add ULL postfix to the devicetree address constants
 */
#if defined(_LINKER) || defined(_ASMLANGUAGE)
#define DT_U64_C(_v) (_v)
#else
#define DT_U64_C(_v) UINT64_C(_v)
#endif

/* Helpers for DT_NODELABEL_STRING_ARRAY. We define our own stringify
 * in order to avoid adding a dependency on toolchain.h..
 */
#define DT_NODELABEL_STRING_ARRAY_ENTRY_INTERNAL(nodelabel) DT_STRINGIFY_INTERNAL(nodelabel),
#define DT_STRINGIFY_INTERNAL(arg) DT_STRINGIFY_INTERNAL_HELPER(arg)
#define DT_STRINGIFY_INTERNAL_HELPER(arg) #arg

/** @endcond */

/* have these last so they have access to all previously defined macros */
#include <zephyr/devicetree/io-channels.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/devicetree/gpio.h>
#include <zephyr/devicetree/spi.h>
#include <zephyr/devicetree/dma.h>
#include <zephyr/devicetree/pwms.h>
#include <zephyr/devicetree/fixed-partitions.h>
#include <zephyr/devicetree/ordinals.h>
#include <zephyr/devicetree/pinctrl.h>
#include <zephyr/devicetree/can.h>
#include <zephyr/devicetree/reset.h>
#include <zephyr/devicetree/mbox.h>

#endif /* DEVICETREE_H */
