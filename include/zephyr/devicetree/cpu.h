/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CPU devicetree macro public API header file.
 */
#ifndef ZEPHYR_INCLUDE_DEVICETREE_CPU_H_
#define ZEPHYR_INCLUDE_DEVICETREE_CPU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>

/**
 * @defgroup devicetree-cpu Devicetree Interrupt CPU API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Invokes @p fn for each child of the devicetree's "/cpus" node whose
 *        name matches "cpu@<hex>"
 *
 * The macro @p fn must take one parameter, which will be the node
 * identifier of a "/cpus" child node named "cpu@<hex>". Children of
 * "/cpus" whose name does not match "cpu@<hex>" are skipped.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     cpus {
 *             cpu0: cpu@0 {
 *                     reg = <0>;
 *             };
 *             cpu1: cpu@1 {
 *                     reg = <1>;
 *             };
 *             power-states {
 *                     idle: idle { ... };
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     #define REG_AND_COMMA(node_id) DT_REG_ADDR(node_id),
 *
 *     const uint32_t cpu_regs[] = {
 *         DT_FOREACH_CPU(REG_AND_COMMA)
 *     };
 * @endcode
 *
 * This expands to:
 *
 * @code{.c}
 *     const uint32_t cpu_regs[] = {
 *         0, 1,
 *     };
 * @endcode
 *
 * Notice that "power-states" is skipped, since its name does not match "cpu@<hex>".
 *
 * @param fn macro to invoke
 */
#define DT_FOREACH_CPU(fn) DT_FOREACH_CHILD_VARGS(DT_PATH(cpus), DT_FOREACH_CPU_INTERNAL, fn)

/**
 * @brief Invokes @p fn for each child of the devicetree's "/cpus" node whose
 *        name matches "cpu@<hex>", with a separator
 *
 * The macro @p fn must take one parameter, which will be the node
 * identifier of a "/cpus" child node named "cpu@<hex>". Children of
 * "/cpus" whose name does not match "cpu@<hex>" are skipped.
 *
 * @p sep is placed after each invocation of @p fn, not between invocations.
 * This means the expansion has a trailing @p sep, which is convenient for
 * initializers since a trailing comma is legal.
 *
 * Example usage:
 *
 * @code{.c}
 *     const uint32_t cpu_regs[] = {
 *         DT_FOREACH_CPU_SEP(DT_REG_ADDR, (,))
 *     };
 * @endcode
 *
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon) placed after each
 *            invocation of @p fn. Must be in parentheses; this is required
 *            to enable providing a comma as separator.
 *
 * @see DT_FOREACH_CPU
 */
#define DT_FOREACH_CPU_SEP(fn, sep)                                                                \
	DT_FOREACH_CHILD_VARGS(DT_PATH(cpus), DT_FOREACH_CPU_SEP_INTERNAL, fn, sep)

/**
 * @brief Invokes @p fn for each child of the devicetree's "/cpus" node whose
 *        name matches "cpu@<hex>", with multiple arguments
 *
 * The macro @p fn takes multiple arguments. The first should be the node
 * identifier for the "/cpus" child node. The remaining are passed-in by the
 * caller.
 *
 * @param fn macro to invoke
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_CPU
 */
#define DT_FOREACH_CPU_VARGS(fn, ...)                                                              \
	DT_FOREACH_CHILD_VARGS(DT_PATH(cpus), DT_FOREACH_CPU_VARGS_INTERNAL, fn, __VA_ARGS__)

/**
 * @brief Invokes @p fn for each child of the devicetree's "/cpus" node whose
 *        name matches "cpu@<hex>", with a separator and multiple arguments
 *
 * The macro @p fn takes multiple arguments. The remaining are passed-in by the caller.
 *
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon) placed after each
 *            invocation of @p fn. Must be in parentheses; this is required
 *            to enable providing a comma as separator.
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_CPU_SEP
 */
#define DT_FOREACH_CPU_SEP_VARGS(fn, sep, ...)                                                     \
	DT_FOREACH_CHILD_VARGS(DT_PATH(cpus), DT_FOREACH_CPU_SEP_VARGS_INTERNAL, fn, sep,          \
			       __VA_ARGS__)

/**
 * @brief Invokes @p fn for each child of the devicetree's "/cpus" node whose
 *        name matches "cpu@<hex>" and status is "okay"
 *
 * This macro iterates through children of the "/cpus" node in the same order
 * as they appear in the final devicetree, but children whose status is not `okay`
 * are also skipped.
 *
 * @param fn macro to invoke
 *
 * @see DT_FOREACH_CPU
 */
#define DT_FOREACH_CPU_STATUS_OKAY(fn)                                                             \
	DT_FOREACH_CHILD_VARGS(DT_PATH(cpus), DT_FOREACH_CPU_STATUS_OKAY_INTERNAL, fn)

/**
 * @brief Invokes @p fn for each child of the devicetree's "/cpus" node whose
 *        name matches "cpu@<hex>" and status is "okay", with a separator
 *
 * This macro iterates through children of the "/cpus" node in the same order
 * as they appear in the final devicetree, but children whose status is not `okay`
 * are also skipped.
 *
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon) placed after each
 *            invocation of @p fn. Must be in parentheses; this is required
 *            to enable providing a comma as separator.
 *
 * @see DT_FOREACH_CPU_SEP
 */
#define DT_FOREACH_CPU_STATUS_OKAY_SEP(fn, sep)                                                    \
	DT_FOREACH_CHILD_VARGS(DT_PATH(cpus), DT_FOREACH_CPU_STATUS_OKAY_SEP_INTERNAL, fn, sep)

/**
 * @brief Invokes @p fn for each child of the devicetree's "/cpus" node whose
 *        name matches "cpu@<hex>" and status is "okay", with a separator and
 *        multiple arguments
 *
 * This macro iterates through children of the "/cpus" node in the same order
 * as they appear in the final devicetree, but children whose status is not `okay`
 * are also skipped.
 *
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon) placed after each
 *            invocation of @p fn. Must be in parentheses; this is required
 *            to enable providing a comma as separator.
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_CPU_STATUS_OKAY_SEP
 */
#define DT_FOREACH_CPU_STATUS_OKAY_SEP_VARGS(fn, sep, ...)                                         \
	DT_FOREACH_CHILD_VARGS(DT_PATH(cpus), DT_FOREACH_CPU_STATUS_OKAY_SEP_VARGS_INTERNAL, fn,   \
			       sep, __VA_ARGS__)

/** @cond INTERNAL_HIDDEN */
/** @brief Helper: does node_id's name strictly match "cpu@<hex>"? */
#define DT_NODE_NAME_IS_CPU_UNIT_ADDR(node_id) \
	IS_ENABLED(DT_CAT(node_id, _NAME_IS_CPU_UNIT_ADDR))

#define DT_FOREACH_CPU_INTERNAL(node_id, fn)                                                       \
	COND_CODE_1(DT_NODE_NAME_IS_CPU_UNIT_ADDR(node_id), (fn(node_id)), ())

#define DT_FOREACH_CPU_VARGS_INTERNAL(node_id, fn, ...)                                            \
	COND_CODE_1(DT_NODE_NAME_IS_CPU_UNIT_ADDR(node_id), \
		    (fn(node_id, __VA_ARGS__)), ())

#define DT_FOREACH_CPU_SEP_INTERNAL(node_id, fn, sep)                                              \
	COND_CODE_1(DT_NODE_NAME_IS_CPU_UNIT_ADDR(node_id), \
		    (fn(node_id) DT_DEBRACKET_INTERNAL sep), ())

#define DT_FOREACH_CPU_SEP_VARGS_INTERNAL(node_id, fn, sep, ...)                                   \
	COND_CODE_1(DT_NODE_NAME_IS_CPU_UNIT_ADDR(node_id), \
		    (fn(node_id, __VA_ARGS__) DT_DEBRACKET_INTERNAL sep), ())

#define DT_FOREACH_CPU_STATUS_OKAY_INTERNAL(node_id, fn)                                           \
	COND_CODE_1(DT_NODE_NAME_IS_CPU_UNIT_ADDR(node_id), \
		    (COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(node_id), (fn(node_id)), ())), ())

#define DT_FOREACH_CPU_STATUS_OKAY_SEP_INTERNAL(node_id, fn, sep)                                  \
	COND_CODE_1(DT_NODE_NAME_IS_CPU_UNIT_ADDR(node_id), \
		    (COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(node_id), \
				  (fn(node_id) DT_DEBRACKET_INTERNAL sep), ())), \
		    ())

#define DT_FOREACH_CPU_STATUS_OKAY_SEP_VARGS_INTERNAL(node_id, fn, sep, ...)                       \
	COND_CODE_1(DT_NODE_NAME_IS_CPU_UNIT_ADDR(node_id), \
		    (COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(node_id), \
				  (fn(node_id, __VA_ARGS__) DT_DEBRACKET_INTERNAL sep), ())), \
		    ())
/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_CPU_H_ */
