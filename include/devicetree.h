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
#include <devicetree_legacy_unfixed.h>
#include <devicetree_fixups.h>

#include <sys/util.h>

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
 * _EXISTS: property is defined
 * _IDX_<i>: logical index into property
 * _IDX_<i>_PH: phandle array's phandle by index (or phandle, phandles)
 * _IDX_<i>_VAL_<val>: phandle array's specifier value by index
 * _IDX_<i>_VAL_<val>_EXISTS: cell value exists, by index
 * _LEN: property logical length
 * _NAME_<name>_PH: phandle array's phandle by name
 * _NAME_<name>_VAL_<val>: phandle array's property specifier by name
 * _NAME_<name>_VAL_<val>_EXISTS: cell value exists, by name
 */

/**
 * @defgroup devicetree-generic-id Node identifiers
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Node identifier for the root node in the devicetree
 */
#define DT_ROOT DT_N

/**
 * @brief Get a node identifier for a devicetree path
 *
 * Example devicetree fragment:
 *
 *     / {
 *             soc {
 *                     my-serial: serial@40002000 {
 *                             status = "okay";
 *                             current-speed = <115200>;
 *                             ...
 *                     };
 *             };
 *     };
 *
 * Example usage with @ref DT_PROP() to get current-speed:
 *
 *     DT_PROP(DT_PATH(soc, serial_40002000), current_speed) // 115200
 *
 * The arguments to this macro are the names of non-root nodes in the
 * tree required to reach the desired node, starting from the root.
 * Non-alphanumeric characters in each name must be converted to
 * underscores to form valid C tokens, and letters must be lowercased.
 *
 * That is:
 *
 * - a first argument corresponds to a child node of the root ("soc" above)
 * - a second argument corresponds to a child of the first argument
 *   ("serial_40002000" above, from the node name "serial@40002000"
 *   after changing "@" to "_")
 * - and so on for deeper nodes until the desired path is given
 *
 * @param ... lowercase-and-underscores node names along the node's path,
 *            with each name given as a separate argument
 * @return node identifier for the node with that path
 */
#define DT_PATH(...) DT_PATH_INTERNAL(__VA_ARGS__)

/**
 * @brief Get a node identifier for a node label
 *
 * Example devicetree fragment:
 *
 *     my-serial: serial@40002000 {
 *             label = "UART_0";
 *             status = "okay";
 *             current-speed = <115200>;
 *             ...
 *     };
 *
 * The only node label in this example is "my-serial".
 * The string "UART_0" is *not* a node label; it's the value of a
 * property named "label".
 *
 * Example usage to get current-speed:
 *
 *     DT_PROP(DT_NODELABEL(my_serial), current_speed) // 115200
 *
 * Convert non-alphanumeric characters in the label to underscores as
 * shown, and lowercase all letters.
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
 * Example usage to get cache-level:
 *
 *     DT_PROP(DT_NODELABEL(l2_0), cache_level) // 2
 *
 * Notice how "L2_0" in the devicetree is lowercased to "l2_0"
 * for this macro's argument.
 *
 * @param label lowercase-and-underscores node label name
 * @return node identifier for the node with that label
 */
#define DT_NODELABEL(label) DT_CAT(DT_N_NODELABEL_, label)

/**
 * @brief Get a node identifier for an alias
 *
 * Example devicetree fragment:
 *
 *     aliases {
 *             my-serial = &serial0;
 *     };
 *
 *     serial0: serial@40002000 {
 *             status = "okay";
 *             current-speed = <115200>;
 *             ...
 *     };
 *
 * Example usage to get current-speed:
 *
 *     DT_PROP(DT_ALIAS(my_serial), current_speed) // 115200
 *
 * Convert non-alphanumeric characters in the alias to underscores as
 * shown, and lowercase all letters.
 *
 * @param alias lowercase-and-underscores alias name.
 * @return node identifier for the node with that alias
 */
#define DT_ALIAS(alias) DT_CAT(DT_N_ALIAS_, alias)

/**
 * @brief Get a node identifier for an instance of a compatible
 *
 * Instance numbers are just indexes among enabled nodes with the same
 * compatible. This complicates their use outside of device drivers.
 * The **only guarantees** are:
 *
 * - instance numbers start at 0,
 * - are contiguous, and
 * - exactly one is assigned for each enabled node with a matching
 *   compatible
 *
 * Instance numbers **in no way reflect** any numbering scheme that
 * might exist in SoC documentation, node labels or unit addresses, or
 * properties of the /aliases node. There **is no guarantee** that the
 * same node will have the same instance number between builds, even
 * if you are building the same application again in the same build
 * directory.
 *
 * Example devicetree fragment:
 *
 *     serial@40002000 {
 *             compatible = "vnd,soc-serial";
 *             status = "okay";
 *             current-speed = <115200>;
 *             ...
 *     };
 *
 * Example usage to get current-speed, **assuming that** this node is
 * instance number zero of the compatible "vnd,soc-serial":
 *
 *     DT_PROP(DT_INST(0, vnd_soc_serial), current_speed) // 115200
 *
 * @param inst instance number
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
 * The following generate equivalent node identifiers:
 *
 *     DT_NODELABEL(parent)
 *     DT_PARENT(DT_NODELABEL(child))
 *
 * @param node_id node identifier
 */
#define DT_PARENT(node_id) UTIL_CAT(node_id, _PARENT)

/**
 * @brief Get a node identifier for a child node
 *
 * Example devicetree fragment:
 *
 *     / {
 *             soc-label: soc {
 *                     my-serial: serial@4 {
 *                             status = "okay";
 *                             current-speed = <115200>;
 *                             ...
 *                     };
 *             };
 *     };
 *
 * Example usage with @ref DT_PROP() to get the status of the child node
 * "serial@4" of the node referenced by node label "soc-label":
 *
 *     DT_PROP(DT_CHILD(DT_NODELABEL(soc_label), serial_4), status) // "okay"
 *
 * @param node_id node identifier
 * @param child lowercase-and-underscores child node name
 * @return node identifier for the node with the name referred to by 'child'
 */
#define DT_CHILD(node_id, child) UTIL_CAT(node_id, DT_S_PREFIX(child))

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
 * - phandle: a node identifier
 *
 * For other properties, behavior is undefined.
 *
 * For examples, see @ref DT_PATH(), @ref DT_ALIAS(), @ref DT_NODELABEL(),
 * and @ref DT_INST().
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @return a representation of the property's value
 */
#define DT_PROP(node_id, prop) DT_CAT(node_id, _P_##prop)

/**
 * @brief Get a property's logical length
 *
 * Here, "length" is a number of elements, which may not
 * be a size in bytes.
 *
 * For properties whose binding has type array, string-array, or
 * uint8-array, this expands to the number of elements in the array.
 *
 * For properties of type phandles or phandle-array, it expands to the
 * number of phandles or phandle+specifiers respectively.
 *
 * These properties are handled as special cases:
 *
 * - reg property: use DT_NUM_REGS(node_id) instead
 * - interrupts property: use DT_NUM_IRQS(node_id) instead
 *
 * It is an error to use this macro with the above properties.
 *
 * For other properties, behavior is undefined.
 *
 * @param node_id node identifier
 * @param prop a lowercase-and-underscores property with a logical length
 * @return the property's length
 */
#define DT_PROP_LEN(node_id, prop) DT_PROP(node_id, prop##_LEN)

/**
 * @brief Is index "idx" valid for an array type property?
 *
 * If this returns 1, then DT_PROP_BY_IDX(node_id, prop, idx) or
 * DT_PHA_BY_IDX(node_id, pha, idx, cell) are valid at index "idx".
 * If it returns 0, it is an error to use those macros with that index.
 *
 * These properties are handled as special cases:
 *
 * - reg property: use DT_REG_HAS_IDX(node_id, idx) instead
 * - interrupts property: use DT_IRQ_HAS_IDX(node_id, idx) instead
 *
 * It is an error to use this macro with the above properties.
 *
 * @param node_id node identifier
 * @param prop a lowercase-and-underscores property with a logical length
 * @param idx index to check
 * @return 1 if "idx" is a valid index into the given property,
 *         0 otherwise.
 */
#define DT_PROP_HAS_IDX(node_id, prop, idx) \
	((idx) < DT_PROP_LEN(node_id, prop))

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

/*
 * phandle properties
 *
 * These are special-cased to manage the impedance mismatch between
 * phandles, which are just u32_t node properties that only make sense
 * within the tree itself, and C values.
 */

/**
 * @brief Get a property value from a phandle's node
 *
 * This is a shorthand for DT_PROP_BY_PHANDLE_IDX(node_id, ph, 0, prop).
 * It helps readability when "ph" has type "phandle".
 *
 * @param node_id node identifier
 * @param ph lowercase-and-underscores property of "node_id"
 *           with type "phandle"
 * @param prop lowercase-and-underscores property of the phandle's node
 * @return the value of "prop" as described in the DT_PROP() documentation
 */
#define DT_PROP_BY_PHANDLE(node_id, ph, prop) \
	DT_PROP_BY_PHANDLE_IDX(node_id, ph, 0, prop)

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
 * @return the value of "prop" as described in the DT_PROP() documentation
 */
#define DT_PROP_BY_PHANDLE_IDX(node_id, phs, idx, prop) \
	DT_PROP(DT_PHANDLE_BY_IDX(node_id, phs, idx), prop)

/**
 * @brief Get a phandle-array specifier value at an index
 *
 * It might help to read the argument order as being similar to
 * "node->phandle[index].cell". That is, the cell value is in the
 * "pha" property of "node_id".
 *
 * Example devicetree fragment:
 *
 *     gpio0: gpio@12340000 {
 *             #gpio-cells = < 2 >;
 *     };
 *
 *     led: led_0 {
 *             gpios = < &gpio0 17 0x1 >;
 *     };
 *
 * Bindings fragment for the gpio0 node:
 *
 *     gpio-cells:
 *       - pin
 *       - flags
 *
 * Example usage:
 *
 *     #define LED DT_NODELABEL(led)
 *
 *     DT_PHA_BY_IDX(LED, gpios, pin, 0)   // 17
 *     DT_PHA_BY_IDX(LED, gpios, flags, 0) // 0x1
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param idx logical index into the property "pha"
 * @param cell binding's cell name within the specifier at index "idx"
 * @return the value of the cell inside the specifier at index "idx"
 */
#define DT_PHA_BY_IDX(node_id, pha, idx, cell) \
	DT_PROP(node_id, pha##_IDX_##idx##_VAL_##cell)

/**
 * @brief Equivalent to DT_PHA_BY_IDX(node_id, pha, 0, cell)
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param cell binding's cell name for the specifier at "pha" index 0
 * @return the cell value
 */
#define DT_PHA(node_id, pha, cell) DT_PHA_BY_IDX(node_id, pha, 0, cell)

/**
 * @brief Get a value within a phandle-array specifier by name
 *
 * This is like DT_PHA_BY_IDX(), except it treats "pha" as a structure
 * where each specifier has a name.
 *
 * It might help to read the argument order as being similar to
 * "node->phandle_struct.name.cell". That is, the cell value is in the
 * "pha" property of "node_id", treated as a data structure with named
 * components.
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
 * @param cell binding's cell name for the named specifier
 * @return the cell value
 */
#define DT_PHA_BY_NAME(node_id, pha, name, cell) \
	DT_PROP(node_id, pha##_NAME_##name##_VAL_##cell)

/**
 * @brief Get a phandle's node identifier from a phandle array by name
 *
 * It might help to read the argument order as being similar to
 * "node->phandle_struct.name.phandle". That is, the phandle array is
 * treated as a structure with named components. The return value is
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
 * Example usage:
 *
 *     #define NODE DT_NODELABEL(n)
 *
 *     DT_LABEL(DT_PHANDLE_BY_NAME(NODE, io_channels, sensor))  // "ADC_1"
 *     DT_LABEL(DT_PHANDLE_BY_NAME(NODE, io_channels, bandgap)) // "ADC_2"
 *
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param name lowercase-and-underscores name of an element in "pha"
 * @return node identifier for the phandle at the element named "name"
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
 * @return a node identifier for the phandle at index "idx" in "prop"
 */
#define DT_PHANDLE_BY_IDX(node_id, prop, idx) \
	DT_PROP(node_id, prop##_IDX_##idx##_PH)

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

/*
 * reg property
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

/*
 * interrupts property
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
 *     Example usage                    Value
 *     -------------                    -----
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
 * This is only valid to call if DT_HAS_CHOSEN(prop) is 1.
 * @param prop lowercase-and-underscores property name for
 *             the /chosen node
 * @return a node identifier for the chosen node property
 */
#define DT_CHOSEN(prop) DT_CAT(DT_CHOSEN_, prop)

/**
 * @}
 */

/**
 * @defgroup devicetree-generic-exist Existence checks
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Does a node identifier refer to a usable node?
 *
 * Example uses:
 *
 *     DT_HAS_NODE(DT_PATH(soc, i2c@12340000))
 *     DT_HAS_NODE(DT_ALIAS(an_alias_name))
 *
 * Tests whether a node identifier refers to a node which:
 *
 * - exists in the devicetree, and
 * - is enabled (has status property "okay"), and
 * - has a matching binding
 *
 * @param node_id a node identifier
 * @return 1 if the node identifier refers to a usable node,
 *         0 otherwise.
 */
#define DT_HAS_NODE(node_id) IS_ENABLED(DT_CAT(node_id, _EXISTS))

/**
 * @brief Does the devicetree have any usable nodes with a compatible?
 *
 * Test for whether the devicetree has any usable nodes (as determined by
 * @ref DT_HAS_NODE()) with a given compatible, i.e. if there is at least one
 * "x" for which "DT_HAS_NODE(DT_INST(x, compat))" is 1.
 *
 * @param compat lowercase-and-underscores version of a compatible
 * @return 0 if no nodes of the compatible are available for use,
 *         1 if at least one is enabled and has a matching binding
 */
#define DT_HAS_COMPAT(compat) DT_HAS_NODE(DT_INST(0, compat))

/**
 * @brief Get the number of enabled instances for a given compatible
 * @param compat lowercase-and-underscores version of a compatible
 * @return Number of enabled instances
 */
#define DT_NUM_INST(compat)					\
	UTIL_AND(DT_HAS_COMPAT(compat),				\
		 UTIL_CAT(DT_N_INST, DT_DASH(compat, NUM)))

/**
 * @brief Test if the devicetree has a /chosen node
 * @param prop lowercase-and-underscores devicetree property
 * @return 1 if the chosen property exists and refers to a node,
 *         0 otherwise
 */
#define DT_HAS_CHOSEN(prop) IS_ENABLED(DT_CHOSEN_##prop##_EXISTS)

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
 * on its value.
 *
 * @param node_id node identifier
 * @param compat lowercase-and-underscorse compatible value
 * @return 1 if the node's compatible property contains compat,
 *         0 otherwise.
 */
#define DT_NODE_HAS_COMPAT(node_id, compat) \
	IS_ENABLED(DT_CAT(node_id, _COMPAT_MATCHES_##compat))

/**
 * @brief Does a devicetree node have a property?
 *
 * Tests whether a devicetree node has a property defined. This
 * tests whether the property is part of the node at all, not whether
 * a boolean property is true or not.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @return 1 if the node has the property, 0 otherwise.
 */
#define DT_NODE_HAS_PROP(node_id, prop) \
	IS_ENABLED(DT_CAT(node_id, _P_##prop##_EXISTS))


/**
 * @brief Does a phandle array have a named cell specifier at an index?
 * If this returns 1, then the cell argument to
 * DT_PHA_BY_IDX(node_id, pha, idx, cell) is valid.
 * If it returns 0, it is an error.
 * @param node_id node identifier
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param idx index to check
 * @param cell named cell value whose existence to check
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
 * @param cell named cell value whose existence to check
 * @return 1 if the named ceell exists in the specifier at index 0,
 *         0 otherwise.
 */
#define DT_PHA_HAS_CELL(node_id, pha, cell) \
	DT_PHA_HAS_CELL_AT_IDX(node_id, pha, 0, cell)

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
 * @brief Test if a node's bus type is a given type
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
 * @param bus a binding's bus type as a C token, lowercased and without quotes
 * @return 1 if the node is on a bus of the given type,
 *         0 otherwise
 */
#define DT_ON_BUS(node_id, bus) IS_ENABLED(DT_CAT(node_id, _BUS_##bus))

/**
 * @brief Test if any node of a compatible is on a bus of a given type
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
 *     DT_COMPAT_ON_BUS(vnd_some_sensor, i2c) // 1
 *
 * @param compat lowercase-and-underscores version of a compatible
 * @param bus a binding's bus type as a C token, lowercased and without quotes
 * @return 1 if any enabled node with that compatible is on that bus type,
 *         0 otherwise
 */
#define DT_COMPAT_ON_BUS(compat, bus) \
	IS_ENABLED(UTIL_CAT(DT_CAT(DT_COMPAT_, compat), _BUS_##bus))

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
 * @brief Get a DT_DRV_COMPAT instance property
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return a representation of the property's value
 */
#define DT_INST_PROP(inst, prop) DT_PROP(DT_DRV_INST(inst), prop)

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
 * @brief Get a DT_DRV_COMPAT property length
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return logical length of the property
 */
#define DT_INST_PROP_LEN(inst, prop) DT_PROP_LEN(DT_DRV_INST(inst), prop)

/**
 * @brief Get a DT_DRV_COMPAT instance's "label" property
 * @param inst instance number
 * @return instance's label property value
 */
#define DT_INST_LABEL(inst) DT_INST_PROP(inst, label)

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
 * @brief Get a DT_DRV_COMPAT instance's phandle-array specifier value
 * Equivalent to DT_INST_PHA_BY_IDX(inst, pha, 0, cell)
 * @param inst instance number
 * @param pha lowercase-and-underscores property with type "phandle-array"
 * @param cell binding's cell name for the specifier at "pha" index 0
 * @return the cell value
 */
#define DT_INST_PHA(inst, pha, cell) DT_INST_PHA_BY_IDX(inst, pha, 0, cell)

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
 * Equivalent to DT_INST_REG_ADDR_BY_IDX(inst, 0).
 * @param inst instance number
 * @return instance's register block address
 */
#define DT_INST_REG_ADDR(inst) DT_INST_REG_ADDR_BY_IDX(inst, 0)

/**
 * @brief Get a DT_DRV_COMPAT's (only) register block size
 * Equivalent to DT_INST_REG_SIZE_BY_IDX(inst, 0).
 * @param inst instance number
 * @return instance's register block size
 */
#define DT_INST_REG_SIZE(inst) DT_INST_REG_SIZE_BY_IDX(inst, 0)

/**
 * @brief Get a DT_DRV_COMPAT interrupt specifier value at an index
 *        (see @ref DT_IRQ_BY_IDX)
 * @param inst instance number
 * @param idx logical index into the interrupt specifier array
 * @param cell cell name specifier
 * @return the named value at the specifier given by the index
 */
#define DT_INST_IRQ_BY_IDX(inst, idx, cell) \
	DT_IRQ_BY_IDX(DT_DRV_INST(inst), idx, cell)

/**
 * @brief Get a DT_DRV_COMPAT interrupt specifier value by name
 *        (see @ref DT_IRQ_BY_NAME)
 * @param inst instance number
 * @param name lowercase-and-underscores interrupt specifier name
 * @param cell cell name specifier
 * @return the named value at the specifier given by the index
 */
#define DT_INST_IRQ_BY_NAME(inst, name, cell) \
	DT_IRQ_BY_NAME(DT_DRV_INST(inst), name, cell)

/**
 * @brief Get a DT_DRV_COMAPT interrupt specifier's value
 * Equivalent to DT_INST_IRQ_BY_IDX(inst, 0, cell).
 * @param inst instance number
 * @param cell cell name specifier
 * @return the named value at that index
 */
#define DT_INST_IRQ(inst, cell) DT_INST_IRQ_BY_IDX(inst, 0, cell)

/**
 * @brief Get a DT_DRV_COMPAT's (only) irq number
 * Equivalent to DT_INST_IRQ(inst, irq).
 * @param inst instance number
 * @return the interrupt number for the node's only interrupt
 */
#define DT_INST_IRQN(inst) DT_INST_IRQ(inst, irq)

/**
 * @brief Get a DT_DRV_COMPAT's bus node's label property
 * Equivalent to DT_BUS_LABEL(DT_DRV_INST(inst))
 * @param inst instance number
 * @return the label property of the instance's bus controller
 */
#define DT_INST_BUS_LABEL(inst) DT_BUS_LABEL(DT_DRV_INST(inst))

/**
 * @brief Does the devicetree have a particular instance number?
 *
 * This is equivalent to DT_HAS_NODE(DT_DRV_INST(inst)).
 * @param inst instance number
 * @return 1 if the devicetree has that numbered instance,
 *         0 otherwise.
 */
#define DT_HAS_DRV_INST(inst) DT_HAS_NODE(DT_DRV_INST(inst))

/**
 * @brief Test if a DT_DRV_COMPAT's bus type is a given type
 * This is equivalent to DT_ON_BUS(DT_DRV_INST(inst), bus).
 * @param inst instance number
 * @param bus a binding's bus type as a C token, lowercased and without quotes
 * @return 1 if the given instance is on a bus of the given type,
 *         0 otherwise
 */
#define DT_INST_ON_BUS(inst, bus) DT_ON_BUS(DT_DRV_INST(inst), bus)

/**
 * @brief Test if any node with compatible DT_DRV_COMPAT is on a bus
 *
 * This is equivalent to DT_COMPAT_ON_BUS(DT_DRV_COMPAT, bus).
 * @param bus a binding's bus type as a C token, lowercased and without quotes
 */
#define DT_ANY_INST_ON_BUS(bus) DT_COMPAT_ON_BUS(DT_DRV_COMPAT, bus)

/**
 * @def DT_INST_FOREACH
 *
 * @brief Call specified macro for all nodes with compatible DT_DRV_COMPAT
 *
 * @details This macro will scan for all DT_INST_ device nodes for that driver.
 * The macro then calls the supplied macro with the instance number.
 * This macro can be used for example to call the init macro of a driver
 * for each device specified in the device tree.
 *
 * @param inst_expr Macro or function that is called for each device node.
 * Has to accept instance_number as only parameter.
 */
#define DT_INST_FOREACH(inst_expr) \
	COND_CODE_1(DT_HAS_COMPAT(DT_DRV_COMPAT), \
		    (UTIL_CAT(DT_FOREACH_INST_, DT_DRV_COMPAT)(inst_expr)), ())

/**
 * @brief Does a DT_DRV_COMPAT instance have a property?
 * Equivalent to DT_NODE_HAS_PROP(DT_DRV_INST(inst), prop)
 * @param inst instance number
 * @param prop lowercase-and-underscores property name
 * @return 1 if the instance has the property, 0 otherwise.
 */
#define DT_INST_NODE_HAS_PROP(inst, prop) \
	DT_NODE_HAS_PROP(DT_DRV_INST(inst), prop)

/**
 * @brief Does a phandle array have a named cell specifier at an index?
 *        for a DT_DRV_COMPAT instance
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
 * Equivalent to DT_IRQ_HAS_IDX(DT_DRV_INST(inst), idx).
 * @param inst instance number
 * @param idx logical index into the interrupt specifier array
 * @return 1 if the idx is valid for the interrupt property
 *         0 otherwise.
 */
#define DT_INST_IRQ_HAS_IDX(inst, idx) DT_IRQ_HAS_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Does a DT_DRV_COMPAT instance have an interrupt named cell specifier?
 * Equivalent to DT_IRQ_HAS_CELL_AT_IDX(DT_DRV_INST(inst), idx, cell).
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
 * Equivalent to DT_INST_IRQ_HAS_IDX(DT_DRV_INST(inst), 0, cell).
 * @param inst instance number
 * @param cell named cell value whose existence to check
 * @return 1 if the named cell exists in the interrupt specifier at index 0
 *         0 otherwise.
 */
#define DT_INST_IRQ_HAS_CELL(inst, cell) \
	DT_INST_IRQ_HAS_CELL_AT_IDX(inst, 0, cell)

/**
 * @brief Does a DT_DRV_COMPAT instance have an interrupt value?
 * Equivalent to DT_INST_IRQ_HAS_NAME(DT_DRV_INST(inst), name).
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
/** @internal concatenation helper, sometimes used to force expansion */
#define DT_CAT(node_id, prop_suffix) node_id##prop_suffix
/** @internal helper for node identifier macros to expand args */
#define DT_DASH(...) MACRO_MAP_CAT(DT_DASH_PREFIX, __VA_ARGS__)
/** @internal helper for DT_DASH(): prepends _ to a name */
#define DT_DASH_PREFIX(name) _##name

/* have these last so the have access to all previously defined macros */
#include <devicetree/adc.h>
#include <devicetree/clocks.h>
#include <devicetree/gpio.h>
#include <devicetree/spi.h>
#include <devicetree/dma.h>
#include <devicetree/zephyr.h>

#endif /* DEVICETREE_H */
