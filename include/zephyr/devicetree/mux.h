/**
 * @file
 * @brief MUX Devicetree macro public API header file.
 *
 * Pure devicetree helpers for the MUX subsystem: macros that resolve to
 * preprocessor tokens (identifiers, cell values, node identifiers,
 * struct @c device pointers via @c DEVICE_DT_GET).
 *
 * Helpers that emit @c struct @c mux_control or @c struct @c mux_state
 * initializers live in @c <zephyr/drivers/mux.h> because they require
 * the full struct definitions at expansion site.
 */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_MUX_H_
#define ZEPHYR_INCLUDE_DEVICETREE_MUX_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-mux Devicetree MUX API
 * @ingroup devicetree
 * @ingroup mux_interface
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/* Helper that emits one cell value for DT_FOREACH_PHA_CELL_BY_IDX_SEP. */
#define Z_MUX_CELL_VAL(node_id, pha, idx, cell) \
	DT_PHA_BY_IDX(node_id, pha, idx, cell)

/* Number of cells in the idx'th element of a phandle-array property. */
#define Z_MUX_PHA_NUM_CELLS(node_id, pha, idx) \
	DT_PHA_NUM_CELLS_BY_IDX(node_id, pha, idx)

/* === Internal storage identifier generators (mux-controls) === */

#define Z_MUX_CTRL_CELLS_NAME(node_id, idx) \
	DT_CAT4(zpriv_mux_ctrl_cells_, node_id, _, idx)

#define Z_MUX_CTRL_OBJ_NAME(node_id, idx) \
	DT_CAT4(zpriv_mux_ctrl_, node_id, _, idx)

/* === Internal storage identifier generators (mux-states) === */

#define Z_MUX_STATE_CELLS_NAME(node_id, idx) \
	DT_CAT4(zpriv_mux_state_cells_, node_id, _, idx)

#define Z_MUX_STATE_CTRL_NAME(node_id, idx) \
	DT_CAT4(zpriv_mux_state_ctrl_, node_id, _, idx)

#define Z_MUX_STATE_OBJ_NAME(node_id, idx) \
	DT_CAT4(zpriv_mux_state_, node_id, _, idx)

/* The first (N - 1) cells form the addressing; cell at index (N - 1) is the
 * state. The cells[] array stores only the addressing portion so the embedded
 * mux_control has the same shape as mux-controls-derived ones. The trailing
 * state value is captured into mux_state.state by position (last argument),
 * not by name, so backends are free to name the trailing cell whatever fits
 * their hardware ("state", "input", "source", ...).
 *
 * GET_ARGS_FIRST_N(N, x0, x1, ..., xK) keeps the first N arguments. Combined
 * with NUM_VA_ARGS_LESS_1(...) it yields "everything except the last", which
 * is exactly the addressing portion of a mux-states specifier.
 *
 * GET_ARG_N(N, x0, x1, ..., xK) is 1-indexed, so GET_ARG_N(K, ...) over a
 * list of K cells yields the last one (the state value).
 *
 * For 1-cell mux-states (trailing state cell only, no addressing) the cells
 * array is empty (len == 0).
 */
#define Z_MUX_STATE_ADDRESSING_CELLS(node_id, idx)                                            \
	GET_ARGS_FIRST_N(                                                                     \
		NUM_VA_ARGS_LESS_1(                                                           \
			DT_FOREACH_PHA_CELL_BY_IDX_SEP(node_id, mux_states, idx,              \
						       Z_MUX_CELL_VAL, (,))),                 \
		DT_FOREACH_PHA_CELL_BY_IDX_SEP(node_id, mux_states, idx, Z_MUX_CELL_VAL, (,)))

#define Z_MUX_STATE_LAST_CELL(node_id, idx)                                                   \
	GET_ARG_N(Z_MUX_PHA_NUM_CELLS(node_id, mux_states, idx),                              \
		DT_FOREACH_PHA_CELL_BY_IDX_SEP(node_id, mux_states, idx, Z_MUX_CELL_VAL, (,)))

/** @endcond */

/**
 * @brief Get a pointer to the @c mux_control object defined by
 *        MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX().
 *
 * The MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX() macro that allocates the underlying
 * storage lives in @c <zephyr/drivers/mux.h>; this macro just returns its
 * address.
 *
 * Example devicetree fragment:
 *
 *     mux0: mux-controller@... {
 *             compatible = "vendor,demux";
 *             #mux-control-cells = <1>;
 *             ...
 *     };
 *
 *     n: node {
 *             mux-controls = <&mux0 5>, <&mux0 7>;
 *     };
 *
 * Example usage:
 *
 *     MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX(DT_NODELABEL(n), 0);
 *     MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX(DT_NODELABEL(n), 1);
 *
 *     struct mux_control *m0 =
 *             MUX_CONTROL_DT_GET_BY_IDX(DT_NODELABEL(n), 0);  // channel 5
 *     struct mux_control *m1 =
 *             MUX_CONTROL_DT_GET_BY_IDX(DT_NODELABEL(n), 1);  // channel 7
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @param idx     Logical index into the @c mux-controls property.
 * @return Pointer to a @c struct @c mux_control.
 * @see MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX()
 */
#define MUX_CONTROL_DT_GET_BY_IDX(node_id, idx) (&Z_MUX_CTRL_OBJ_NAME(node_id, idx))

/**
 * @brief Equivalent to MUX_CONTROL_DT_GET_BY_IDX(node_id, 0).
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             mux-controls = <&mux0 5>;
 *     };
 *
 * Example usage:
 *
 *     MUX_CONTROL_DT_SPEC_DEFINE(DT_NODELABEL(n));
 *     struct mux_control *m = MUX_CONTROL_DT_GET(DT_NODELABEL(n));
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @return Pointer to a @c struct @c mux_control for the first entry.
 * @see MUX_CONTROL_DT_GET_BY_IDX()
 */
#define MUX_CONTROL_DT_GET(node_id) MUX_CONTROL_DT_GET_BY_IDX(node_id, 0)

/**
 * @brief Get a pointer to the @c mux_control object defined by
 *        MUX_CONTROL_DT_SPEC_DEFINE_BY_NAME().
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             mux-controls = <&mux0 5>, <&mux0 7>;
 *             mux-control-names = "phase_a", "phase_b";
 *     };
 *
 * Example usage:
 *
 *     MUX_CONTROL_DT_SPEC_DEFINE_BY_NAME(DT_NODELABEL(n), phase_a);
 *     struct mux_control *ma =
 *             MUX_CONTROL_DT_GET_BY_NAME(DT_NODELABEL(n), phase_a);  // channel 5
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @param name    Lowercase-and-underscores name from the
 *                @c mux-control-names property.
 * @return Pointer to a @c struct @c mux_control for the named entry.
 * @see MUX_CONTROL_DT_GET_BY_IDX()
 */
#define MUX_CONTROL_DT_GET_BY_NAME(node_id, name) \
	MUX_CONTROL_DT_GET_BY_IDX(node_id,        \
		DT_PHA_ELEM_IDX_BY_NAME(node_id, mux_controls, name))

/**
 * @brief Get a pointer to the @c mux_state object defined by
 *        MUX_STATE_DT_SPEC_DEFINE_BY_IDX().
 *
 * Example devicetree fragment:
 *
 *     mux0: mux-controller@... {
 *             compatible = "vendor,demux";
 *             #mux-control-cells = <1>;
 *             #mux-state-cells   = <2>;
 *             ...
 *     };
 *
 *     n: node {
 *             mux-states = <&mux0 5 2>, <&mux0 7 3>;
 *             // entry 0: channel=5, state=2
 *             // entry 1: channel=7, state=3
 *     };
 *
 * Example usage:
 *
 *     MUX_STATE_DT_SPEC_DEFINE_BY_IDX(DT_NODELABEL(n), 0);
 *     struct mux_state *s0 = MUX_STATE_DT_GET_BY_IDX(DT_NODELABEL(n), 0);
 *     // s0->control->cells[0] == 5, s0->state == 2
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @param idx     Logical index into the @c mux-states property.
 * @return Pointer to a @c struct @c mux_state.
 * @see MUX_STATE_DT_SPEC_DEFINE_BY_IDX()
 */
#define MUX_STATE_DT_GET_BY_IDX(node_id, idx) (&Z_MUX_STATE_OBJ_NAME(node_id, idx))

/**
 * @brief Equivalent to MUX_STATE_DT_GET_BY_IDX(node_id, 0).
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             mux-states = <&mux0 5 2>;
 *     };
 *
 * Example usage:
 *
 *     MUX_STATE_DT_SPEC_DEFINE(DT_NODELABEL(n));
 *     struct mux_state *s = MUX_STATE_DT_GET(DT_NODELABEL(n));
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @return Pointer to a @c struct @c mux_state for the first entry.
 * @see MUX_STATE_DT_GET_BY_IDX()
 */
#define MUX_STATE_DT_GET(node_id) MUX_STATE_DT_GET_BY_IDX(node_id, 0)

/**
 * @brief Get a pointer to the @c mux_state object defined by
 *        MUX_STATE_DT_SPEC_DEFINE_BY_NAME().
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             mux-states = <&mux0 5 2>, <&mux0 7 3>;
 *             mux-state-names = "default", "alt";
 *     };
 *
 * Example usage:
 *
 *     MUX_STATE_DT_SPEC_DEFINE_BY_NAME(DT_NODELABEL(n), default);
 *     struct mux_state *def = MUX_STATE_DT_GET_BY_NAME(DT_NODELABEL(n), default);
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @param name    Lowercase-and-underscores name from the
 *                @c mux-state-names property.
 * @return Pointer to a @c struct @c mux_state for the named entry.
 * @see MUX_STATE_DT_GET_BY_IDX()
 */
#define MUX_STATE_DT_GET_BY_NAME(node_id, name) \
	MUX_STATE_DT_GET_BY_IDX(node_id,        \
		DT_PHA_ELEM_IDX_BY_NAME(node_id, mux_states, name))

/**
 * @brief Get the MUX controller @c device pointer for a @c mux-controls entry.
 *
 * Equivalent to looking up the phandle of @c mux-controls at index @p idx and
 * fetching its @c struct @c device pointer with @c DEVICE_DT_GET(). Use the
 * returned device with mux_control_set() (and friends).
 *
 * Example devicetree fragment:
 *
 *     mux0: mux-controller@... { compatible = "vendor,demux"; ... };
 *     mux1: mux-controller@... { compatible = "vendor,demux"; ... };
 *
 *     n: node {
 *             mux-controls = <&mux0 5>, <&mux1 9>;
 *     };
 *
 * Example usage:
 *
 *     const struct device *d0 = MUX_CONTROL_DT_DEV_GET_BY_IDX(DT_NODELABEL(n), 0); // &mux0
 *     const struct device *d1 = MUX_CONTROL_DT_DEV_GET_BY_IDX(DT_NODELABEL(n), 1); // &mux1
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @param idx     Logical index into the @c mux-controls property.
 * @return Pointer to the @c struct @c device for the MUX controller at
 *         index @p idx.
 * @see DEVICE_DT_GET()
 */
#define MUX_CONTROL_DT_DEV_GET_BY_IDX(node_id, idx) \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, mux_controls, idx))

/**
 * @brief Equivalent to MUX_CONTROL_DT_DEV_GET_BY_IDX(node_id, 0).
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @return Pointer to the @c struct @c device for the first MUX controller.
 * @see MUX_CONTROL_DT_DEV_GET_BY_IDX()
 */
#define MUX_CONTROL_DT_DEV_GET(node_id) MUX_CONTROL_DT_DEV_GET_BY_IDX(node_id, 0)

/**
 * @brief Get the MUX controller @c device pointer for a @c mux-states entry.
 *
 * Equivalent to looking up the phandle of @c mux-states at index @p idx and
 * fetching its @c struct @c device pointer with @c DEVICE_DT_GET(). Pair with
 * MUX_STATE_DT_GET_BY_IDX() to feed mux_state_apply():
 *
 *     mux_state_apply(MUX_STATE_DT_DEV_GET_BY_IDX(node, idx),
 *                     MUX_STATE_DT_GET_BY_IDX(node, idx));
 *
 * Example devicetree fragment:
 *
 *     mux0: mux-controller@... { compatible = "vendor,demux"; ... };
 *
 *     n: node {
 *             mux-states = <&mux0 5 2>;
 *     };
 *
 * Example usage:
 *
 *     const struct device *dev = MUX_STATE_DT_DEV_GET_BY_IDX(DT_NODELABEL(n), 0); // &mux0
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @param idx     Logical index into the @c mux-states property.
 * @return Pointer to the @c struct @c device for the MUX controller at
 *         index @p idx.
 * @see DEVICE_DT_GET()
 */
#define MUX_STATE_DT_DEV_GET_BY_IDX(node_id, idx) \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, mux_states, idx))

/**
 * @brief Equivalent to MUX_STATE_DT_DEV_GET_BY_IDX(node_id, 0).
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @return Pointer to the @c struct @c device for the first MUX controller.
 * @see MUX_STATE_DT_DEV_GET_BY_IDX()
 */
#define MUX_STATE_DT_DEV_GET(node_id) MUX_STATE_DT_DEV_GET_BY_IDX(node_id, 0)

/** @cond INTERNAL_HIDDEN */

#define Z_MUX_STATE_DEFINE_FOR_EACH(node_id, prop, idx) \
	MUX_STATE_DT_SPEC_DEFINE_BY_IDX(node_id, idx);

/** @endcond */

/**
 * @brief Define a @ref mux_state object for every entry in a node's
 *        @c mux-states property.
 *
 * Convenience over calling MUX_STATE_DT_SPEC_DEFINE_BY_IDX() in a loop.
 * The emitted @ref mux_state storage is then referenced from a consumer-
 * defined entry array; the consumer chooses the entry struct shape.
 *
 * Example devicetree fragment:
 *
 *     mux0: mux-controller@... { compatible = "vendor,demux"; ... };
 *     mux1: mux-controller@... { compatible = "vendor,demux"; ... };
 *
 *     consumer: my-consumer {
 *             mux-states = <&mux0 5 2>, <&mux1 9 3>;
 *     };
 *
 * Example usage:
 *
 *     struct my_mux_entry {
 *             const struct device *dev;
 *             const struct mux_state *state;
 *     };
 *
 *     #define MY_ENTRY_INIT(node_id, prop, idx)                       \
 *             { .dev   = MUX_STATE_DT_DEV_GET_BY_IDX(node_id, idx),   \
 *               .state = MUX_STATE_DT_GET_BY_IDX(node_id, idx), },
 *
 *     MUX_STATE_DT_SPEC_DEFINE_ALL(DT_NODELABEL(consumer));
 *     static const struct my_mux_entry entries[] = {
 *             DT_FOREACH_PROP_ELEM(DT_NODELABEL(consumer), mux_states,
 *                                  MY_ENTRY_INIT)
 *     };
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @see MUX_STATE_DT_SPEC_DEFINE_BY_IDX()
 */
#define MUX_STATE_DT_SPEC_DEFINE_ALL(node_id) \
	DT_FOREACH_PROP_ELEM(node_id, mux_states, Z_MUX_STATE_DEFINE_FOR_EACH)

/**
 * @brief Equivalent to MUX_STATE_DT_SPEC_DEFINE_ALL() for @c DT_DRV_INST(inst).
 *
 * @param inst @c DT_DRV_COMPAT instance number.
 */
#define MUX_STATE_DT_INST_SPEC_DEFINE_ALL(inst) \
	MUX_STATE_DT_SPEC_DEFINE_ALL(DT_DRV_INST(inst))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEVICETREE_MUX_H_ */
