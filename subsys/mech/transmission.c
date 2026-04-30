/*
 * Copyright (c) 2026 Sylvain Mosnier
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/mech/mech.h>

/* -------------------------------------------------------------------------
 * DT-driven instantiation
 * ---------------------------------------------------------------------- */

#define _MECH_TX_DEFINE(node_id)                                               \
	MECH_JOINT_DT_DECLARE(DT_PHANDLE(node_id, joint));                    \
	struct mech_transmission UTIL_CAT(_mech_tx_, DT_DEP_ORD(node_id)) = { \
		.joint       = MECH_JOINT_DT_PTR(DT_PHANDLE(node_id, joint)), \
		.actuator    = COND_CODE_1(                                    \
			DT_NODE_HAS_STATUS(                                    \
				DT_PHANDLE(node_id, actuator_device), okay),   \
			(DEVICE_DT_GET(DT_PHANDLE(node_id, actuator_device))),\
			(NULL)),                                               \
		.numerator   = DT_PROP(node_id, gear_ratio_numerator),         \
		.denominator = DT_PROP(node_id, gear_ratio_denominator),       \
	};

DT_FOREACH_STATUS_OKAY(zephyr_mech_transmission, _MECH_TX_DEFINE)

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

int mech_transmission_update(const struct mech_transmission *tx,
			     int32_t actuator_position,
			     int32_t actuator_velocity)
{
	/*
	 * joint = actuator * denominator / numerator
	 *
	 * Use 64-bit intermediate to avoid overflow for large values.
	 */
	int32_t pos = (int32_t)((int64_t)actuator_position
				* tx->numerator / tx->denominator);
	int32_t vel = (int32_t)((int64_t)actuator_velocity
				* tx->numerator / tx->denominator);

	return mech_joint_update(tx->joint, pos, vel);
}

int mech_transmission_joint_to_actuator(const struct mech_transmission *tx,
					int32_t joint_position,
					int32_t *actuator_position)
{
	*actuator_position = (int32_t)((int64_t)joint_position
				       * tx->denominator / tx->numerator);
	return 0;
}
