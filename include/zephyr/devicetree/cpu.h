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
 * @brief Invokes @p fn for each CPU node in the devicetree's "/cpus" node
 *
 * The macro @p fn must take one parameter, which will be the node
 * identifier of a "/cpus/cpu@N" node. The list is determined at build time
 * by the Python devicetree tooling (edt.cpus) using a structural check:
 * direct children of "/cpus" whose name prefix is "cpu". Children of
 * "/cpus" that do not match (e.g. "power-states") are excluded.
 *
 * Example devicetree fragment:
 *
 * @code{.dts}
 *     cpus {
 *             cpu0: cpu@0 {
 *                     device_type = "cpu";
 *                     reg = <0>;
 *             };
 *             cpu1: cpu@1 {
 *                     device_type = "cpu";
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
 * Notice that "power-states" is excluded.
 *
 * @param fn macro to invoke
 */
#define DT_FOREACH_CPU(fn) DT_FOREACH_CPU_HELPER(fn)

/**
 * @brief Invokes @p fn for each CPU node in the devicetree's "/cpus" node,
 *        with a separator
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
#define DT_FOREACH_CPU_SEP(fn, sep) DT_FOREACH_CPU_SEP_HELPER(fn, sep)

/**
 * @brief Invokes @p fn for each CPU node in the devicetree's "/cpus" node,
 *        with multiple arguments
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
#define DT_FOREACH_CPU_VARGS(fn, ...) DT_FOREACH_CPU_VARGS_HELPER(fn, __VA_ARGS__)

/**
 * @brief Invokes @p fn for each CPU node in the devicetree's "/cpus" node,
 *        with a separator and multiple arguments
 *
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon) placed after each
 *            invocation of @p fn. Must be in parentheses; this is required
 *            to enable providing a comma as separator.
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_CPU_SEP
 */
#define DT_FOREACH_CPU_SEP_VARGS(fn, sep, ...) DT_FOREACH_CPU_SEP_VARGS_HELPER(fn, sep, __VA_ARGS__)

/**
 * @brief Invokes @p fn for each CPU node in the devicetree's "/cpus" node
 *        whose status is "okay"
 *
 * @param fn macro to invoke
 *
 * @see DT_FOREACH_CPU
 */
#define DT_FOREACH_CPU_STATUS_OKAY(fn) DT_FOREACH_CPU_OKAY_HELPER(fn)

/**
 * @brief Invokes @p fn for each CPU node in the devicetree's "/cpus" node
 *        whose status is "okay", with a separator
 *
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon) placed after each
 *            invocation of @p fn. Must be in parentheses; this is required
 *            to enable providing a comma as separator.
 *
 * @see DT_FOREACH_CPU_SEP
 */
#define DT_FOREACH_CPU_STATUS_OKAY_SEP(fn, sep) DT_FOREACH_CPU_OKAY_SEP_HELPER(fn, sep)

/**
 * @brief Invokes @p fn for each CPU node in the devicetree's "/cpus" node
 *        whose status is "okay", with a separator and multiple arguments
 *
 * @param fn macro to invoke
 * @param sep Separator (e.g. comma or semicolon) placed after each
 *            invocation of @p fn. Must be in parentheses; this is required
 *            to enable providing a comma as separator.
 * @param ... variable number of arguments to pass to @p fn
 *
 * @see DT_FOREACH_CPU_STATUS_OKAY_SEP
 */
#define DT_FOREACH_CPU_STATUS_OKAY_SEP_VARGS(fn, sep, ...) \
	DT_FOREACH_CPU_OKAY_SEP_VARGS_HELPER(fn, sep, __VA_ARGS__)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_CPU_H_ */
