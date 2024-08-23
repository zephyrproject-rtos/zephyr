/**
 * lpm_ra2.h
 *
 * RA2l Low Power Module driver declarations
 *
 * Copyright (c) 2022-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __LPM_RA2_H__
#define __LPM_RA2_H__

/**
 * NOTE: In this module, the states are described relative to a "stop" state,
 * meaning the module is either stopped or not; as opposed to started or not.
 * When a stop state is active, the module is off.
 */

/**
 * Sets the @param module stop-state to deactivated.
 * The module can subsequently be run
 * Returns -ENODV in case of forbidden/unknown module id
 */
int lpm_ra_activate_module(unsigned int module);

/**
 * Sets the @param module stop-state to activated.
 * The module will switch off
 * Returns -ENODV in case of forbidden/unknown module id
 */
int lpm_ra_deactivate_module(unsigned int module);


/**
 * Gets the @param module stop-state.
 * @return 1 if the module is stopped, 0 is running,
 * or negative in case of error.
 * Returns -ENODV in case of forbidden/unknown module id
 */
int lpm_ra_get_module_state(unsigned int module);

enum lpm_operating_modes {
	OM_HIGH_SPEED		= 0,
	OM_MIDDLE_SPEED		= 1,
	OM_LOW_SPEED		= 3,
};

/**
 * Returns the current SOCs operational mode (high, middle or low)
 */
enum lpm_operating_modes lpm_ra_get_op_mode(void);

/**
 * Sets the SOC into the @param mode state.
 * Returns -EINVAL in case of forbidden/unknown mode
 */
int lpm_ra_set_op_mode(enum lpm_operating_modes mode);

/* Protection must be removed for this functions */
void lpm_enter_standby(void);
void lpm_leave_standby(void);

#endif /* __LPM_RA2_H__ */
