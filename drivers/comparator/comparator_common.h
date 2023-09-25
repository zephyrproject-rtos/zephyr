/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_COMPARATOR_COMPARATOR_COMMON_H_
#define ZEPHYR_DRIVERS_COMPARATOR_COMPARATOR_COMMON_H_

#include <zephyr/drivers/comparator.h>

#define Z_COMP_DT_CFG_NODE(node_id) \
	DT_CHILD(node_id, initial_configuration)
#define Z_COMP_DT_CFG_NAME(node_id) \
	_CONCAT(__comparator_dt_cfg, DEVICE_DT_NAME_GET(node_id))
#define Z_COMP_DT_CFG(dt_cfg_node_id) \
	.input_positive = DT_PROP(dt_cfg_node_id, input_positive), \
	.input_negative = DT_PROP(dt_cfg_node_id, input_negative), \
	.flags = DT_PROP(dt_cfg_node_id, flags),

/*
 * Helper macro to define a structure with initial configuration for a given
 * DT node.
 */
#define COMPARATOR_DT_CFG_DEFINE(node_id) \
	COND_CODE_1(DT_NODE_EXISTS(Z_COMP_DT_CFG_NODE(node_id)), \
		(static const struct comparator_cfg \
			Z_COMP_DT_CFG_NAME(node_id) = { \
				Z_COMP_DT_CFG(Z_COMP_DT_CFG_NODE(node_id)) \
			}), \
		())

/*
 * Helper macro to obtain a reference to the structure defined with
 * COMPARATOR_DT_CFG_DEFINE(), or NULL if initial configuration was
 * not provided for a given node.
 */
#define COMPARATOR_DT_CFG_GET(node_id) \
	COND_CODE_1(DT_NODE_EXISTS(Z_COMP_DT_CFG_NODE(node_id)), \
		(&Z_COMP_DT_CFG_NAME(node_id)), \
		(NULL))

/*
 * Common initialization part for comparator drivers. Applies initial
 * configuration from DT if it was provided there. Should be supplied
 * with the pointer returned by COMPARATOR_DT_CFG_GET().
 */
static inline int comparator_common_init(const struct device *dev,
					 const struct comparator_cfg *dt_cfg)
{
	if (dt_cfg) {
		return z_impl_comparator_configure(dev, dt_cfg);
	}

	return 0;
}

#endif /* ZEPHYR_DRIVERS_COMPARATOR_COMPARATOR_COMMON_H_ */
