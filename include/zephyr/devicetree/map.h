/*
 * Copyright 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_MAP_H_
#define ZEPHYR_INCLUDE_DEVICETREE_MAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Helper macro for extract args and execute GET_ARGS_LESS_N and GET_ARGS_FIRST_N.
 */
#define DT_MAP_HELPER_DO_ARGS_RANGE(start, len, ...)                                               \
	GET_ARGS_FIRST_N(len, GET_ARGS_LESS_N(start, __VA_ARGS__))

/**
 * Helper macro for extract _IDX and _LEN macro.
 */
#define DT_MAP_HELPER_ARGS_RANGE(name, node_id, prop, idx, ...)                                    \
	DT_MAP_HELPER_DO_ARGS_RANGE(DT_CAT3(DT_MAP_, name, _IDX)(node_id, prop, idx),              \
				    DT_CAT3(DT_MAP_, name, _LEN)(node_id, prop, idx), __VA_ARGS__)

/**
 * Helper macro for extract args and execute GET_ARGS_LESS_N.
 */
#define DT_MAP_HELPER_ARGS_LESS_N(N, ...) GET_ARGS_LESS_N(N, __VA_ARGS__)

/**
 * @brief Invokes @p fn for each map entry.
 *
 * The macro @p fn must take three parameters(node_id, prop, idx)
 * and variable arguments.
 *
 * @p node_id and @p prop are the same as what is passed to
 * DT_MAP_FOREACH(), and @p idx is the current index into the array.
 * The @p idx values are integer literals starting from 0.
 *
 * The @p prop argument must refer to a property that can be passed to
 * DT_MAP_LEN().
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     n: node {
 *             gpio-map = <0 1 &gpio0 2 3>, <4 5 &gpio0 6 7>;
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     #define SRC_AND_DST(node_id, prop, idx, ...) \
 *              { GET_ARG_N(1, DT_MAP_ARGS_CHILD_CELL(node_id, prop, idx, __VA_ARGS__), \
 *                GET_ARG_N(1, DT_MAP_ARGS_PARENT_CELL(node_id, prop, idx, __VA_ARGS__) }
 *
 *     int src_and_dst[2][] = {
 *             DT_MAP_FOREACH_SEP(DT_NODELABEL(n), gpio_map, SRC_AND_DST, (,))
 *     };
 * @endcode
 *
 * This expands to:
 *
 * @code{.c}
 *     int src_and_dst[2][] = {{0, 2}, {4, 6}};
 * @endcode
 *
 * In general, this macro expands to:
 *
 *     fn(node_id, prop, 0, <map[0] cells...>)
 *     fn(node_id, prop, 1, <map[1] cells...>)
 *     [...]
 *     fn(node_id, prop, n-1, <map[n-1] cells ...>)
 *
 * where `n` is the number of map entries, as it would be
 * returned by `DT_MAP_LEN(node_id, prop)`.
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @see DT_MAP_FOREACH_SEP
 * @see DT_MAP_FOREACH_VARGS
 * @see DT_MAP_FOREACH_SEP_VARGS
 * @see DT_PROP_ELEM_FOREACH
 */
#define DT_MAP_FOREACH(node_id, prop, fn) DT_CAT4(node_id, _P_, prop, _MAP_FOREACH)(fn)

/**
 * @brief Invokes @p fn for each map entry with multiple separator.
 *
 * The @p prop parameter has the same restrictions as the same parameter
 * given to DT_MAP_FOREACH().
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 *
 * @see DT_MAP_FOREACH
 * @see DT_PROP_ELEM_FOREACH_SEP
 */
#define DT_MAP_FOREACH_SEP(node_id, prop, fn, sep)                                                 \
	DT_CAT4(node_id, _P_, prop, _MAP_FOREACH_SEP)(fn, sep)

/**
 * @brief Invokes @p fn for each map entry with multiple arguments.
 *
 * The @p prop parameter has the same restrictions as the same parameter
 * given to DT_MAP_FOREACH().
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_MAP_FOREACH
 * @see DT_PROP_ELEM_FOREACH_VARGS
 */
#define DT_MAP_FOREACH_VARGS(node_id, prop, fn, ...)                                               \
	DT_CAT4(node_id, _P_, prop, _MAP_FOREACH_VARGS)(fn, __VA_ARGS__)

/**
 * @brief Invokes @p fn for each map entry with multiple arguments and separator.
 *
 * The @p prop parameter has the same restrictions as the same parameter
 * given to DT_MAP_FOREACH().
 *
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 * @param ... variable number of arguments to pass to fn
 *
 * @see DT_MAP_FOREACH
 * @see DT_PROP_ELEM_FOREACH_SEP_VARGS
 */
#define DT_MAP_FOREACH_SEP_VARGS(node_id, prop, fn, sep, ...)                                      \
	DT_CAT4(node_id, _P_, prop, _MAP_FOREACH_SEP_VARGS)(fn, sep, __VA_ARGS__)

/**
 * @brief Get a map entry by idx.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return The map entry that corresponds to the @p idx.
 *         The map entry is comma-separated values.
 */
#define DT_MAP_IDX(node_id, prop, idx) DT_CAT5(node_id, _P_, prop, _MAP_IDX_, idx)

/**
 * @brief Get a number of map entry.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @return The number of maps that the @p prop has.
 */
#define DT_MAP_LEN(node_id, prop) DT_CAT4(node_id, _P_, prop, _MAP_LEN)

/**
 * @brief Get the start index of the child address cell.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return The start index of the child address cell in map entry.
 */
#define DT_MAP_CHILD_ADDR_IDX(node_id, prop, idx)                                                  \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, CHILD_ADDR_IDX)

/**
 * @brief Get the length of the child address cell.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return The length of the child address cell in map entry.
 */
#define DT_MAP_CHILD_ADDR_LEN(node_id, prop, idx)                                                  \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, CHILD_ADDR_LEN)

/**
 * @brief Get the start index of the child address cell.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return The start index of the child address cell in map entry.
 */
#define DT_MAP_CHILD_CELL_IDX(node_id, prop, idx)                                                  \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, CHILD_CELL_IDX)

/**
 * @brief Get the length of the child address cell.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return The length of the child address cell in map entry.
 */
#define DT_MAP_CHILD_CELL_LEN(node_id, prop, idx)                                                  \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, CHILD_CELL_LEN)

/**
 * @brief Get the start index of the child address cell.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return The start index of the child address cell in map entry.
 */
#define DT_MAP_PARENT_IDX(node_id, prop, idx)                                                      \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, PARENT_IDX)

/**
 * @brief Get the length of the child address cell.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return The length of the child address cell in map entry.
 */
#define DT_MAP_PARENT_LEN(node_id, prop, idx)                                                      \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, PARENT_LEN)

/**
 * @brief Get the start index of the child address cell.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return The start index of the child address cell in map entry.
 */
#define DT_MAP_PARENT_ADDR_IDX(node_id, prop, idx)                                                 \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, PARENT_ADDR_IDX)

/**
 * @brief Get the length of the child address cell.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return The length of the child address cell in map entry.
 */
#define DT_MAP_PARENT_ADDR_LEN(node_id, prop, idx)                                                 \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, PARENT_ADDR_LEN)

/**
 * @brief Get the start index of the child address cell.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return The start index of the child address cell in map entry.
 */
#define DT_MAP_PARENT_CELL_IDX(node_id, prop, idx)                                                 \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, PARENT_CELL_IDX)

/**
 * @brief Get the length of the child address cell.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return The length of the child address cell in map entry.
 */
#define DT_MAP_PARENT_CELL_LEN(node_id, prop, idx)                                                 \
	DT_CAT7(node_id, _P_, prop, _MAP_IDX_, idx, _, PARENT_CELL_LEN)

/**
 * @brief Get the start index of the child address cell.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @return The start index of the child address cell in map entry.
 */
#define DT_MAP_VA_ARGS_IDX(node_id, prop, idx)                                                     \
	DT_MAP_HELPER_IDX_CAT(PARENT_CELL_IDX, node_id, prop, idx)

/**
 * @brief Get the child address args of map entry.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @param ... The map entry args
 * @return The child address args
 */
#define DT_MAP_ARGS_CHILD_ADDR(node_id, prop, idx, ...)                                            \
	DT_MAP_HELPER_ARGS_RANGE(CHILD_ADDR, node_id, prop, idx, __VA_ARGS__)

/**
 * @brief Get the child cell args of map entry.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @param ... The map entry args
 * @return The child cell args
 */
#define DT_MAP_ARGS_CHILD_CELL(node_id, prop, idx, ...)                                            \
	DT_MAP_HELPER_ARGS_RANGE(CHILD_CELL, node_id, prop, idx, __VA_ARGS__)

/**
 * @brief Get the parent args of map entry.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @param ... The map entry args
 * @return The parent args
 */
#define DT_MAP_ARGS_PARENT(node_id, prop, idx, ...)                                                \
	DT_MAP_HELPER_ARGS_RANGE(PARENT, node_id, prop, idx, __VA_ARGS__)

/**
 * @brief Get the parent address args of map entry.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @param ... The map entry args
 * @return The parent address args
 */
#define DT_MAP_ARGS_PARENT_ADDR(node_id, prop, idx, ...)                                           \
	DT_MAP_HELPER_ARGS_RANGE(PARENT_ADDR, node_id, prop, idx, __VA_ARGS__)

/**
 * @brief Get the parent cell args of map entry.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @param ... The map entry args
 * @return The parent cell args
 */
#define DT_MAP_ARGS_PARENT_CELL(node_id, prop, idx, ...)                                           \
	DT_MAP_HELPER_ARGS_RANGE(PARENT_CELL, node_id, prop, idx, __VA_ARGS__)

/**
 * @brief Get the variable arguments part.
 * @param node_id node identifier
 * @param prop lowercase-and-underscores property name
 * @param idx the index to get
 * @param ... The map entry args
 * @return The variable arguments part
 */
#define DT_MAP_ARGS_VARGS(node_id, prop, idx, ...)                                                 \
	DT_MAP_HELPER_ARGS_LESS_N(DT_MAP_VARGS_IDX(node_id, prop, idx), node_id, prop, idx,        \
				  __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
