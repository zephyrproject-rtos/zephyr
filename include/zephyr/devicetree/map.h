/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_MAP_H_
#define ZEPHYR_INCLUDE_DEVICETREE_MAP_H_

/**
 * @file
 * @brief Map devicetree macro public API header file.
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
 * The map API provides the following macros for access to specific parts of a mapping entry:
 *  - DT_MAP_ENTRY_CHILD_ADDRESS_BY_IDX()
 *  - DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX()
 *  - DT_MAP_ENTRY_PARENT_ADDRESS_BY_IDX()
 *  - DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX()
 *  - DT_MAP_ENTRY_PARENT_BY_IDX()
 *
 * These macros extract, respectively, the child specifier arguments, the parent specifier
 * arguments, and the parent node argument from a mapping element identified by its node ID,
 * property name, and index.
 *
 * For instance:
 *
 * @code{.c}
 *     #define SRC_AND_DST(node_id, map, entry_idx) \
 *         { DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, map, entry_idx, 0), \
 *           DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, map, entry_idx, 0) }
 *
 *     int src_and_dst[][2] = {
 *         DT_FOREACH_MAP_ENTRY_SEP(DT_NODELABEL(n), gpio_map, SRC_AND_DST, (,))
 *     };
 * @endcode
 *
 * The above expansion yields:
 *
 * @code{.c}
 *     int src_and_dst[][2] = {{0, 2}, {4, 6}};
 * @endcode
 *
 */

/**
 * @defgroup devicetree-map Map helpers
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Returns the existence of map property.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @retval True if the map exists, otherwise 0.
 *
 * @see DT_NODE_HAS_PROP
 */
#define DT_NODE_HAS_MAP(node_id, prop) DT_NODE_HAS_PROP(node_id, prop)

/**
 * @brief Returns the number of maps for the given property.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @return The total count of mapping entries.
 *
 * @see DT_PROP_LEN
 */
#define DT_MAP_LEN(node_id, prop) DT_PROP_LEN(node_id, prop)

/**
 * @brief Is index @p idx valid for an array type property?
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx index to check
 * @return An expression which evaluates to 1 if @p idx is a valid index
 *         into the given property, and 0 otherwise.
 */
#define DT_MAP_HAS_ENTRY_BY_IDX(node_id, prop, entry_idx)                                          \
	IS_ENABLED(DT_CAT6(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _EXISTS))

/**
 * @brief Checks if the map property has any entries.
 *
 * Equivalent to calling @ref DT_MAP_HAS_ENTRY_BY_IDX with @p entry_idx set to
 * zero.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @return An expression which evaluates to 1 if @p idx is a valid index
 *         into the given property, and 0 otherwise.
 */
#define DT_MAP_HAS_ENTRY(node_id, prop) DT_MAP_HAS_ENTRY_BY_IDX(node_id, prop, 0)

/**
 * @brief Get the number of child addresses.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @return The number of child addresses.
 */
#define DT_MAP_ENTRY_CHILD_ADDRESS_LEN(node_id, prop, entry_idx)                                   \
	DT_CAT6(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _CHILD_ADDRESS_LEN)

/**
 * @brief Checks if the child address has the specified index.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @param param_idx The index in the child addresses.
 * @retval True if the child address has the index, otherwise 0.
 */
#define DT_MAP_ENTRY_HAS_CHILD_ADDRESS_BY_IDX(node_id, prop, entry_idx, param_idx)                 \
	IS_ENABLED(DT_CAT8(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _CHILD_ADDRESS_IDX_,        \
			   param_idx, _EXISTS))

/**
 * @brief Checks if the mapping entry has any child addresses.
 *
 * Equivalent to calling @ref DT_MAP_ENTRY_HAS_CHILD_ADDRESS_BY_IDX with
 * @p param_idx set to zero.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @retval True if the child address has the index, otherwise 0.
 */
#define DT_MAP_ENTRY_HAS_CHILD_ADDRESS(node_id, prop, entry_idx)                                   \
	DT_MAP_ENTRY_HAS_CHILD_ADDRESS_BY_IDX(node_id, prop, entry_idx, 0)

/**
 * @brief Get the child address element from a mapping entry, by index.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @param param_idx The index in the child addresses.
 * @return The element of the specified position of the child addresses.
 */
#define DT_MAP_ENTRY_CHILD_ADDRESS_BY_IDX(node_id, prop, entry_idx, param_idx)                     \
	DT_CAT7(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _CHILD_ADDRESS_IDX_, param_idx)

/**
 * @brief Get the first child address element from a mapping entry.
 *
 * Equivalent to calling @ref DT_MAP_ENTRY_CHILD_ADDRESS_BY_IDX with
 * @p param_idx set to zero.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @return The element of the specified position of the child addresses.
 */
#define DT_MAP_ENTRY_CHILD_ADDRESS(node_id, prop, entry_idx)                                       \
	DT_MAP_ENTRY_CHILD_ADDRESS_BY_IDX(node_id, prop, entry_idx, 0)

/**
 * @brief Get the number of child specifiers.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @return The number of child specifiers.
 */
#define DT_MAP_ENTRY_CHILD_SPECIFIER_LEN(node_id, prop, entry_idx)                                 \
	DT_CAT6(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _CHILD_SPECIFIER_LEN)

/**
 * @brief Checks if the child specifier has the specified index.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @param param_idx The index in the child specifiers.
 * @retval True if the child specifier has the index, otherwise 0.
 */
#define DT_MAP_ENTRY_HAS_CHILD_SPECIFIER_BY_IDX(node_id, prop, entry_idx, param_idx)               \
	IS_ENABLED(DT_CAT8(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _CHILD_SPECIFIER_IDX_,      \
			   param_idx, _EXISTS))

/**
 * @brief Checks if the mapping entry has any child specifiers.
 *
 * Equivalent to calling @ref DT_MAP_ENTRY_HAS_CHILD_SPECIFIER_BY_IDX with
 * @p param_idx set to zero.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @retval True if the child specifier has the index, otherwise 0.
 */
#define DT_MAP_ENTRY_HAS_CHILD_SPECIFIER(node_id, prop, entry_idx)                                 \
	DT_MAP_ENTRY_HAS_CHILD_SPECIFIER_BY_IDX(node_id, prop, entry_idx, 0)

/**
 * @brief Get the child specifier element from a mapping entry, by index.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @param param_idx The index in the child specifiers.
 * @return The element of the specified position of the child specifiers.
 */
#define DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, entry_idx, param_idx)                   \
	DT_CAT7(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _CHILD_SPECIFIER_IDX_, param_idx)

/**
 * @brief Get the first child specifier element from a mapping entry.
 *
 * Equivalent to calling @ref DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX with
 * @p param_idx set to zero.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @return The element of the specified position of the child specifiers.
 */
#define DT_MAP_ENTRY_CHILD_SPECIFIER(node_id, prop, entry_idx)                                     \
	DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, entry_idx, 0)

/**
 * @brief Extracts the parent node from a mapping entry.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @return The parent node id.
 */
#define DT_MAP_ENTRY_PARENT_BY_IDX(node_id, prop, entry_idx)                                       \
	DT_CAT6(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _PARENT)

/**
 * @brief Extracts the parent node from the first mapping entry.
 *
 * Equivalent to calling @ref DT_MAP_ENTRY_PARENT_BY_IDX with @p entry_idx set
 * to zero.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @return The parent node id.
 */
#define DT_MAP_ENTRY_PARENT(node_id, prop)                                       \
	DT_MAP_ENTRY_PARENT_BY_IDX(node_id, prop, 0)

/**
 * @brief Get the number of parent addresses.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @return The number of parent addresses.
 */
#define DT_MAP_ENTRY_PARENT_ADDRESS_LEN(node_id, prop, entry_idx)                                  \
	DT_CAT6(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _PARENT_ADDRESS_LEN)

/**
 * @brief Checks if the parent address has the specified index.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @param param_idx The index in the parent addresses.
 * @retval True if the parent address has the index, otherwise 0.
 */
#define DT_MAP_ENTRY_HAS_PARENT_ADDRESS_BY_IDX(node_id, prop, entry_idx, param_idx)                \
	IS_ENABLED(DT_CAT8(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _PARENT_ADDRESS_IDX_,       \
			   param_idx, _EXISTS))

/**
 * @brief Checks if the mapping entry has any parent addresses.
 *
 * Equivalent to calling @ref DT_MAP_ENTRY_HAS_PARENT_ADDRESS_BY_IDX with
 * @p param_idx set to zero.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @retval True if the parent address has the index, otherwise 0.
 */
#define DT_MAP_ENTRY_HAS_PARENT_ADDRESS(node_id, prop, entry_idx)                                  \
	DT_MAP_ENTRY_HAS_PARENT_ADDRESS_BY_IDX(node_id, prop, entry_idx, 0)

/**
 * @brief Get the parent address element from a mapping entry, by index.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @param param_idx The index in the parent addresses.
 * @retval The element of the specified position of the parent addresses.
 */
#define DT_MAP_ENTRY_PARENT_ADDRESS_BY_IDX(node_id, prop, entry_idx, param_idx)                    \
	DT_CAT7(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _PARENT_ADDRESS_IDX_, param_idx)

/**
 * @brief Get the first parent address element from a mapping entry.
 *
 * Equivalent to calling @ref DT_MAP_ENTRY_PARENT_ADDRESS_BY_IDX with
 * @p param_idx set to zero.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @retval The element of the specified position of the parent addresses.
 */
#define DT_MAP_ENTRY_PARENT_ADDRESS(node_id, prop, entry_idx)                                      \
	DT_MAP_ENTRY_PARENT_ADDRESS_BY_IDX(node_id, prop, entry_idx, 0)

/**
 * @brief Get the number of parent specifiers.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @retval The number of parent specifiers.
 */
#define DT_MAP_ENTRY_PARENT_SPECIFIER_LEN(node_id, prop, entry_idx)                                \
	DT_CAT6(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _PARENT_SPECIFIER_LEN)

/**
 * @brief Checks if the parent specifier has the specified index.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @param param_idx The index in the parent specifiers.
 * @retval True if the parent specifier has the index, otherwise 0.
 */
#define DT_MAP_ENTRY_HAS_PARENT_SPECIFIER_BY_IDX(node_id, prop, entry_idx, param_idx)              \
	IS_ENABLED(DT_CAT8(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _PARENT_SPECIFIER_IDX_,     \
			   param_idx, _EXISTS))

/**
 * @brief Checks if the mapping entry has any parent specifiers.
 *
 * Equivalent to calling @ref DT_MAP_ENTRY_HAS_PARENT_SPECIFIER_BY_IDX with
 * @p param_idx set to zero.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @retval True if the parent specifier has the index, otherwise 0.
 */
#define DT_MAP_ENTRY_HAS_PARENT_SPECIFIER(node_id, prop, entry_idx)                                \
	DT_MAP_ENTRY_HAS_PARENT_SPECIFIER_BY_IDX(node_id, prop, entry_idx, 0)

/**
 * @brief Get the parent specifier element from a mapping entry, by index.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @param param_idx The index in the parent specifiers.
 * @retval The element of the specified position of the parent specifiers.
 */
#define DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, entry_idx, param_idx)                  \
	DT_CAT7(node_id, _P_, prop, _MAP_ENTRY_, entry_idx, _PARENT_SPECIFIER_IDX_, param_idx)

/**
 * @brief Get the first parent specifier element from a mapping entry.
 *
 * Equivalent to calling @ref DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX with
 * @p param_idx set to zero.
 *
 * @param node_id The node identifier.
 * @param prop The map property name. i.e. "gpio_map"
 * @param entry_idx The mapping entry index.
 * @retval The element of the specified position of the parent specifiers.
 */
#define DT_MAP_ENTRY_PARENT_SPECIFIER(node_id, prop, entry_idx)                                    \
	DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node_id, prop, entry_idx, 0)

/**
 * @brief Invokes @p fn for each map entry in the @p map.
 *
 * @param node_id node identifier
 * @param prop The map property name. i.e. "gpio_map"
 * @param fn macro to invoke
 *
 * @see DT_FOREACH_PROP_ELEM
 */
#define DT_FOREACH_MAP_ENTRY(node_id, prop, fn) DT_CAT4(node_id, _P_, prop, _FOREACH_MAP_ENTRY)(fn)

/**
 * @brief Invokes @p fn for each map entry in the @p map with separator.
 *
 * @param node_id node identifier
 * @param prop The map property name. i.e. "gpio_map"
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 *
 * @see DT_FOREACH_PROP_ELEM_SEP
 */
#define DT_FOREACH_MAP_ENTRY_SEP(node_id, prop, fn, sep)                                           \
	DT_CAT4(node_id, _P_, prop, _FOREACH_MAP_ENTRY_SEP)(fn, sep)

/**
 * @brief Invokes @p fn for each map entry in the @p map with separator.
 *
 * @param node_id node identifier
 * @param prop The map property name. i.e. "gpio_map"
 * @param fn macro to invoke
 * @param ... variable number of arguments to pass to fn
 *
 * @see DT_FOREACH_PROP_ELEM_VARGS
 */
#define DT_FOREACH_MAP_ENTRY_VARGS(node_id, prop, fn, ...)                                         \
	DT_CAT4(node_id, _P_, prop, _FOREACH_MAP_ENTRY_VARGS)(fn, __VA_ARGS__)

/**
 * @brief Invokes @p fn for each map entry in the @p map with separator.
 *
 * @param node_id node identifier
 * @param prop The map property name. i.e. "gpio_map"
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 * @param ... variable number of arguments to pass to fn
 *
 * @see DT_FOREACH_PROP_ELEM_SEP_VARGS
 */
#define DT_FOREACH_MAP_ENTRY_SEP_VARGS(node_id, prop, fn, sep, ...)                                \
	DT_CAT4(node_id, _P_, prop, _FOREACH_MAP_ENTRY_SEP_VARGS)(fn, sep, __VA_ARGS__)

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DEVICETREE_MAP_H_ */
