/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for MUX (Multiplexer) controllers.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MUX_H_
#define ZEPHYR_INCLUDE_DRIVERS_MUX_H_

/**
 * @brief MUX Controller Interface
 * @defgroup mux_interface MUX
 * @since 4.5
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/mux.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Addressing-only specifier for a single MUX control line.
 *
 * One @p mux_control identifies a single output on a MUX controller. Its
 * @c cells array carries only the addressing portion of a devicetree
 * @c mux-controls specifier (no state cell).
 */
struct mux_control {
	/** Pointer to the addressing cells from the devicetree specifier. */
	const uint32_t *cells;
	/** Number of addressing cells (matches @c \#mux-control-cells). */
	size_t len;
};

/**
 * @brief Addressing + default state pair for a MUX control line.
 *
 * One @p mux_state pairs a @ref mux_control with a default state value
 * extracted from the trailing cell of a devicetree @c mux-states specifier.
 * Use mux_state_apply() to drive the MUX to that state in one call.
 */
struct mux_state {
	/** Addressing portion (points to a statically-defined mux_control). */
	const struct mux_control *control;
	/** Default state value taken from the trailing cell of the specifier. */
	uint32_t state;
};

/** @cond INTERNAL_HIDDEN */

#define Z_MUX_CTRL_DEFINE_BY_IDX(node_id, idx)                                                  \
	static const uint32_t Z_MUX_CTRL_CELLS_NAME(node_id, idx)[] = {                         \
		COND_CODE_1(IS_EQ(Z_MUX_PHA_NUM_CELLS(node_id, mux_controls, idx), 0),          \
			    (0),                                                                \
			    (DT_FOREACH_PHA_CELL_BY_IDX_SEP(node_id, mux_controls, idx,         \
							    Z_MUX_CELL_VAL, (,))))              \
	};                                                                                      \
	static const struct mux_control Z_MUX_CTRL_OBJ_NAME(node_id, idx) = {                   \
		.cells = Z_MUX_CTRL_CELLS_NAME(node_id, idx),                                   \
		.len = Z_MUX_PHA_NUM_CELLS(node_id, mux_controls, idx),                         \
	}

#define Z_MUX_STATE_DEFINE_BY_IDX(node_id, idx)                                                 \
	static const uint32_t Z_MUX_STATE_CELLS_NAME(node_id, idx)[] = {                        \
		COND_CODE_1(IS_EQ(Z_MUX_PHA_NUM_CELLS(node_id, mux_states, idx), 1),            \
			    (0),                                                                \
			    (Z_MUX_STATE_ADDRESSING_CELLS(node_id, idx)))                       \
	};                                                                                      \
	static const struct mux_control Z_MUX_STATE_CTRL_NAME(node_id, idx) = {                 \
		.cells = Z_MUX_STATE_CELLS_NAME(node_id, idx),                                  \
		.len = Z_MUX_PHA_NUM_CELLS(node_id, mux_states, idx) - 1,                       \
	};                                                                                      \
	static const struct mux_state Z_MUX_STATE_OBJ_NAME(node_id, idx) = {                    \
		.control = &Z_MUX_STATE_CTRL_NAME(node_id, idx),                                \
		.state = Z_MUX_STATE_LAST_CELL(node_id, idx),                                   \
	}

/** @endcond */

/**
 * @brief Define a @ref mux_control object from a @c mux-controls entry.
 *
 * Emits file-scope storage for the cells array and the @ref mux_control
 * object referencing it. Pair with MUX_CONTROL_DT_GET_BY_IDX() (declared
 * in @c <zephyr/devicetree/mux.h>) to retrieve a pointer.
 *
 * Must be invoked at file scope. Each idx should only be defined once to
 * avoid duplicate storage
 *
 * Example devicetree fragment (a hypothetical multi-channel backend):
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
 *     const struct mux_control *m0 = MUX_CONTROL_DT_GET_BY_IDX(DT_NODELABEL(n), 0);
 *     const struct device *d  = MUX_CONTROL_DT_DEV_GET_BY_IDX(DT_NODELABEL(n), 0);
 *     mux_control_set(d, m0, 1);  // drive channel 5 to state 1
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @param idx     Logical index into the @c mux-controls property.
 * @see MUX_CONTROL_DT_GET_BY_IDX()
 */
#define MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX(node_id, idx) Z_MUX_CTRL_DEFINE_BY_IDX(node_id, idx)

/**
 * @brief Equivalent to MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX(node_id, 0).
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
 *     const struct mux_control *m = MUX_CONTROL_DT_GET(DT_NODELABEL(n));
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @see MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX()
 */
#define MUX_CONTROL_DT_SPEC_DEFINE(node_id) MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX(node_id, 0)

/**
 * @brief Define a @ref mux_control object by name from a @c mux-control-names
 *        entry.
 *
 * Equivalent to MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX() with the index resolved
 * from the @c mux-control-names property.
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
 *     const struct mux_control *ma =
 *             MUX_CONTROL_DT_GET_BY_NAME(DT_NODELABEL(n), phase_a);
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @param name    Lowercase-and-underscores name from the
 *                @c mux-control-names property.
 * @see MUX_CONTROL_DT_GET_BY_NAME()
 */
#define MUX_CONTROL_DT_SPEC_DEFINE_BY_NAME(node_id, name) \
	MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX(node_id,        \
		DT_PHA_ELEM_IDX_BY_NAME(node_id, mux_controls, name))

/**
 * @brief Define a @ref mux_state object from a @c mux-states entry.
 *
 * Emits file-scope storage for the addressing cells, the embedded
 * @ref mux_control, and the @ref mux_state. The leading cells of the
 * specifier become the addressing portion (@c mux_state.control->cells);
 * the trailing cell becomes @c mux_state.state.
 *
 * Must be invoked at file scope. Each idx should only be defined once
 * to avoid duplicate storage.
 *
 * Example devicetree fragment (a hypothetical multi-channel backend):
 *
 *     mux0: mux-controller@... {
 *             compatible = "vendor,demux";
 *             #mux-control-cells = <1>;
 *             #mux-state-cells   = <2>;
 *             ...
 *     };
 *
 *     n: node {
 *             // <controller channel state>
 *             mux-states = <&mux0 5 2>, <&mux0 7 3>;
 *     };
 *
 * Example usage:
 *
 *     MUX_STATE_DT_SPEC_DEFINE_BY_IDX(DT_NODELABEL(n), 0);
 *
 *     struct mux_state *s0 = MUX_STATE_DT_GET_BY_IDX(DT_NODELABEL(n), 0);
 *     // s0->control->cells[0] == 5, s0->state == 2
 *     mux_state_apply(MUX_STATE_DT_DEV_GET_BY_IDX(DT_NODELABEL(n), 0), s0);
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @param idx     Logical index into the @c mux-states property.
 * @see MUX_STATE_DT_GET_BY_IDX(), mux_state_apply()
 */
#define MUX_STATE_DT_SPEC_DEFINE_BY_IDX(node_id, idx) Z_MUX_STATE_DEFINE_BY_IDX(node_id, idx)

/**
 * @brief Equivalent to MUX_STATE_DT_SPEC_DEFINE_BY_IDX(node_id, 0).
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
 * @see MUX_STATE_DT_SPEC_DEFINE_BY_IDX()
 */
#define MUX_STATE_DT_SPEC_DEFINE(node_id) MUX_STATE_DT_SPEC_DEFINE_BY_IDX(node_id, 0)

/**
 * @brief Define a @ref mux_state object by name from a @c mux-state-names entry.
 *
 * Equivalent to MUX_STATE_DT_SPEC_DEFINE_BY_IDX() with the index resolved
 * from the @c mux-state-names property.
 *
 * Example devicetree fragment:
 *
 *     n: node {
 *             mux-states      = <&mux0 5 2>, <&mux0 7 3>;
 *             mux-state-names = "default", "alt";
 *     };
 *
 * Example usage:
 *
 *     MUX_STATE_DT_SPEC_DEFINE_BY_NAME(DT_NODELABEL(n), default);
 *     struct mux_state *def =
 *             MUX_STATE_DT_GET_BY_NAME(DT_NODELABEL(n), default);
 *
 * @param node_id Devicetree node identifier of the consumer.
 * @param name    Lowercase-and-underscores name from the
 *                @c mux-state-names property.
 * @see MUX_STATE_DT_GET_BY_NAME()
 */
#define MUX_STATE_DT_SPEC_DEFINE_BY_NAME(node_id, name) \
	MUX_STATE_DT_SPEC_DEFINE_BY_IDX(node_id,        \
		DT_PHA_ELEM_IDX_BY_NAME(node_id, mux_states, name))

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Driver callback: drive @p control to @p state.
 */
typedef int (*mux_control_set_fn)(const struct device *dev,
				  const struct mux_control *control,
				  uint32_t state);

/**
 * @brief Driver callback: read back the current state of @p control.
 *        Optional; leave NULL when the hardware cannot read back.
 */
typedef int (*mux_state_get_fn)(const struct device *dev,
					const struct mux_control *control,
					uint32_t *state);

/**
 * @brief Driver callback: lock or unlock @p control. Optional.
 */
typedef int (*mux_lock_fn)(const struct device *dev,
				   const struct mux_control *control,
				   bool lock);

/**
 * @brief MUX controller driver API.
 */
__subsystem struct mux_control_driver_api {
	mux_control_set_fn set;
	mux_state_get_fn get_state;
	mux_lock_fn lock;
};

/** @endcond */

/**
 * @brief Drive a MUX control line to the given state.
 *
 * @param dev     MUX controller device.
 * @param control Addressing of the control line to drive.
 * @param state   Target state value.
 *
 * @retval 0 On success.
 * @retval -EINVAL If addressing or state is out of range
 * @retval --EACCES If mux is locked
 * @retval -errno Other negative errno on failure.
 */
__syscall int mux_control_set(const struct device *dev,
			      const struct mux_control *control,
			      uint32_t state);

static inline int z_impl_mux_control_set(const struct device *dev,
					 const struct mux_control *control,
					 uint32_t state)
{
	const struct mux_control_driver_api *api = (const struct mux_control_driver_api *)
		DEVICE_API_GET(mux_control, dev);

	return api->set(dev, control, state);
}

/**
 * @brief Apply a devicetree-defined default state to a MUX control line.
 *
 * Convenience for @c mux-states consumers: extracts the default state from
 * @p mstate and forwards to mux_control_set().
 *
 * @param dev    MUX controller device.
 * @param mstate Addressing + default state pair.
 *
 * @retval 0 On success.
 * @retval -errno Negative errno on failure.
 */
__syscall int mux_state_apply(const struct device *dev,
			      const struct mux_state *mstate);

static inline int z_impl_mux_state_apply(const struct device *dev,
					 const struct mux_state *mstate)
{
	const struct mux_control_driver_api *api = (const struct mux_control_driver_api *)
		DEVICE_API_GET(mux_control, dev);

	return api->set(dev, mstate->control, mstate->state);
}

/**
 * @brief Read back the current state of a MUX control line.
 *
 * @param dev     MUX controller device.
 * @param control Addressing of the control line to query.
 * @param state   Output: current state value.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If the driver does not implement this operation.
 * @retval -errno Other negative errno on failure.
 */
__syscall int mux_state_get(const struct device *dev,
				    const struct mux_control *control,
				    uint32_t *state);

static inline int z_impl_mux_state_get(const struct device *dev,
					       const struct mux_control *control,
					       uint32_t *state)
{
	const struct mux_control_driver_api *api = (const struct mux_control_driver_api *)
		DEVICE_API_GET(mux_control, dev);

	if (api->get_state == NULL) {
		return -ENOSYS;
	}

	return api->get_state(dev, control, state);
}

/**
 * @brief Lock or unlock a MUX control line.
 *
 * Some MUX hardware exposes a register-lock bit that prevents further state
 * changes until reset. Drivers that support this functionality implement the
 * @c lock op; others return @c -ENOSYS.
 *
 * @param dev     MUX controller device.
 * @param control Addressing of the control line.
 * @param lock    @c true to lock, @c false to unlock.
 *
 * @retval 0 On success.
 * @retval -ENOSYS If the driver does not implement this operation.
 * @retval -ENOTSUP If the config is not supported.
 * @retval -errno Other negative errno on failure.
 */
__syscall int mux_lock(const struct device *dev,
			       const struct mux_control *control,
			       bool lock);

static inline int z_impl_mux_lock(const struct device *dev,
					  const struct mux_control *control,
					  bool lock)
{
	const struct mux_control_driver_api *api = (const struct mux_control_driver_api *)
		DEVICE_API_GET(mux_control, dev);

	if (api->lock == NULL) {
		return -ENOSYS;
	}

	return api->lock(dev, control, lock);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/mux.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_MUX_H_ */
