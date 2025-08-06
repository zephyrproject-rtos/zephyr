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

/** @cond INTERNAL_HIDDEN */
struct mux_control {
	/* pointer to the list of control cells (uint32_t) from the mux specifier,
	 * not including any state cell. The size of the list should be known by the
	 * controller driver since it knows its own #mux-control-cells or #mux-state-cells values.
	 */
	uint32_t *control;
	/* Whether or not the specifier is from a mux-states or mux-controls. This lets the
	 * controller driver know how many cells are in the buffer pointed to above.
	 */
	bool has_state_cell;
};

#define ZPRIV_MUX_CONTROL_DT_IDX_BY_NAME(node_id, name)				\
	DT_PHA_ELEM_IDX_BY_NAME(node_id, ZPRIV_MUX_PROP_STRING(node_id), name)

#define ZPRIV_MUX_CONTROL_DT_DEFINE_BY_IDX(node_id, idx, prop)			\
	static uint32_t zpriv_mux_control_cells_##node_id##_##idx[] = {		\
		DT_FOREACH_PHA_CELL_BY_IDX_SEP(node_id, prop, 			\
					       idx, DT_PHA_BY_IDX, (,)) 	\
	};									\
	static struct mux_control zpriv_mux_control_##node_id##_##idx = {	\
		.control = zpriv_mux_control_cells_##node_id##_##idx,		\
		.has_state_cell = DT_NODE_HAS_PROP(node_id, mux_states),	\
	};
/** @endcond */

#define MUX_CONTROL_DT_SPEC_DEFINE_BY_IDX(node_id, idx)					\
	ZPRIV_MUX_CONTROL_DT_DEFINE_BY_IDX(node_id, idx, ZPRIV_MUX_PROP_STRING(node_id))\

#define MUX_CONTROL_DT_GET_BY_IDX(node_id, idx) \
	&(DT_CAT4(zpriv_mux_control_, node_id, _, idx))

#define MUX_CONTROL_DT_SPEC_DEFINE_BY_NAME(node_id, name)				\
	ZPRIV_MUX_CONTROL_DT_DEFINE_BY_IDX(node_id, 					\
			ZPRIV_MUX_CONTROL_DT_IDX_BY_NAME(node_id, name) 		\
			ZPRIV_MUX_PROP_STRING(node_id))

#define MUX_CONTROL_DT_GET_BY_NAME(node_id, name) \
	MUX_CONTROL_DT_GET_BY_IDX(node_id, ZPRIV_MUX_CONTROL_DT_IDX_BY_NAME(node_id, name))

typedef uint32_t mux_state_t;
typedef struct mux_control *mux_control_t;

#ifdef CONFIG_MUX_CONTROL_DYNAMIC_CONFIGURE
/* in case want to do something other than what is in DT */
typedef int (*mux_configure_dynamic_fn)(const struct device *dev,
					mux_control_t control,
					mux_state_t state);
#endif

typedef int (*mux_configure_fn)(const struct device *dev,
				mux_control_t control);

typedef int (*mux_state_get_fn)(const struct device *dev,
				mux_control_t control,
				mux_state_t *state);

__subsystem struct mux_control_driver_api {
	mux_state_get_fn		state_get;
	mux_configure_fn		configure;
#ifdef CONFIG_MUX_CONTROL_DYNAMIC_CONFIGURE
	mux_configure_dynamic_fn	configure_dynamic;
#endif
};

#ifdef CONFIG_MUX_CONTROL_DYNAMIC_CONFIGURE
static inline int mux_control_configure_dynamic(const struct device *dev,
						mux_control_t control,
						mux_state_t state)
{
	const struct mux_control_driver_api *api =
		(const struct mux_control_driver_api *)dev->api;

	if (api->configure_dynamic == NULL) {
		return -ENOSYS;
	}

	return api->configure(dev, control, state);
}
#endif

/* only required API */
static inline int mux_control_configure(const struct device *dev,
					mux_control_t control)
{
	const struct mux_control_driver_api *api =
		(const struct mux_control_driver_api *)dev->api;

	return api->configure(dev, control);
}

static inline int mux_state_get(const struct device *dev,
				mux_control_t control,
				mux_state_t *state)
{
	const struct mux_control_driver_api *api =
		(const struct mux_control_driver_api *)dev->api;

	if (api->state_get == NULL) {
		return -ENOSYS;
	}

	return api->state_get(dev, control, state);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MUX_CONTROL_H_ */
