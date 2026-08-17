/*
 * Copyright (c) 2025 ITE Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PWM_IT51XXX_PWM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PWM_IT51XXX_PWM_H_

#include <zephyr/dt-bindings/dt-util.h>

/* PWM prescaler references */
#define PWM_PRESCALER_C4	1
#define PWM_PRESCALER_C6	2
#define PWM_PRESCALER_C7	3

/* PWM channel references */
#define PWM_CHANNEL_0		0
#define PWM_CHANNEL_1		1
#define PWM_CHANNEL_2		2
#define PWM_CHANNEL_3		3
#define PWM_CHANNEL_4		4
#define PWM_CHANNEL_5		5
#define PWM_CHANNEL_6		6
#define PWM_CHANNEL_7		7

/**
 * @name PWM Dimming Duty Cycle Values
 * @{
 */
/** PWM dimming maximum duty cycle 25% */
#define PWM_DIMMING_DC_40H 0
/** PWM dimming maximum duty cycle 50% */
#define PWM_DIMMING_DC_80H 1
/** PWM dimming maximum duty cycle 75% */
#define PWM_DIMMING_DC_C0H 2
/** PWM dimming maximum duty cycle 100% */
#define PWM_DIMMING_DC_FFH 3
/**@}*/

/**
 * @name PWM Dimming Duty Increase/Decrease Step Values
 * @{
 */
/** PWM dimming duty change step: 1 */
#define PWM_DIMMING_DC_STEP_1H 0
/** PWM dimming duty change step: 2 */
#define PWM_DIMMING_DC_STEP_2H 1
/** PWM dimming duty change step: 4 */
#define PWM_DIMMING_DC_STEP_4H 2
/** PWM dimming duty change step: 8 */
#define PWM_DIMMING_DC_STEP_8H 3
/**@}*/

/**
 * @brief Provides a type to hold PWM configuration flags.
 *
 * The upper 8 bits are reserved for SoC specific flags.
 *    Output open-drain flag    [ 8 ]
 *    Output dimming mode flag  [ 9 ]
 * @{
 */
/** PWM open drain flag */
#define PWM_IT51XXX_OPEN_DRAIN   BIT(8)
/** Enable PWM dimming mode flag */
#define PWM_IT51XXX_DIMMING_MODE BIT(9)
/**@}*/

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PWM_IT51XXX_PWM_H_ */
