/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* TODO: doc */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MUX_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_MUX_CONTROL_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/devicetree/mux.h>

#ifdef __cplusplus
extern "C" {
#endif

enum mux_spec_type {
	MUX_CONTROLS_SPEC,
	MUX_STATES_SPEC,
};

/* this struct is for raw info from DT props converted to a C object */
struct mux_control {
	/* pointer to array of control/state cells, whichever is specified on the node */
	uint32_t *cells;
	/* number of cells in the specifier */
	size_t len;
	/* whether the specifier is from a mux-controls or mux-states property */
	enum mux_spec_type type;
};

/** @cond INTERNAL_HIDDEN */
#define ZPRIV_MUX_PROP_TYPE(node_id) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, mux_states), (MUX_STATES_SPEC), (MUX_CONTROLS_SPEC))

#define ZPRIV_MUX_CONTROL_DT_IDX_BY_NAME(node_id, name)				\
	DT_PHA_ELEM_IDX_BY_NAME(node_id, DT_MUX_PROP_STRING(node_id), name)

#define ZPRIV_MUX_CONTROL_DT_DEFINE_BY_IDX(node_id, idx, prop)			\
	static uint32_t zpriv_mux_control_cells_##node_id##_##idx[] = {		\
		DT_FOREACH_PHA_CELL_BY_IDX_SEP(node_id, prop, 			\
					       idx, DT_PHA_BY_IDX, (,)) 	\
	};									\
	static struct mux_control zpriv_mux_control_##node_id##_##idx = {	\
		.cells = zpriv_mux_control_cells_##node_id##_##idx,		\
		.len = DT_PHA_NUM_CELLS_BY_IDX(node_id, prop, idx),		\
		.type = ZPRIV_MUX_PROP_TYPE(node_id)				\
	};
/** @endcond */

/* Used by mux consumer to initially get the mux info from their DT node by idx */
#define MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX(node_id, idx)					\
	ZPRIV_MUX_CONTROL_DT_DEFINE_BY_IDX(node_id, idx, DT_MUX_PROP_STRING(node_id))	\

/* Used by mux consumer to reference the struct mux_control
 * defined by MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX
 */
#define MUX_CONTROL_DT_GET_BY_IDX(node_id, idx) \
	&(DT_CAT4(zpriv_mux_control_, node_id, _, idx))

/* Used by mux consumer to initially get the mux info from their DT node by idx 0 */
#define MUX_CONTROL_DT_SPEC_DEFINE(node_id) MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX(node_id, 0)

/* Used by mux consumer to reference the struct mux_control
 * defined by MUX_CONTROL_DT_SPEC_DEFINE
 */
#define MUX_CONTROL_DT_GET(node_id) MUX_CONTROL_DT_GET_BY_IDX(node_id, 0)

/* Used by mux consumer to initially get the mux info from their DT node by name */
#define MUX_CONTROL_DT_SPEC_DEFINE_BY_NAME(node_id, name)				\
	ZPRIV_MUX_CONTROL_DT_DEFINE_BY_IDX(node_id, 					\
			ZPRIV_MUX_CONTROL_DT_IDX_BY_NAME(node_id, name) 		\
			DT_MUX_PROP_STRING(node_id))

/* Used by mux consumer to reference the struct mux_control
 * defined by MUX_CONTROL_DT_SPEC_DEFINE_BY_NAME
 */
#define MUX_CONTROL_DT_GET_BY_NAME(node_id, name) \
	MUX_CONTROL_DT_GET_BY_IDX(node_id, ZPRIV_MUX_CONTROL_DT_IDX_BY_NAME(node_id, name))


typedef int (*mux_configure_fn)(const struct device *dev,
				struct mux_control *control,
				uint32_t state);

typedef int (*mux_state_get_fn)(const struct device *dev,
				struct mux_control *control,
				uint32_t *state);

typedef int (*mux_lock_fn)(const struct device *dev, struct mux_control *control, bool lock);

__subsystem struct mux_control_driver_api {
	mux_state_get_fn state_get;
	mux_configure_fn configure;
	mux_lock_fn lock;
};

/*
 * In case the consumer does know the meanings of the state value and wants
 * to control it at runtime besides from what is in DT specifiers.
 *
 * Technically all the states could be made into DT specifiers but this might be
 * inefficient for some use case especially with a lot of different state,
 * it could be better to have one struct mux_control coming from a mux-controls
 * prop in DT and set the state manually, instead of having many mux-states elements.
 *
 */
static inline int mux_configure(const struct device *dev,
				struct mux_control *control,
				uint32_t state)
{
	const struct mux_control_driver_api *api =
		(const struct mux_control_driver_api *)dev->api;

	return api->configure(dev, control, state);
}

/*
 * Configure the mux with the default state in DT if provided with mux-states.
 * This is the most expected use case in Zephyr.
 *
 * If the specifier came from a mux-controls property, then return -ESRCH.
 * If wanting to use mux-controls (or mux-states) prop and
 * specify state arbitrarily without using DT,
 * then use mux_configure instead.
 */
static inline int mux_configure_default(const struct device *dev,
					struct mux_control *control)
{
	const struct mux_control_driver_api *api =
		(const struct mux_control_driver_api *)dev->api;

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

	/* the state cell is the last cell */
	return api->configure(dev, control, control->cells[control->len - 1]);
}

/*
 * Optional API for returning mux state
 */
static inline int mux_state_get(const struct device *dev,
				struct mux_control *control,
				uint32_t *state)
{
	const struct mux_control_driver_api *api =
		(const struct mux_control_driver_api *)dev->api;

	if (api->state_get == NULL) {
		return -ENOSYS;
	}

	return api->state_get(dev, control, state);
}

/* if needing to lock a mux, return -EBUSY if trying to lock but already locked.
 * lock = 0 to unlock
 */
static inline int mux_lock(const struct device *dev, struct mux_control *control, bool lock)
{
	const struct mux_control_driver_api *api =
		(const struct mux_control_driver_api *)dev->api;

	if (api->lock == NULL) {
		return -ENOSYS;
	}

	return api->lock(dev, control, lock);
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MUX_CONTROL_H_ */
