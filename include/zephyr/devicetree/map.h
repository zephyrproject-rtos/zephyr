/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_MAP_H_
#define ZEPHYR_INCLUDE_DEVICETREE_MAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-map Devicetree Map API
 *
 * @brief Helper macros for handling map properties.
 *
 * This module provides helper macros that facilitate interrupt mapping and
 * specifier mapping based on DeviceTree specifications. It enables the extraction
 * and interpretation of mapping data represented as phandle-arrays.
 *
 * In a typical DeviceTree fragment, properties ending with "-map" specify:
 *  - The child specifier to be mapped.
 *  - The parent node (phandle) to which the mapping applies.
 *  - The parent specifier associated with the mapping.
 *
 * For example, when the following DeviceTree snippet is defined:
 *
 * @code{.dts}
 *     n: node {
 *         gpio-map = <0 1 &gpio0 2 3>, <4 5 &gpio0 6 7>;
 *     };
 * @endcode
 *
 * In the first mapping entry:
 *  - `0 1` are the child specifiers.
 *  - &gpio0 is the parent node.
 *  - `2 3` are the parent specifiers.
 *
 * Since map properties are implemented as phandle-arrays, macros such as
 * DT_PHANDLE_BY_IDX() and DT_PHA_BY_IDX() can be used to access individual elements.
 *
 * Both child and parent specifiers are treated as cells in a phandle-array.
 * By default, each group of specifiers is given a sequential cell name
 * (child_specifier_0, child_specifier_1, ..., parent_specifier_0, ...).
 *
 * If cell names are specified in dt-bindings, they will be used for the child specifier cell names.
 * Parent specifiers always use the default naming convention.
 *
 * Example usage:
 *
 * A mapping entry is a phandle-array whose elements can be referenced as follows:
 *  - Child specifiers can be accessed via names such as `child_specifier_0`,
 *    `child_specifier_1`, ...
 *  - The parent node is accessed via DT_PHANDLE_BY_IDX().
 *  - Parent specifiers are accessed via names such as `parent_specifier_0`,
 *    `parent_specifier_1`, ...
 *
 * @code{.c}
 *     int cspec_0 = DT_PHA_BY_IDX(DT_NODELABEL(n), gpio_map, 0, child_specifier_0); // 0
 *     int cspec_1 = DT_PHA_BY_IDX(DT_NODELABEL(n), gpio_map, 0, child_specifier_1); // 1
 *     const struct device *parent =
 *         device_get_binding(DT_PHANDLE_BY_IDX(DT_NODELABEL(n), gpio_map, 0)); // &gpio0
 *     int pspec_0 = DT_PHA_BY_IDX(DT_NODELABEL(n), gpio_map, 0, parent_specifier_0); // 2
 *     int pspec_1 = DT_PHA_BY_IDX(DT_NODELABEL(n), gpio_map, 0, parent_specifier_1); // 3
 * @endcode
 *
 * The map helper API also provides the following macros for convenient access to
 * specific parts of a mapping entry:
 *  - DT_MAP_CHILD_SPECIFIER_ARGS_BY_IDX()
 *  - DT_MAP_PARENT_SPECIFIER_ARGS_BY_IDX()
 *  - DT_MAP_PARENT_ARG_BY_IDX()
 *
 * These macros extract, respectively, the child specifier arguments, the parent specifier
 * arguments, and the parent node argument from a mapping element identified by its node ID,
 * property name, and index.
 *
 * For instance:
 *
 * @code{.c}
 *     #define SRC_AND_DST(node_id, prop, idx) \
 *         { GET_ARG_N(1, DT_MAP_CHILD_SPECIFIER_ARGS_BY_IDX(node_id, prop, idx)), \
 *           GET_ARG_N(1, DT_MAP_PARENT_SPECIFIER_ARGS_BY_IDX(node_id, prop, idx)) }
 *
 *     int src_and_dst[2][] = {
 *         DT_FOREACH_PROP_ELEM_SEP(DT_NODELABEL(n), gpio_map, SRC_AND_DST, (,))
 *     };
 * @endcode
 *
 * The above expansion yields:
 *
 * @code{.c}
 *     int src_and_dst[2][] = {{0, 2}, {4, 6}};
 * @endcode
 *
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Extracts a specified range of arguments.
 *
 * This helper macro first skips a given number of arguments and then selects
 * the first @p len arguments from the remaining list.
 *
 * @param start The number of arguments to skip.
 * @param len The number of arguments to extract after skipping.
 * @param ... The list of input arguments.
 */
#define DT_MAP_HELPER_DO_ARGS_RANGE(start, len, ...)                                               \
	GET_ARGS_FIRST_N(len, GET_ARGS_LESS_N(start, __VA_ARGS__))

/**
 * @brief Extracts a range of mapping arguments for a specific field.
 *
 * This macro concatenates the field name with the appropriate suffixes to determine
 * the starting index and length of the arguments for a map entry, and then extracts
 * those arguments.
 *
 * @param name The mapping field name (e.g., CHILD_SPECIFIER, PARENT).
 * @param node_id The node identifier.
 * @param prop The property name in lowercase and underscores.
 * @param idx The index of the mapping entry.
 * @param ... Additional arguments corresponding to the mapping entry.
 */
#define DT_MAP_HELPER_ARGS_RANGE(name, node_id, prop, idx, ...)                                    \
	DT_MAP_HELPER_DO_ARGS_RANGE(DT_CAT3(DT_MAP_, name, _POS_BY_IDX)(node_id, prop, idx),       \
				    DT_CAT3(DT_MAP_, name, _LEN_BY_IDX)(node_id, prop, idx),       \
				    __VA_ARGS__)

/**
 * @brief Retrieves the mapping entry at the specified index.
 *
 * @param node_id The node identifier.
 * @param prop The property name in lowercase with underscores.
 * @param idx The mapping entry index.
 * @return The mapping entry as a list of comma-separated values.
 */
#define DT_MAP_BY_IDX(node_id, prop, idx) DT_CAT5(node_id, _P_, prop, _MAP_IDX_, idx)

/**
 * @brief Retrieves the first mapping entry.
 * @see DT_MAP_BY_IDX
 */
#define DT_MAP(node_id, prop) DT_MAP_BY_IDX(node_id, prop, 0)

/**
 * @brief Returns the number of mapping entries for the given property.
 *
 * @param node_id The node identifier.
 * @param prop The property name in lowercase with underscores.
 * @return The total count of mapping entries.
 */
#define DT_MAP_LEN(node_id, prop) DT_CAT4(node_id, _P_, prop, _MAP_LEN)

/**
 * @brief Retrieves the starting index of the child specifier cell within a mapping entry.
 *
 * @param node_id The node identifier.
 * @param prop The property name.
 * @param idx The mapping entry index.
 * @return The starting index of the child specifier cell.
 */
#define DT_MAP_CHILD_SPECIFIER_POS_BY_IDX(node_id, prop, idx)                                      \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, CHILD_SPECIFIER_POS)

/**
 * @brief Retrieves the starting index of the child specifier cell within the first mapping entry.
 * @see DT_MAP_CHILD_SPECIFIER_POS_BY_IDX
 */
#define DT_MAP_CHILD_SPECIFIER_POS(node_id, prop)                                                  \
	DT_MAP_CHILD_SPECIFIER_POS_BY_IDX(node_id, prop, 0)

/**
 * @brief Returns the length (number of cells) of the child specifier within a mapping entry.
 *
 * @param node_id The node identifier.
 * @param prop The property name.
 * @param idx The mapping entry index.
 * @return The length (in cells) of the child specifier.
 */
#define DT_MAP_CHILD_SPECIFIER_LEN_BY_IDX(node_id, prop, idx)                                      \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, CHILD_SPECIFIER_LEN)

/**
 * @brief Returns the length (number of cells) of the child specifier within the first mapping
 * entry.
 * @see DT_MAP_CHILD_SPECIFIER_LEN_BY_IDX
 */
#define DT_MAP_CHILD_SPECIFIER_LEN(node_id, prop)                                                  \
	DT_MAP_CHILD_SPECIFIER_LEN_BY_IDX(node_id, prop, 0)

/**
 * @brief Retrieves the starting index of the parent cell in a mapping entry.
 *
 * @param node_id The node identifier.
 * @param prop The property name.
 * @param idx The mapping entry index.
 * @return The starting index of the parent cell.
 */
#define DT_MAP_PARENT_POS_BY_IDX(node_id, prop, idx)                                               \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, PARENT_POS)

/**
 * @brief Retrieves the starting index of the parent cell in the first mapping entry.
 * @see DT_MAP_PARENT_POS_BY_IDX
 */
#define DT_MAP_PARENT_POS(node_id, prop) DT_MAP_PARENT_POS_BY_IDX(node_id, prop, 0)

/**
 * @brief Returns the length (number of cells) of the parent cell in a mapping entry.
 *
 * @param node_id The node identifier.
 * @param prop The property name.
 * @param idx The mapping entry index.
 * @return The length (in cells) of the parent cell.
 */
#define DT_MAP_PARENT_LEN_BY_IDX(node_id, prop, idx)                                               \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, PARENT_LEN)

/**
 * @brief Returns the length (number of cells) of the parent cell in the first mapping entry.
 * @see DT_MAP_PARENT_LEN_BY_IDX
 */
#define DT_MAP_PARENT_LEN(node_id, prop) DT_MAP_PARENT_LEN_BY_IDX(node_id, prop, 0)

/**
 * @brief Retrieves the starting index of the parent specifier cell within a mapping entry.
 *
 * @param node_id The node identifier.
 * @param prop The property name.
 * @param idx The mapping entry index.
 * @return The starting index of the parent specifier cell.
 */
#define DT_MAP_PARENT_SPECIFIER_POS_BY_IDX(node_id, prop, idx)                                     \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, PARENT_SPECIFIER_POS)

/**
 * @brief Retrieves the starting index of the parent specifier cell within the first mapping entry.
 * @see DT_MAP_PARENT_SPECIFIER_POS_BY_IDX
 */
#define DT_MAP_PARENT_SPECIFIER_POS(node_id, prop)                                                 \
	DT_MAP_PARENT_SPECIFIER_POS_BY_IDX(node_id, prop, 0)

/**
 * @brief Returns the length (number of cells) of the parent specifier in a mapping entry.
 *
 * @param node_id The node identifier.
 * @param prop The property name.
 * @param idx The mapping entry index.
 * @return The length (in cells) of the parent specifier.
 */
#define DT_MAP_PARENT_SPECIFIER_LEN_BY_IDX(node_id, prop, idx)                                     \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, PARENT_SPECIFIER_LEN)

/**
 * @brief Returns the length (number of cells) of the parent specifier of the first mapping entry.
 * @see DT_MAP_PARENT_SPECIFIER_LEN_BY_IDX
 */
#define DT_MAP_PARENT_SPECIFIER_LEN(node_id, prop)                                                 \
	DT_MAP_PARENT_SPECIFIER_LEN_BY_IDX(node_id, prop, 0)

/**
 * @brief Extracts the child specifier arguments from a mapping entry.
 *
 * This macro returns the comma-separated list of arguments for the child specifier.
 *
 * @param node_id The node identifier.
 * @param prop The property name in lowercase with underscores.
 * @param idx The mapping entry index.
 * @return The child specifier arguments.
 */
#define DT_MAP_CHILD_SPECIFIER_ARGS_BY_IDX(node_id, prop, idx)                                     \
	DT_MAP_HELPER_ARGS_RANGE(CHILD_SPECIFIER, node_id, prop, idx,                              \
				 DT_MAP_BY_IDX(node_id, prop, idx))

/**
 * @brief Extracts the child specifier arguments from the first mapping entry.
 * @see DT_MAP_CHILD_SPECIFIER_ARGS_BY_IDX
 */
#define DT_MAP_CHILD_SPECIFIER_ARGS(node_id, prop)                                                 \
	DT_MAP_CHILD_SPECIFIER_ARGS_BY_IDX(node_id, prop, 0)

/**
 * @brief Extracts the parent node argument from a mapping entry.
 *
 * @param node_id The node identifier.
 * @param prop The property name in lowercase with underscores.
 * @param idx The mapping entry index.
 * @return The parent node argument.
 */
#define DT_MAP_PARENT_ARG_BY_IDX(node_id, prop, idx)                                               \
	DT_MAP_HELPER_ARGS_RANGE(PARENT, node_id, prop, idx, DT_MAP_BY_IDX(node_id, prop, idx))

/**
 * @brief Extracts the parent node argument from the first mapping entry.
 * @see DT_MAP_PARENT_ARG_BY_IDX
 */
#define DT_MAP_PARENT_ARG(node_id, prop) DT_MAP_PARENT_ARG_BY_IDX(node_id, prop, 0)

/**
 * @brief Extracts the parent specifier arguments from a mapping entry.
 *
 * This macro returns the comma-separated list of arguments for the parent specifier.
 *
 * @param node_id The node identifier.
 * @param prop The property name in lowercase with underscores.
 * @param idx The mapping entry index.
 * @return The parent specifier arguments.
 */
#define DT_MAP_PARENT_SPECIFIER_ARGS_BY_IDX(node_id, prop, idx)                                    \
	DT_MAP_HELPER_ARGS_RANGE(PARENT_SPECIFIER, node_id, prop, idx,                             \
				 DT_MAP_BY_IDX(node_id, prop, idx))

/**
 * @brief Extracts the parent specifier arguments of the first mapping entry.
 * @see DT_MAP_PARENT_SPECIFIER_ARGS_BY_IDX
 */
#define DT_MAP_PARENT_SPECIFIER_ARGS(node_id, prop)                                                \
	DT_MAP_PARENT_SPECIFIER_ARGS_BY_IDX(node_id, prop, 0)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_MAP_H_ */
