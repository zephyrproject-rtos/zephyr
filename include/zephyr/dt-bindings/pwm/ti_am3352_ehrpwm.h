/*
 * SPDX-FileCopyrightText: 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PWM_TI_AM3352_EHRPWM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PWM_TI_AM3352_EHRPWM_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @name TI EHRPWM custom flags
 * These flags can be used with `pwm_set()`, `pwm_set_cycles()`, or in the devicetree `pwms`
 * property's flags cell to select how the TI EHRPWM action qualifier pairs its up/down actions.
 * The custom flags are on the upper 24 bits of the pwm_flags_t, above the bit reserved for
 * `PWM_POLARITY_NORMAL`/`PWM_POLARITY_INVERTED`.
 * @{
 */

/**
 * @brief Drive the action qualifier up/down actions from the ZRO/PRD time-base events.
 *
 * By default, the action qualifier pairs the up/down actions with the channel's own
 * compare match (CAU/CAD for channel A, CBU/CBD for channel B). Set this flag to instead
 * pair the up action with the time-base counter equal to zero (ZRO) event and the down
 * action with the time-base counter equal to period (PRD) event.
 */
#define TI_EHRPWM_USE_AQ_SET_ZRO_PRD BIT(8)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PWM_TI_AM3352_EHRPWM_H_ */
