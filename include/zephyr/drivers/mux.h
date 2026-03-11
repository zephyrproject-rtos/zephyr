/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mux_interface
 * @brief Main header file for MUX (Multiplexer) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MUX_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_MUX_CONTROL_H_

/**
 * @brief Interfaces for MUX (Multiplexer).
 * @defgroup mux_interface MUX
 * @since 4.4
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/devicetree/mux.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MUX specification type
 *
 * Indicates whether a MUX specification comes from a mux-controls
 * or mux-states devicetree property.
 */
enum mux_spec_type {
	/** MUX specification from mux-controls property */
	MUX_CONTROLS_SPEC,
	/** MUX specification from mux-states property */
	MUX_STATES_SPEC,
};

/**
 * @brief MUX control structure
 *
 * This structure contains raw information from devicetree mux-controls
 * or mux-states properties converted to a C object.
 */
struct mux_control {
	/** Pointer to array of control/state cells from the devicetree specifier */
	uint32_t *cells;
	/** Number of cells in the specifier */
	size_t len;
	/** Whether the specifier is from a mux-controls or mux-states property */
	enum mux_spec_type type;
};

/** @cond INTERNAL_HIDDEN */
#define ZPRIV_MUX_PROP_SPEC_TYPE_mux_controls MUX_CONTROLS_SPEC
#define ZPRIV_MUX_PROP_SPEC_TYPE_mux_states MUX_STATES_SPEC

#define ZPRIV_MUX_PROP_TYPE(prop) \
	DT_CAT(ZPRIV_MUX_PROP_SPEC_TYPE_, prop)

#define ZPRIV_MUX_CONTROL_DT_IDX_BY_NAME(node_id, name)				\
	DT_PHA_ELEM_IDX_BY_NAME(node_id, DT_MUX_PROP_STRING(node_id), name)

#define ZPRIV_MUX_CONTROL_DT_DEFINE_BY_IDX(node_id, idx, prop)			\
	static uint32_t zpriv_mux_control_cells_##node_id##_##idx[] = {		\
		DT_FOREACH_PHA_CELL_BY_IDX_SEP(node_id, prop,			\
						idx, DT_PHA_BY_IDX, (,))	\
	};									\
	static struct mux_control zpriv_mux_control_##node_id##_##idx = {	\
		.cells = zpriv_mux_control_cells_##node_id##_##idx,		\
		.len = DT_PHA_NUM_CELLS_BY_IDX(node_id, prop, idx),		\
		.type = ZPRIV_MUX_PROP_TYPE(prop)				\
	};
/** @endcond */

/**
 * @brief Define a MUX control specification from devicetree by index
 *
 * This macro defines a static mux_control structure from a devicetree
 * node's mux-controls or mux-states property at the given index.
 *
 * @param node_id Devicetree node identifier
 * @param idx Index of the MUX control specification in the property
 */
#define MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX(node_id, idx)					\
	ZPRIV_MUX_CONTROL_DT_DEFINE_BY_IDX(node_id, idx, DT_MUX_PROP_STRING(node_id))	\

/**
 * @brief Get a reference to a MUX control specification by index
 *
 * This macro returns a pointer to the mux_control structure
 * defined by MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX().
 *
 * @param node_id Devicetree node identifier
 * @param idx Index of the MUX control specification
 * @return Pointer to the mux_control structure
 */
#define MUX_CONTROL_DT_GET_BY_IDX(node_id, idx) \
	&(DT_CAT4(zpriv_mux_control_, node_id, _, idx))

/**
 * @brief Define a MUX control specification from devicetree
 *
 * This is a convenience macro that defines a mux_control structure from
 * a devicetree node's first (index 0) mux-controls or mux-states entry.
 *
 * @param node_id Devicetree node identifier
 */
#define MUX_CONTROL_DT_SPEC_DEFINE(node_id) MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX(node_id, 0)

/**
 * @brief Get a reference to the first MUX control specification
 *
 * This macro returns a pointer to the first mux_control structure
 * defined by MUX_CONTROL_DT_SPEC_DEFINE().
 *
 * @param node_id Devicetree node identifier
 * @return Pointer to the mux_control structure
 */
#define MUX_CONTROL_DT_GET(node_id) MUX_CONTROL_DT_GET_BY_IDX(node_id, 0)

/**
 * @brief Define a MUX control specification from devicetree by name
 *
 * This macro defines a static mux_control structure from a devicetree
 * node's mux-controls or mux-states property using a named entry.
 *
 * @param node_id Devicetree node identifier
 * @param name Name of the MUX control specification
 */
#define MUX_CONTROL_DT_SPEC_DEFINE_BY_NAME(node_id, name)				\
	ZPRIV_MUX_CONTROL_DT_DEFINE_BY_IDX(node_id,					\
			ZPRIV_MUX_CONTROL_DT_IDX_BY_NAME(node_id, name),		\
			DT_MUX_PROP_STRING(node_id))

/**
 * @brief Get a reference to a MUX control specification by name
 *
 * This macro returns a pointer to the mux_control structure
 * defined by MUX_CONTROL_DT_SPEC_DEFINE_BY_NAME().
 *
 * @param node_id Devicetree node identifier
 * @param name Name of the MUX control specification
 * @return Pointer to the mux_control structure
 */
#define MUX_CONTROL_DT_GET_BY_NAME(node_id, name) \
	MUX_CONTROL_DT_GET_BY_IDX(node_id, ZPRIV_MUX_CONTROL_DT_IDX_BY_NAME(node_id, name))

/** @cond INTERNAL_HIDDEN */
/* Callback for DT_INST_FOREACH_PROP_ELEM: define one mux_control spec */
#define ZPRIV_MUX_CONTROL_DT_INST_DEFINE_ELEM(node_id, prop, idx) \
	ZPRIV_MUX_CONTROL_DT_DEFINE_BY_IDX(node_id, idx, prop)
/* Callback for DT_INST_FOREACH_PROP_ELEM_SEP: get one mux_control pointer */
#define ZPRIV_MUX_CONTROL_DT_INST_GET_ELEM(node_id, prop, idx) \
	MUX_CONTROL_DT_GET_BY_IDX(node_id, idx)
/** @endcond */

/**
 * @brief Define mux_control specs for all entries of a devicetree node.
 *
 * Iterates over every entry in a node's @c mux-states (or @c mux-controls)
 * property and defines a static @c mux_control object for each one.
 * Equivalent to calling MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX() for every valid
 * index, but without repeating the index by hand.
 *
 * After calling this macro, retrieve individual specs with
 * MUX_CONTROL_DT_GET_BY_IDX() or MUX_CONTROL_DT_GET_BY_NAME().
 *
 * @param node_id Devicetree node identifier
 */
#define MUX_CONTROL_DT_SPEC_DEFINE_ALL(node_id)				\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, mux_states),			\
		(DT_FOREACH_PROP_ELEM(node_id, mux_states,			\
			ZPRIV_MUX_CONTROL_DT_INST_DEFINE_ELEM)),		\
		(DT_FOREACH_PROP_ELEM(node_id, mux_controls,			\
			ZPRIV_MUX_CONTROL_DT_INST_DEFINE_ELEM)))

/**
 * @brief Define all mux_control specs and the pointer array for one DT instance.
 *
 * Eliminates the repetitive boilerplate that drivers need to iterate over
 * a MUX phandle-array property. Expands to static storage at file scope —
 * place it
 * inside a per-instance @c INIT macro or wrap it with a one-line
 * @c DT_INST_FOREACH_STATUS_OKAY helper.
 *
 * When the instance has no selected property the macro still emits an empty
 * pointer array so that MUX_CONTROL_DT_INST_ARRAY_GET() and
 * MUX_CONTROL_DT_INST_COUNT_BY_PROP() remain usable without a build error.
 *
 * After calling this macro for instance @p n with token @p prefix, use:
 *  - MUX_CONTROL_DT_INST_ARRAY_GET(prefix, n) to retrieve the pointer array
 *  - MUX_CONTROL_DT_INST_DEV_GET(n)           to get the MUX controller device
 *  - MUX_CONTROL_DT_INST_COUNT(n)             to get the entry count (0 if absent)
 *
 * @param prefix Unique token to avoid name collisions (e.g. the driver name)
 * @param n      DT instance number
 */
#define MUX_CONTROL_DT_INST_DEFINE_BY_PROP(prefix, n, prop)			\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prop),				\
	(									\
		DT_INST_FOREACH_PROP_ELEM(n, prop,				\
			ZPRIV_MUX_CONTROL_DT_INST_DEFINE_ELEM)			\
		static struct mux_control *prefix##_##n##_muxes[] = {		\
			DT_INST_FOREACH_PROP_ELEM_SEP(n, prop,			\
				ZPRIV_MUX_CONTROL_DT_INST_GET_ELEM, (,))	\
		};								\
	),									\
	(static struct mux_control *prefix##_##n##_muxes[] = {};))

/**
 * @brief Define all mux_control specs and the pointer array for one DT instance.
 *
 * Uses @c mux-states if present, otherwise falls back to @c mux-controls.
 * Use MUX_CONTROL_DT_INST_DEFINE_BY_PROP() when the property must be explicit.
 *
 * @param prefix Unique token to avoid name collisions (e.g. the driver name)
 * @param n      DT instance number
 */
#define MUX_CONTROL_DT_INST_DEFINE(prefix, n)					\
	MUX_CONTROL_DT_INST_DEFINE_BY_PROP(prefix, n, DT_MUX_PROP_STRING(DT_DRV_INST(n)))

/**
 * @brief Get the mux_control pointer array for a DT instance.
 *
 * Returns the array name created by MUX_CONTROL_DT_INST_DEFINE().
 *
 * @param prefix Same token used in MUX_CONTROL_DT_INST_DEFINE()
 * @param n      DT instance number
 */
#define MUX_CONTROL_DT_INST_ARRAY_GET(prefix, n) prefix##_##n##_muxes

/**
 * @brief Get the MUX controller device handle for a DT instance.
 *
 * Returns the device for the first controller referenced by the
 * @c mux-states (or @c mux-controls) property of instance @p n.
 *
 * @param n DT instance number
 * @return Pointer to the MUX controller @c struct device
 */
#define MUX_CONTROL_DT_INST_DEV_GET_BY_PROP(n, prop) \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(DT_DRV_INST(n), prop, 0))

/**
 * @brief Get the MUX controller device handle for a DT instance.
 *
 * Uses @c mux-states if present, otherwise falls back to @c mux-controls.
 * Use MUX_CONTROL_DT_INST_DEV_GET_BY_PROP() when the property must be explicit.
 *
 * @param n DT instance number
 * @return Pointer to the MUX controller @c struct device
 */
#define MUX_CONTROL_DT_INST_DEV_GET(n) \
	MUX_CONTROL_DT_INST_DEV_GET_BY_PROP(n, DT_MUX_PROP_STRING(DT_DRV_INST(n)))

/**
 * @brief Get the number of entries in a selected MUX property for a DT instance.
 *
 * Returns 0 when the instance has no selected property.
 *
 * @param n DT instance number
 * @param prop MUX property token, for example @c mux_states or @c mux_controls
 * @return Number of entries in the selected MUX property, or 0 if absent
 */
#define MUX_CONTROL_DT_INST_COUNT_BY_PROP(n, prop)				\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, prop),				\
		(DT_INST_PROP_LEN(n, prop)),				\
		(0))

/**
 * @brief Get the number of MUX entries for a DT instance.
 *
 * Uses @c mux-states if present, otherwise falls back to @c mux-controls.
 * Use MUX_CONTROL_DT_INST_COUNT_BY_PROP() when the property must be explicit.
 *
 * @param n DT instance number
 * @return Number of entries in the selected MUX property, or 0 if absent
 */
#define MUX_CONTROL_DT_INST_COUNT(n)					\
	MUX_CONTROL_DT_INST_COUNT_BY_PROP(n, DT_MUX_PROP_STRING(DT_DRV_INST(n)))


/**
 * @brief Callback API for configuring a MUX control
 *
 * @see mux_configure()
 *
 * @param dev Pointer to the MUX controller device
 * @param control Pointer to the MUX control specification
 * @param state The desired state value to configure
 * @return 0 on success, negative errno code on failure
 */
typedef int (*mux_configure_fn)(const struct device *dev,
				struct mux_control *control,
				uint32_t state);

/**
 * @brief Callback API for getting the current state of a MUX control
 *
 * @see mux_state_get()
 *
 * @param dev Pointer to the MUX controller device
 * @param control Pointer to the MUX control specification
 * @param state Pointer to store the current state value
 * @return 0 on success, negative errno code on failure
 */
typedef int (*mux_state_get_fn)(const struct device *dev,
				struct mux_control *control,
				uint32_t *state);

/**
 * @brief Callback API for locking/unlocking a MUX control
 *
 * @see mux_lock()
 *
 * @param dev Pointer to the MUX controller device
 * @param control Pointer to the MUX control specification
 * @param lock true to lock, false to unlock
 * @return 0 on success, -EBUSY if already locked, negative errno code on other failures
 */
typedef int (*mux_lock_fn)(const struct device *dev, struct mux_control *control, bool lock);

/**
 * @brief MUX controller driver API
 *
 * This structure contains the callback functions that MUX controller
 * drivers must implement.
 */
__subsystem struct mux_control_driver_api {
	/**
	 * @driver_ops_optional @copybrief mux_state_get
	 */
	mux_state_get_fn state_get;
	/**
	 * @driver_ops_mandatory @copybrief mux_configure
	 */
	mux_configure_fn configure;
	/**
	 * @driver_ops_optional @copybrief mux_lock
	 */
	mux_lock_fn lock;
};

/**
 * @brief Configure a MUX control to a specific state
 *
 * This function configures the MUX control to the specified state value.
 * Use this when you need to control the MUX state at runtime with a value
 * not defined in devicetree.
 *
 * This is useful when there are many different states and it would be
 * inefficient to define them all as separate mux-states entries in devicetree.
 * Instead, use a single mux-controls property and set the state manually.
 *
 * @param dev Pointer to the MUX controller device
 * @param control Pointer to the MUX control specification
 * @param state The desired state value to configure
 * @return 0 on success, negative errno code on failure
 */
__syscall int mux_configure(const struct device *dev,
			    struct mux_control *control,
			    uint32_t state);

static inline int z_impl_mux_configure(const struct device *dev,
				       struct mux_control *control,
				       uint32_t state)
{
	return DEVICE_API_GET(mux_control, dev)->configure(dev, control, state);
}

/**
 * @brief Configure a MUX control to its default state from devicetree
 *
 * This function configures the MUX control to the default state specified
 * in the devicetree mux-states property. This is the most common use case
 * in Zephyr.
 *
 * @note If the control specification came from a mux-controls property
 * (which does not contain a default state), this function returns -ESRCH.
 * In that case, use mux_configure() instead to specify the state manually.
 *
 * @param dev Pointer to the MUX controller device
 * @param control Pointer to the MUX control specification (must be from mux-states)
 * @return 0 on success
 * @retval -ESRCH If the control is from mux-controls (no default state)
 * @retval -ENOENT If the control has no cells (should not happen)
 * @retval Negative errno code on other failures
 */
static inline int mux_configure_default(const struct device *dev,
					struct mux_control *control)
{
	/* mux-controls properties do not contain a default state */
	if (control->type != MUX_STATES_SPEC) {
		return -ESRCH;
	}

	/* should never happen due to "#mux-state-cells" must be >= 1,
	 * but just in case, prevent some buffer overrun on the next line
	 */
	if (control->len == 0) {
		return -ENOENT;
	}

	/* the state cell is the last cell; delegate to the syscall-able API */
	return mux_configure(dev, control, control->cells[control->len - 1]);
}

/**
 * @brief Get the current state of a MUX control
 *
 * This function retrieves the current state value of the MUX control.
 *
 * @note This API is optional. If the driver does not implement it,
 * this function returns -ENOSYS.
 *
 * @param dev Pointer to the MUX controller device
 * @param control Pointer to the MUX control specification
 * @param state Pointer to store the current state value
 * @return 0 on success
 * @retval -ENOSYS If the driver does not implement this function
 * @retval Negative errno code on other failures
 */
__syscall int mux_state_get(const struct device *dev,
			    struct mux_control *control,
			    uint32_t *state);

static inline int z_impl_mux_state_get(const struct device *dev,
				       struct mux_control *control,
				       uint32_t *state)
{
	const struct mux_control_driver_api *api =
		DEVICE_API_GET(mux_control, dev);

	if (api->state_get == NULL) {
		return -ENOSYS;
	}

	return api->state_get(dev, control, state);
}

/**
 * @brief Lock or unlock a MUX control
 *
 * This function locks or unlocks the MUX control to prevent or allow
 * configuration changes.
 *
 * @note This API is optional. If the driver does not implement it,
 * this function returns -ENOSYS.
 *
 * @param dev Pointer to the MUX controller device
 * @param control Pointer to the MUX control specification
 * @param lock true to lock the MUX, false to unlock it
 * @return 0 on success
 * @retval -EBUSY If trying to lock but the MUX is already locked
 * @retval -ENOSYS If the driver does not implement this function
 * @retval Negative errno code on other failures
 */
__syscall int mux_lock(const struct device *dev, struct mux_control *control, bool lock);

static inline int z_impl_mux_lock(const struct device *dev,
				  struct mux_control *control,
				  bool lock)
{
	const struct mux_control_driver_api *api =
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

#endif /* ZEPHYR_INCLUDE_DRIVERS_MUX_CONTROL_H_ */
