/**
 * @file
 * @brief Clock Management Devicetree macro public API header file.
 */

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_CLOCK_MGMT_H_
#define ZEPHYR_INCLUDE_DEVICETREE_CLOCK_MGMT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/devicetree.h>

/**
 * @defgroup devicetree-clock-mgmt Devicetree Clock Management API
 * @ingroup devicetree
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/** @brief DT_CLOCK_STATE_PHA
 * Helper to add a layer of indirection when expanding state, since we want
 * to expand to an integer value (IE clock_state_0)
 */
#define DT_CLOCK_STATE_PHA(state) clock_state_ ## state

/* @brief _DT_CLOCK_STATE_ID_TOKEN
 * Internal macro that prepends CLOCK_ID to token. We require this additional
 * level of indirection because the DT_CLOCK_STATE_ID_TOKEN macro utilizes
 * EMPTY to achieve deferred expansion of the "token" argument
 */
#define _DT_CLOCK_STATE_ID_TOKEN(token) CLOCK_ID_ ## token
/** @brief DT_CLOCK_STATE_ID_TOKEN
 * Helper to get the string "clock-id" property from a clock node, and
 * transform it into a C token. This will transform the clock-id
 * "VND_CLOCK_ID" into the C token CLOCK_ID_VND_CLOCK_ID
 */
#define DT_CLOCK_STATE_ID_TOKEN(token) _DT_CLOCK_STATE_ID_TOKEN(token) EMPTY

/** @brief DT_CLOCK_STATE_APPLY_ID_INTERNAL
 * This helper adds a layer of indirection, to insure that the "state" value
 * is expanded before it is passed to underlying macros
 */
#define DT_CLOCK_STATE_APPLY_ID_INTERNAL(node, _clock_id, fn, state) \
	IF_ENABLED(DT_CLOCK_STATE_HAS_ID(node, _clock_id, state), \
		   (fn(node, DT_CLOCK_STATE_PHA(state), \
		       DT_CLOCK_STATE_ID_PH_IDX(node, _clock_id, state), \
		       DT_CLOCK_STATE_ID_TOKEN( \
					DT_STRING_UPPER_TOKEN( \
					DT_PHANDLE_BY_IDX(node, \
					DT_CLOCK_STATE_PHA(state), \
					DT_CLOCK_STATE_ID_PH_IDX(node, _clock_id, state)), \
					clock_id)))))

/** @brief DT_CLOCK_STATE_APPLY_ID_INTERNAL_VARGS
 * This helper adds a layer of indirection, to insure that the "state" value
 * is expanded before it is passed to underlying macros
 */
#define DT_CLOCK_STATE_APPLY_ID_INTERNAL_VARGS(node, _clock_id, fn, state, ...) \
	IF_ENABLED(DT_CLOCK_STATE_HAS_ID(node, _clock_id, state), \
		   (fn(node, DT_CLOCK_STATE_PHA(state), \
		       DT_CLOCK_STATE_ID_PH_IDX(node, _clock_id, state), \
		       DT_CLOCK_STATE_ID_TOKEN( \
					DT_STRING_UPPER_TOKEN( \
					DT_PHANDLE_BY_IDX(node, \
					DT_CLOCK_STATE_PHA(state), \
					DT_CLOCK_STATE_ID_PH_IDX(node, _clock_id, state)), \
					clock_id)), __VA_ARGS__)))
/** @endcond */

/**
 * @brief Test if a node's clock-"state" property refers to a clock node with
 *        a given clock ID
 *
 * This expands to 1 if given node's clock-"state" property has a phandle
 * reference to a clock node with the given clock ID.
 * Otherwise, it expands to 0.
 *
 * Example devicetree fragment:
 *
 *     div: clock-div {
 *             clock-id = "VND_CLOCK_DIV";
 *     };
 *
 *     src: clock-source {
 *            clock-id = "VND_CLOCK_SRC";
 *     };
 *
 *     node: vnd-device {
 *            clock-state-0 = <&div 3>;
 *            clock-state-1 = <&src 3>; <&div 1>;
 *            clock-state-2 = <&src 3>; <&div 1>;
 *            clock-state-3 = <&src 1>;
 *            clock-state-names = "default", "sleep", "custom";
 *     };
 *
 * Example usage:
 *
 *     #define CLOCK_STATE_CUSTOM 2
 *
 *     DT_CLOCK_STATE_HAS_ID(DT_NODELABEL(node), VND_CLOCK_DIV, CLOCK_STATE_DEFAULT) // 1
 *     DT_CLOCK_STATE_HAS_ID(DT_NODELABEL(node), VND_CLOCK_SRC, CLOCK_STATE_DEFAULT) // 0
 *     DT_CLOCK_STATE_HAS_ID(DT_NODELABEL(node), VND_CLOCK_DIV, CLOCK_STATE_SLEEP) // 1
 *     DT_CLOCK_STATE_HAS_ID(DT_NODELABEL(node), VND_CLOCK_SRC, CLOCK_STATE_SLEEP) // 1
 *     DT_CLOCK_STATE_HAS_ID(DT_NODELABEL(node), VND_CLOCK_DIV, CLOCK_STATE_CUSTOM) // 0
 *     DT_CLOCK_STATE_HAS_ID(DT_NODELABEL(node), VND_CLOCK_SRC, CLOCK_STATE_CUSTOM) // 1
 *
 * @param node node identifier; may or may not have any clock-state-"state" property
 * @param clock_id clock ID to check for in node clock state
 * @param state state identifier. Integer index equal to clock-state-"n" value
 *        in devicetree
 * @return 1 if the ID is present for the given state, 0 otherwise
 */
#define DT_CLOCK_STATE_HAS_ID(node, clock_id, state) \
	IS_ENABLED(DT_CAT6(node, _CLOCK_STATE_, state, _, clock_id, _EXISTS))

/**
 * @brief Get index of a clock ID phandle within a clock state
 *
 * This helper returns the index of a phandle for a given clock ID within
 * a clock state. If the clock ID does not exist within the clock state, using
 * this macro is an error.
 *
 * Example devicetree fragment:
 *
 *     div: clock-div {
 *             compatible = "clock-div"
 *     };
 *
 *     src: clock-source {
 *            compatible = "clock-source"
 *     };
 *
 *     node: vnd-device {
 *            clock-state-0 = <&div 3>;
 *            clock-state-1 = <&src 3>; <&div 1>;
 *            clock-state-2 = <&src 3>; <&div 1>;
 *            clock-state-3 = <&src 1>;
 *            clock-state-names = "default", "sleep", "custom";
 *     };
 *
 * Example usage:
 *
 *     #define CLOCK_STATE_CUSTOM 2
 *
 *     DT_CLOCK_STATE_ID_PH_IDX(DT_NODELABEL(node), VND_CLOCK_DIV, CLOCK_STATE_DEFAULT) // 0
 *     DT_CLOCK_STATE_ID_PH_IDX(DT_NODELABEL(node), VND_CLOCK_DIV, CLOCK_STATE_SLEEP) // 1
 *     DT_CLOCK_STATE_ID_PH_IDX(DT_NODELABEL(node), VND_CLOCK_SRC, CLOCK_STATE_SLEEP) // 0
 *     DT_CLOCK_STATE_ID_PH_IDX(DT_NODELABEL(node), VND_CLOCK_SRC, CLOCK_STATE_CUSTOM) // 0
 *
 * @param node node identifier. Must have a clock-state-"state" property.
 * @param clock_id clock ID to get phandle index for
 * @param state state identifier. Integer index equal to clock-state-"n" value
 *        in devicetree
 * @return value present in the selected cell
 */
#define DT_CLOCK_STATE_ID_PH_IDX(node, clock_id, state) \
	DT_CAT6(node, _CLOCK_STATE_, state, _, clock_id, _IDX)

/**
 * @brief Read a specifier cell for a given clock ID within a clock state
 *
 * This helper returns the specifier cell value for a given clock ID within
 * a clock state. If the clock ID does not exist within the clock state, using
 * this macro is an error.
 *
 * Example devicetree bindings
 *
 * clock-div.yaml:
 *   ...
 *   clock-cells:
 *    - div
 *   ...
 *
 * clock-source.yaml:
 *   ...
 *   clock-cells:
 *    - freq
 *   ...
 *
 * Example devicetree fragment:
 *
 *     div: clock-div {
 *             compatible = "clock-div"
 *             clock-id = "VND_CLOCK_DIV";
 *     };
 *
 *     src: clock-source {
 *            compatible = "clock-source"
 *            clock-id = "VND_CLOCK_SRC";
 *     };
 *
 *     node: vnd-device {
 *            clock-state-0 = <&div 3>;
 *            clock-state-1 = <&src 3>; <&div 1>;
 *            clock-state-2 = <&src 3>; <&div 1>;
 *            clock-state-3 = <&src 1>;
 *            clock-state-names = "default", "sleep", "custom";
 *     };
 *
 * Example usage:
 *
 *     #define CLOCK_STATE_CUSTOM 2
 *
 *     DT_CLOCK_STATE_ID_READ_CELL(DT_NODELABEL(node), VND_CLOCK_DIV, div, CLOCK_STATE_DEFAULT) // 3
 *     DT_CLOCK_STATE_ID_READ_CELL(DT_NODELABEL(node), VND_CLOCK_DIV, div, CLOCK_STATE_SLEEP) // 1
 *     DT_CLOCK_STATE_ID_READ_CELL(DT_NODELABEL(node), VND_CLOCK_SRC, freq, CLOCK_STATE_SLEEP) // 3
 *     DT_CLOCK_STATE_ID_READ_CELL(DT_NODELABEL(node), VND_CLOCK_SRC, freq, CLOCK_STATE_CUSTOM) // 1
 *
 * @param node node identifier. Must have a clock-state-"state" property.
 * @param clock_id clock ID to read specifier cell for.
 * @param name lowercase-and-underscores clock-names cell value name to check
 * @param state state identifier. Integer index equal to clock-state-"n" value
 *        in devicetree
 * @return value present in the selected cell
 */
#define DT_CLOCK_STATE_ID_READ_CELL(node, clock_id, name, state) \
	DT_PHA_BY_IDX(node, DT_CLOCK_STATE_PHA(state), \
		      DT_CLOCK_STATE_ID_PH_IDX(node, clock_id, state), \
		      name)


/**
 * @brief Read a specifier cell for a given clock ID within a clock state
 *        with a fallback value
 *
 * This helper is similar to @ref DT_CLOCK_STATE_ID_READ_CELL, but allows
 * the user to provide a fallback value that will be used if the clock ID
 * is not present within the given clock state
 * @param node node identifier. Must have a clock-state-"state" property.
 * @param clock_id clock ID to read specifier cell for.
 * @param name lowercase-and-underscores clock-names cell value name to check
 * @param state state identifier. Integer index equal to clock-state-"n" value
 *        in devicetree
 * @param fallback Fallback value to use if clock ID is not present
 * @return value present in the selected cell, or fallback value
 */
#define DT_CLOCK_STATE_ID_READ_CELL_OR(node, clock_id, name, state, fallback) \
	COND_CODE_1(DT_CLOCK_STATE_HAS_ID(node, clock_id, state), \
		    (DT_PHA_BY_IDX(node, DT_CLOCK_STATE_PHA(state), \
		      DT_CLOCK_STATE_ID_PH_IDX(node, clock_id, state), \
		      name)), (fallback))


/**
 * @brief Call macro to apply clock ID configuration for given state
 *
 * This helper will call the given macro function if a phandle referring to
 * the clock node with the provided clock ID exists for a given state. If
 * no such phandle exists, this macro expands to nothing.
 *
 * The macro function takes the following arguments:
 * - node: Node with the clocks-state-"state" property
 * - pha: name of the phandle property for clocks-state-"state"
 * - idx: index of the phandle with the given clock_id in the
 *   clock-state-"state" property
 * - clock_id: C token for the given clock ID, with CLOCK_ID_ prepended.
 *   For example, "VND_CLOCK_ID" would become CLOCK_ID_VND_CLOCK_ID
 *
 * Example C code:
 *
 * ```
 * #define CLOCK_ID_VND_CLOCK_DIV 0x100
 *
 * void apply_clock_div(uint32_t clock_id, uint32_t div_val);
 *
 * #define APPLY_CLOCK_DIV(node, pha, idx, clock_id) \
 *        apply_clock_div(clock_id, DT_PHA_BY_IDX(node, pha, idx, div));
 * ```
 *
 * Example devicetree fragment:
 * ```
 *
 *     div: clock-div {
 *             compatible = "clock-div"
 *             clock-id = "VND_CLOCK_DIV";
 *     };
 *
 *     node: vnd-device {
 *            clock-state-0 = <&div 3>;
 *            clock-state-names = "default";
 *     };
 * ```
 *
 * Example usage:
 *
 * ```
 * // Expands to: apply_clock_div(0x100, 3);
 * DT_CLOCK_STATE_APPLY_ID(DT_NODELABEL(node), VND_CLOCK_DIV, APPLY_CLOCK_DIV,
 *                         CLOCK_STATE_DEFAULT)
 * // Expands to nothing
 * DT_CLOCK_STATE_APPLY_ID(DT_NODELABEL(node), VND_CLOCK_SOURCE, APPLY_CLOCK_DIV,
 *                         CLOCK_STATE_DEFAULT)
 * ```
 *
 * Resulting C code:
 *
 * ```
 * apply_clock(0x100, 3);
 * ```
 *
 * @param node_id node identifier. Must have a clock-state-"state" property.
 * @param clock_id clock ID to apply setting for
 * @param fn macro function to call to apply clock state
 * @param state state identifier. Integer index equal to clock-state-"n" value
 *        in devicetree
 */
#define DT_CLOCK_STATE_APPLY_ID(node_id, clock_id, fn, state) \
	DT_CLOCK_STATE_APPLY_ID_INTERNAL(node_id, clock_id, fn, state)

/**
 * @brief Call macro to apply clock ID configuration for given state
 *
 * This helper will call the given macro function if a phandle referring to
 * the clock node with the provided clock ID exists for a given state. If
 * no such phandle exists, this macro expands to nothing.
 *
 * The macro @p fn takes multiple arguments. The first 4 are the following:
 * - node: Node with the clocks-state-"state" property
 * - pha: name of the phandle property for clocks-state-"state"
 * - idx: index of the phandle with the given clock_id in the
 *   clock-state-"state" property
 * - clock_id: C token for the given clock ID, with CLOCK_ID_ prepended.
 *   For example, "VND_CLOCK_ID" would become CLOCK_ID_VND_CLOCK_ID
 *
 * @param node_id node identifier. Must have a clock-state-"state" property.
 * @param clock_id clock ID to apply setting for
 * @param fn macro function to call to apply clock state
 * @param state state identifier. Integer index equal to clock-state-"n" value
 *        in devicetree
 * @param ... variable number of arguments to pass to @p fn
 * The remaining arguments are passed in by the caller
 */
#define DT_CLOCK_STATE_APPLY_ID_VARGS(node_id, clock_id, fn, state, ...) \
	DT_CLOCK_STATE_APPLY_ID_INTERNAL_VARGS(node_id, clock_id, fn, state, __VA_ARGS__)

/**
 * @brief Number of clock management states for a node identifier
 * Gets the number of clock management states for a given node identifier
 * @param node_id Node identifier to get the number of states for
 */
#define DT_NUM_CLOCK_MGMT_STATES(node_id) \
	DT_CAT(node_id, _CLOCK_STATE_NUM)

/**
 * @brief Check if a clock subsystem is referenced by a node in the
 * devicetree
 * @param clock_id Clock subsystem identifier to check
 * @return 1 if ID is used, 0 otherwise
 */
#define DT_CLOCK_ID_USED(clock_id) DT_CAT3(DT_CLOCK_ID_, clock_id, _USED)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif  /* ZEPHYR_INCLUDE_DEVICETREE_CLOCK_MGMT_H_ */
